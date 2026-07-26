// SPDX-License-Identifier: GPL-2.0-only
/*
 * rfsa_daemon - userspace RFSA (service 0x1C) for shared-memory clients.
 *
 * The 4.4-derived MPSS queries RFSA service 0x1C at boot to obtain the
 * physical addresses of the rmtfs (client_id=1) and oembackup (client_id=4)
 * buffers. The latter is used for NV backup/restore which carries IMEI. On
 * 4.19 the kernel no longer hosts RFSA (sharedmem_qmi.c was dropped), so
 * this daemon maps both /dev/uioN regions and answers GET_BUFF_ADDR.
 *
 * Modelled on qmi_rmt_storage/rmt_storage_svc.c (RMTFS service), stripped
 * down to the two stock RFSA clients and a single message (GET_BUFF_ADDR).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/select.h>

#define LOG_TAG "rfsa_daemon"
#include <cutils/log.h>

#include "common_v01.h"
#include "qmi_idl_lib.h"
#include "qmi_csi.h"
#include "remote_filesystem_access_v01.h"

#define UIO_NAME_RMTFS       "rmtfs"
#define UIO_NAME_OEMBACK     "oembackup"
#define UIO_VERSION_EXPECTED "1.0"

struct rfsa_client {
	uint32_t  client_id;
	const char *uio_name;
	void     *shared_mem;       /* mmap result, kept for sanity/future use */
	uint64_t  shared_mem_phys;  /* physical address returned to modem */
	uint64_t  shared_mem_size;
	int       uio_fd;
};

struct rfsa_state {
	struct rfsa_client clients[2];
	qmi_csi_service_handle service_handle;
};

static struct rfsa_state g_state = {
	.clients = {
		{
			.client_id = RFSA_RMTFS_CLIENT_ID_V01,
			.uio_name = UIO_NAME_RMTFS,
			.uio_fd = -1,
		},
		{
			.client_id = RFSA_OEMBACK_CLIENT_ID_V01,
			.uio_name = UIO_NAME_OEMBACK,
			.uio_fd = -1,
		},
	},
};

/* Read a sysfs file into buf and trim trailing newline. 0 on success. */
static int read_sysfs(const char *path, char *buf, size_t len)
{
	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;
	ssize_t n = read(fd, buf, len - 1);
	close(fd);
	if (n <= 0)
		return -1;
	buf[n] = '\0';
	while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
		buf[--n] = '\0';
	return 0;
}

static int read_sysfs_u64(const char *path, uint64_t *value)
{
	char buf[32];
	char *end;
	unsigned long long parsed;

	if (read_sysfs(path, buf, sizeof(buf)) != 0)
		return -1;

	errno = 0;
	parsed = strtoull(buf, &end, 0);
	if (errno != 0 || end == buf || *end != '\0')
		return -1;

	*value = parsed;
	return 0;
}

static int parse_uio_num(const char *name, unsigned int *dev_num)
{
	char *end;
	unsigned long parsed;

	if (strncmp(name, "uio", 3) != 0 || name[3] == '\0')
		return -1;

	errno = 0;
	parsed = strtoul(name + 3, &end, 10);
	if (errno != 0 || *end != '\0' || parsed > UINT_MAX)
		return -1;

	*dev_num = (unsigned int)parsed;
	return 0;
}

/* Find the UIO device for one RFSA client, mmap it, and capture its phys addr.
 * Mirrors rmt_storage's uio_device_setup_shared_mem + setup_client_mmap_buffer
 * while keeping the client-id-to-buffer association explicit. */
static int setup_client_mmap(struct rfsa_client *client)
{
	DIR *d = opendir("/sys/class/uio");
	if (!d) {
		ALOGE("open /sys/class/uio failed: %s", strerror(errno));
		return -1;
	}

	char path[96], val[32], dev_path[32];
	struct dirent *e;

	while ((e = readdir(d)) != NULL) {
		unsigned int dev_num;
		uint64_t shared_mem_size;
		uint64_t shared_mem_phys;

		if (parse_uio_num(e->d_name, &dev_num) != 0)
			continue;

		snprintf(path, sizeof(path), "/sys/class/uio/%s/name", e->d_name);
		if (read_sysfs(path, val, sizeof(val)) != 0)
			continue;
		if (strcmp(val, client->uio_name) != 0)
			continue;

		snprintf(path, sizeof(path), "/sys/class/uio/%s/version",
			 e->d_name);
		if (read_sysfs(path, val, sizeof(val)) != 0) {
			ALOGE("failed to read %s version", e->d_name);
			closedir(d);
			return -1;
		}
		if (strcmp(val, UIO_VERSION_EXPECTED) != 0) {
			ALOGE("%s has unsupported UIO version '%s'", e->d_name,
			      val);
			closedir(d);
			return -1;
		}

		snprintf(path, sizeof(path),
			 "/sys/class/uio/%s/maps/map0/size", e->d_name);
		if (read_sysfs_u64(path, &shared_mem_size) != 0) {
			ALOGE("failed to read %s size", e->d_name);
			closedir(d);
			return -1;
		}

		snprintf(path, sizeof(path),
			 "/sys/class/uio/%s/maps/map0/addr", e->d_name);
		if (read_sysfs_u64(path, &shared_mem_phys) != 0) {
			ALOGE("failed to read %s address", e->d_name);
			closedir(d);
			return -1;
		}
		if (shared_mem_size == 0 || shared_mem_phys == 0) {
			ALOGE("%s has invalid addr/size", e->d_name);
			closedir(d);
			return -1;
		}

		snprintf(dev_path, sizeof(dev_path), "/dev/uio%u", dev_num);
		client->uio_fd = open(dev_path, O_RDWR | O_CLOEXEC);
		if (client->uio_fd < 0) {
			ALOGE("open %s failed: %s", dev_path, strerror(errno));
			closedir(d);
			return -1;
		}
		client->shared_mem = mmap(NULL, shared_mem_size,
					  PROT_READ | PROT_WRITE, MAP_SHARED,
					  client->uio_fd, 0);
		if (client->shared_mem == MAP_FAILED) {
			ALOGE("mmap %s failed: %s", dev_path, strerror(errno));
			close(client->uio_fd);
			client->uio_fd = -1;
			closedir(d);
			return -1;
		}

		client->shared_mem_phys = shared_mem_phys;
		client->shared_mem_size = shared_mem_size;
		ALOGI("%s client_id=%u %s: phys=0x%llx size=0x%llx",
		      client->uio_name, client->client_id, dev_path,
		      (unsigned long long)client->shared_mem_phys,
		      (unsigned long long)client->shared_mem_size);
		closedir(d);
		return 0;
	}
	closedir(d);

	ALOGE("no UIO device named '%s' found", client->uio_name);
	return -1;
}

static struct rfsa_client *find_client(uint32_t client_id)
{
	size_t i;

	for (i = 0; i < sizeof(g_state.clients) / sizeof(g_state.clients[0]);
	     ++i) {
		if (g_state.clients[i].client_id == client_id)
			return &g_state.clients[i];
	}
	return NULL;
}

static qmi_csi_cb_error rfsa_connect_cb(qmi_client_handle client,
					void *service_cookie,
					void **connection_handle)
{
	*connection_handle = (void *)client;
	return QMI_CSI_CB_NO_ERR;
}

static void rfsa_disconnect_cb(void *connection_handle, void *service_cookie)
{
}

static qmi_csi_cb_error rfsa_handle_req_cb(void *connection_handle,
					   qmi_req_handle req_handle,
					   unsigned int msg_id,
					   void *req_c_struct,
					   unsigned int req_c_struct_len,
					   void *service_cookie)
{
	if (msg_id != QMI_RFSA_GET_BUFF_ADDR_REQ_MSG_V01) {
		ALOGE("unexpected msg_id 0x%x", msg_id);
		return QMI_CSI_CB_INTERNAL_ERR;
	}

	rfsa_get_buff_addr_req_msg_v01 *req = req_c_struct;
	rfsa_get_buff_addr_resp_msg_v01 resp;
	struct rfsa_client *client = NULL;
	memset(&resp, 0, sizeof(resp));

	if (req && req_c_struct_len == sizeof(*req))
		client = find_client(req->client_id);

	if (!client || req->size == 0 ||
	    req->size > client->shared_mem_size) {
		ALOGE("invalid GET_BUFF_ADDR client_id=%u size=%u len=%u",
		      req ? req->client_id : 0, req ? req->size : 0,
		      req_c_struct_len);
		resp.resp.result = QMI_RESULT_FAILURE_V01;
		resp.resp.error = QMI_ERR_INVALID_ARG_V01;
	} else {
		ALOGI("GET_BUFF_ADDR client_id=%u size=%u -> phys=0x%llx",
		      req->client_id, req->size,
		      (unsigned long long)client->shared_mem_phys);
		resp.resp.result = QMI_RESULT_SUCCESS_V01;
		resp.resp.error = QMI_ERR_NONE_V01;
		resp.address_valid = 1;
		resp.address = client->shared_mem_phys;
	}

	qmi_csi_error rc = qmi_csi_send_resp(req_handle,
					     QMI_RFSA_GET_BUFF_ADDR_RESP_MSG_V01,
					     &resp, sizeof(resp));
	if (rc != QMI_CSI_NO_ERR)
		ALOGE("send_resp failed: %d", rc);
	return (rc == QMI_CSI_NO_ERR) ? QMI_CSI_CB_NO_ERR
				      : QMI_CSI_CB_INTERNAL_ERR;
}

int main(int argc, char **argv)
{
	size_t i;

	(void)argc; (void)argv;

	for (i = 0; i < sizeof(g_state.clients) / sizeof(g_state.clients[0]);
	     ++i) {
		if (setup_client_mmap(&g_state.clients[i]) != 0) {
			ALOGE("%s mmap failed; exiting for init restart",
			      g_state.clients[i].uio_name);
			return 1;
		}
	}

	qmi_csi_os_params os_params, os_params_in;
	qmi_csi_options options;
	qmi_csi_error rc;

	QMI_CSI_OPTIONS_INIT(options);
	QMI_CSI_OPTIONS_SET_INSTANCE_ID(options, RFSA_SERVICE_INSTANCE_V01);

	rc = qmi_csi_register_with_options(rfsa_get_service_object_v01(),
					   rfsa_connect_cb, rfsa_disconnect_cb,
					   rfsa_handle_req_cb, &g_state,
					   &os_params, &options,
					   &g_state.service_handle);
	if (rc != QMI_CSI_NO_ERR) {
		ALOGE("qmi_csi_register failed: %d", rc);
		return 1;
	}
	ALOGI("RFSA service 0x1C instance %u registered, waiting for modem queries",
	      RFSA_SERVICE_INSTANCE_V01);

	for (;;) {
		fd_set fds = os_params.fds;
		int ret = select(os_params.max_fd + 1, &fds, NULL, NULL, NULL);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			ALOGE("select failed: %s", strerror(errno));
			break;
		}
		os_params_in.fds = fds;
		qmi_csi_handle_event(g_state.service_handle, &os_params_in);
	}

	qmi_csi_unregister(g_state.service_handle);
	return 0;
}

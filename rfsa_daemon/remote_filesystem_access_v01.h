/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * RFSA (Remote File System Access) QMI service - userspace definitions.
 *
 * Service ID 0x1C. Mirrors the kernel-side remote_filesystem_access_v01.h
 * from drivers/uio/msm_sharedmem (Qualcomm 4.4), re-expressed in the
 * userspace QMI IDL layout used by qmi-framework (see remote_storage_v01).
 *
 * GET_BUFF_ADDR returns the physical address of the shared-memory region
 * selected by client_id. The stock kernel publishes both rmtfs (client 1)
 * and oembackup (client 4) through this service.
 */
#ifndef __REMOTE_FILESYSTEM_ACCESS_V01_H__
#define __REMOTE_FILESYSTEM_ACCESS_V01_H__

#include "common_v01.h"

#define RFSA_SERVICE_ID_V01             0x1C
#define RFSA_SERVICE_VERS_V01           0x01
#define RFSA_SERVICE_INSTANCE_V01       0x01
#define RFSA_RMTFS_CLIENT_ID_V01        0x01
#define RFSA_OEMBACK_CLIENT_ID_V01      0x04

#define QMI_RFSA_GET_BUFF_ADDR_REQ_MSG_V01   0x0023
#define QMI_RFSA_GET_BUFF_ADDR_RESP_MSG_V01  0x0023

#define RFSA_GET_BUFF_ADDR_REQ_MSG_MAX_LEN_V01   14
#define RFSA_GET_BUFF_ADDR_RESP_MSG_MAX_LEN_V01  18

/* IDL version tags consumed by the service-object accessor below. */
#define RFSA_V01_IDL_MAJOR_VERS  0x01
#define RFSA_V01_IDL_MINOR_VERS  0x01
#define RFSA_V01_IDL_TOOL_VERS   0x06

typedef struct {
	uint32_t client_id;
	uint32_t size;
} rfsa_get_buff_addr_req_msg_v01;

typedef struct {
	qmi_response_type_v01 resp;
	uint8_t  address_valid;
	uint64_t address;
} rfsa_get_buff_addr_resp_msg_v01;

/* Service-object accessor (matches rmtfs_get_service_object_v01 pattern). */
qmi_idl_service_object_type rfsa_get_service_object_internal_v01(
	int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version);

#define rfsa_get_service_object_v01() \
	rfsa_get_service_object_internal_v01(RFSA_V01_IDL_MAJOR_VERS, \
					    RFSA_V01_IDL_MINOR_VERS, \
					    RFSA_V01_IDL_TOOL_VERS)

#endif /* __REMOTE_FILESYSTEM_ACCESS_V01_H__ */

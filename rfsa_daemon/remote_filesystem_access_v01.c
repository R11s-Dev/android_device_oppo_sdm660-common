// SPDX-License-Identifier: GPL-2.0-only
/*
 * RFSA QMI IDL service object - userspace.
 *
 * Hand-written to mirror the layout of remote_storage_v01.c (the rmtfs
 * service object). RFSA has a single message pair, GET_BUFF_ADDR, whose
 * wire layout is identical to rmtfs alloc_buff ({u32,u32} -> {resp,u8,u64}),
 * so the TLV tables are lifted from rmtfs with only field names changed.
 *
 * service_id = 0x1C. Type table references common_v01 (qmi_response_type_v01).
 */

#include "stdint.h"
#include "qmi_idl_lib_internal.h"
#include "remote_filesystem_access_v01.h"
#include "common_v01.h"

/* ---- message TLV encodings -------------------------------------------- */

/* req: { client_id(u32), size(u32) } */
static const uint8_t rfsa_get_buff_addr_req_msg_data_v01[] = {
  0x01,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(rfsa_get_buff_addr_req_msg_v01, client_id),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(rfsa_get_buff_addr_req_msg_v01, size)
};

/* resp: { resp(aggregate), address_valid(u8 opt), address(u64 opt) } */
static const uint8_t rfsa_get_buff_addr_resp_msg_data_v01[] = {
  0x02,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(rfsa_get_buff_addr_resp_msg_v01, resp),
  QMI_IDL_TYPE88(1, 0),  /* referenced_tables[1] = common, entry[0] = qmi_response_type_v01 */

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL |
   (QMI_IDL_OFFSET8(rfsa_get_buff_addr_resp_msg_v01, address) -
    QMI_IDL_OFFSET8(rfsa_get_buff_addr_resp_msg_v01, address_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(rfsa_get_buff_addr_resp_msg_v01, address)
};

/* ---- type / message tables -------------------------------------------- */

static const qmi_idl_message_table_entry rfsa_message_table_v01[] = {
  {sizeof(rfsa_get_buff_addr_req_msg_v01),  rfsa_get_buff_addr_req_msg_data_v01},
  {sizeof(rfsa_get_buff_addr_resp_msg_v01), rfsa_get_buff_addr_resp_msg_data_v01}
};

/* Forward-declare, then build the referenced-tables array (self + common). */
static const qmi_idl_type_table_object rfsa_qmi_idl_type_table_object_v01;

static const qmi_idl_type_table_object
	*rfsa_qmi_idl_type_table_object_referenced_tables_v01[] = {
	&rfsa_qmi_idl_type_table_object_v01,
	&common_qmi_idl_type_table_object_v01
};

static const qmi_idl_type_table_object rfsa_qmi_idl_type_table_object_v01 = {
  0,  /* n_types */
  sizeof(rfsa_message_table_v01) / sizeof(qmi_idl_message_table_entry),  /* n_messages */
  1,  /* n_referenced_tables */
  NULL,  /* no custom aggregate types */
  rfsa_message_table_v01,
  rfsa_qmi_idl_type_table_object_referenced_tables_v01,
  NULL  /* p_ranges */
};

/* ---- service message tables ------------------------------------------- */

static const qmi_idl_service_message_table_entry rfsa_service_command_messages_v01[] = {
  {QMI_RFSA_GET_BUFF_ADDR_REQ_MSG_V01,  QMI_IDL_TYPE16(0, 0),
   RFSA_GET_BUFF_ADDR_REQ_MSG_MAX_LEN_V01}
};

static const qmi_idl_service_message_table_entry rfsa_service_response_messages_v01[] = {
  {QMI_RFSA_GET_BUFF_ADDR_RESP_MSG_V01, QMI_IDL_TYPE16(0, 1),
   RFSA_GET_BUFF_ADDR_RESP_MSG_MAX_LEN_V01}
};

/* No indications. */

/* ---- service object --------------------------------------------------- */

struct qmi_idl_service_object rfsa_qmi_idl_service_object_v01 = {
  RFSA_V01_IDL_TOOL_VERS,     /* library_version */
  RFSA_V01_IDL_MAJOR_VERS,    /* idl_version */
  RFSA_SERVICE_ID_V01,        /* service_id = 0x1C */
  RFSA_GET_BUFF_ADDR_RESP_MSG_MAX_LEN_V01,  /* max_msg_len */
  {
    sizeof(rfsa_service_command_messages_v01)  / sizeof(qmi_idl_service_message_table_entry),
    sizeof(rfsa_service_response_messages_v01) / sizeof(qmi_idl_service_message_table_entry),
    0  /* indications */
  },
  {
    rfsa_service_command_messages_v01,
    rfsa_service_response_messages_v01,
    NULL
  },
  &rfsa_qmi_idl_type_table_object_v01,
  RFSA_V01_IDL_MINOR_VERS,    /* idl_minor_version */
  NULL                        /* parent_service_obj */
};

/* ---- accessor --------------------------------------------------------- */

qmi_idl_service_object_type rfsa_get_service_object_internal_v01(
	int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version)
{
	if (RFSA_V01_IDL_MAJOR_VERS != idl_maj_version ||
	    RFSA_V01_IDL_MINOR_VERS != idl_min_version ||
	    RFSA_V01_IDL_TOOL_VERS != library_version)
		return NULL;
	return (qmi_idl_service_object_type)&rfsa_qmi_idl_service_object_v01;
}

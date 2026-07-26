# SPDX-License-Identifier: Apache-2.0
#
# rfsa_daemon: userspace RFSA (0x1C) service that hands the rmtfs and
# oembackup shared-memory physical addresses to the modem, replacing the 4.4
# kernel-side sharedmem_qmi.c which cannot be ported to 4.19 (900E).
#
# This tree ships libqmi_csi / libqmi_common_so only as prebuilt .so blobs
# (PRODUCT_COPY_FILES in sdm660-common-vendor.mk) with no headers and no
# build-module definition, so:
#  - qmi-framework headers are vendored under include/
#  - the .so blobs are linked directly via LOCAL_LDFLAGS (they are already
#    copied to /vendor/lib64 at install time by the vendor mk).

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := rfsa_daemon
LOCAL_INIT_RC := vendor.oppo.rfsa.rc
LOCAL_MULTILIB := 64

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

LOCAL_SRC_FILES := remote_filesystem_access_v01.c rfsa_svc.c

LOCAL_SHARED_LIBRARIES := libcutils liblog
LOCAL_HEADER_LIBRARIES := libcutils_headers liblog_headers

# libqmi_csi / libqmi_common_so: prebuilt blobs, linked directly.
# Path matches the PRODUCT_COPY_FILES source in sdm660-common-vendor.mk.
QMI_BLOB_DIR := $(TOP)/vendor/oppo/sdm660-common/proprietary/vendor/lib64
LOCAL_LDFLAGS += $(QMI_BLOB_DIR)/libqmi_csi.so
LOCAL_LDFLAGS += $(QMI_BLOB_DIR)/libqmi_common_so.so

LOCAL_MODULE_TAGS := optional
LOCAL_CLANG := true
LOCAL_CFLAGS := -DLOG_NIDEBUG=0 -Wno-unused-parameter

LOCAL_MODULE_OWNER := oppo
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_EXECUTABLE)

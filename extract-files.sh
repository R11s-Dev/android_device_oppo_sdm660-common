#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
# Copyright (C) 2017-2020 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

set -e

DEVICE=sdm660-common
VENDOR=oppo

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

ANDROID_ROOT="${MY_DIR}/../../.."

HELPER="${ANDROID_ROOT}/tools/extract-utils/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at ${HELPER}"
    exit 1
fi
source "${HELPER}"

# Default to sanitizing the vendor folder before extraction
CLEAN_VENDOR=true

KANG=
SECTION=

while [ "${#}" -gt 0 ]; do
    case "${1}" in
        -n | --no-cleanup )
                CLEAN_VENDOR=false
                ;;
        -k | --kang )
                KANG="--kang"
                ;;
        -s | --section )
                SECTION="${2}"; shift
                CLEAN_VENDOR=false
                ;;
        * )
                SRC="${1}"
                ;;
    esac
    shift
done

if [ -z "${SRC}" ]; then
    SRC="adb"
fi

function blob_fixup() {
    case "${1}" in
        # Oppo IMEI fixup: patch libqmiservices / libril-qc-hal-qmi via SigScan
        vendor/lib64/libqmiservices.so)
            [ "${2}" = "" ] && return 0
            "${SIGSCAN}" -p "0B 00 00 00 23 00 0D 00 00 00 24 00 0F 00 00 00 25 00 11 00 00 00 26 00 15 00 00 00 27 00 17 00 16 00 28 00 19 00" \
                          -P "0B 00 00 00 23 00 0D 00 00 00 24 00 0F 00 00 00 25 00 55 00 04 00 26 00 15 00 00 00 27 00 17 00 16 00 28 00 19 00" \
                          -f "${2}"
            ;;
        vendor/lib64/libril-qc-hal-qmi.so)
            [ "${2}" = "" ] && return 0
            "${SIGSCAN}" -p "06 A6 8E 52 E3 C7 41 B9 E4 BF 40 F9 E5 77 41 B9 42 7F 35 94 E0 EB 01 B9 E0 EB 41 B9" \
                          -P "06 A6 8E 52 E3 C7 41 B9 E4 BF 40 F9 E5 77 41 B9 DC F3 EE 97 E0 EB 01 B9 E0 EB 41 B9" \
                          -f "${2}"
            "${SIGSCAN}" -p "E0 02 80 52 CE 80 46 94 C0 04 00 34 D8 8C 46 94 E0 3B 00 B9 E8 3B 40 B9 1F 01 00 71 E8 B7 9F 1A 08 04 00 37" \
                          -P "E0 02 80 52 CE 80 46 94 C0 04 00 34 D8 8C 46 94 E0 3B 00 B9 E8 3B 40 B9 1F 01 00 71 E8 B7 9F 1A 20 00 00 14" \
                          -f "${2}"
            "${SIGSCAN}" -p "20 BC FF B0 00 E8 17 91 84 80 46 94 E0 17 00 F9 86 80 46 94 E0 13 00 F9 88 80 46 94 E1 03 00 2A 26 7C 40 93 E6 0F 00 F9 73 FF FF 97 E8 3B 40 B9 A9 00 80 52 E0 0B 00 F9 E0 03 09 2A 21 BE FF 90 21 90 2D 91 A2 BF FF F0 42 48 27 91" \
                          -P "FF 03 01 D1 FD 7B 03 A9 FD C3 00 91 E0 13 00 A9 E5 1B 01 A9 39 40 FC 97 08 04 00 11 E8 83 00 39 E0 03 40 F9 A1 04 80 52 E2 83 00 91 23 00 80 52 E4 07 40 F9 E5 13 40 B9 E6 1B 40 B9 CD FF FF 97 FD 7B 43 A9 FF 03 01 91 C0 03 5F D6" \
                          -f "${2}"
            ;;
        # FastRPC: force /dev/adsprpc-smd-secure for all domains (b.ne -> b @0x129D4)
        vendor/lib64/libcdsprpc.so)
            [ "${2}" = "" ] && return 0
            "${SIGSCAN}" -p "FD 7B 02 A9 FD 83 00 91 13 04 00 12 7F 0E 00 71 C1 01 00 54 A9 00 00 B0 68 02 1F 52 29 81 00 91 20 59 68 F8" \
                          -P "FD 7B 02 A9 FD 83 00 91 13 04 00 12 7F 0E 00 71 0E 00 00 14 A9 00 00 B0 68 02 1F 52 29 81 00 91 20 59 68 F8" \
                          -f "${2}"
            ;;
        # Adapt the legacy Oppo camera HAL to the current QTI gralloc handle.
        # 0x34 is layer_count; buffer size and offset are at 0x48 and 0x4c.
        vendor/lib/hw/camera.sdm660.so)
            [ "${2}" = "" ] && return 0
            "${SIGSCAN}" -p "4F F4 51 73 C1 68 D0 E9 0D 20 CD E9 03 20 4B 48" \
                          -P "4F F4 51 73 C1 68 D0 E9 12 20 CD E9 03 20 4B 48" \
                          -f "${2}"
            "${SIGSCAN}" -p "C0 68 CB F8 08 00 31 68 08 9A 00 26 49 6B CB E9 04 21" \
                          -P "C0 68 CB F8 08 00 31 68 08 9A 00 26 89 6C CB E9 04 21" \
                          -f "${2}"
            "${SIGSCAN}" -p "D4 F8 7C 39 01 22 C8 68 49 6B E0 47 C5 F8 80 09 3E E0" \
                          -P "D4 F8 7C 39 01 22 C8 68 89 6C E0 47 C5 F8 80 09 3E E0" \
                          -f "${2}"
            "${SIGSCAN}" -p "DB F8 00 00 CA 68 D1 E9 0D 31 05 91 CD E9 03 03 20 48" \
                          -P "DB F8 00 00 CA 68 D1 E9 12 31 05 91 CD E9 03 03 20 48" \
                          -f "${2}"
            "${SIGSCAN}" -p "30 68 3D 46 C0 68 CA F8 08 00 30 68 06 99 40 6B CA E9 04 10" \
                          -P "30 68 3D 46 C0 68 CA F8 08 00 30 68 06 99 80 6C CA E9 04 10" \
                          -f "${2}"
            "${SIGSCAN}" -p "D4 F8 7C 39 01 22 C8 68 49 6B E0 47 C5 F8 80 09 5A E0" \
                          -P "D4 F8 7C 39 01 22 C8 68 89 6C E0 47 C5 F8 80 09 5A E0" \
                          -f "${2}"
            "${SIGSCAN}" -p "DB F8 00 00 CA 68 D1 E9 0D 31 05 91 CD E9 03 03 21 48" \
                          -P "DB F8 00 00 CA 68 D1 E9 12 31 05 91 CD E9 03 03 21 48" \
                          -f "${2}"
            "${SIGSCAN}" -p "38 68 C0 68 CA F8 08 00 38 68 06 99 40 6B CA E9 04 10 B2 46" \
                          -P "38 68 C0 68 CA F8 08 00 38 68 06 99 80 6C CA E9 04 10 B2 46" \
                          -f "${2}"
            "${SIGSCAN}" -p "CD F8 2C 80 C8 68 49 6B B8 47 DD F8 2C 80 C5 F8 80 09" \
                          -P "CD F8 2C 80 C8 68 89 6C B8 47 DD F8 2C 80 C5 F8 80 09" \
                          -f "${2}"
            "${SIGSCAN}" -p "DF F8 B0 76 E0 68 CA 68 7F 44 D1 E9 0D 31 CD E9 00 78" \
                          -P "DF F8 B0 76 E0 68 CA 68 7F 44 D1 E9 12 31 CD E9 00 78" \
                          -f "${2}"
            "${SIGSCAN}" -p "D6 F8 58 08 10 99 40 6B C4 E9 04 10 01 20" \
                          -P "D6 F8 58 08 10 99 80 6C C4 E9 04 10 01 20" \
                          -f "${2}"
            ;;
        # Fix camera hal to load config from /vendor/etc/camera instead of /system/etc/camera
        vendor/lib/libmmcamera_interface.so)
            sed -i 's|/system/etc/camera|/vendor/etc/camera|g' "${2}"
            ;;
    esac
}

# Initialize the helper
setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false "${CLEAN_VENDOR}"

extract "${MY_DIR}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"

"${MY_DIR}/setup-makefiles.sh"

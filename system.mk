#
# Copyright (C) 2025 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Bluetooth
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    bt.max.hfpclient.connections=1 \
    ro.bluetooth.emb_wp_mode=false \
    ro.bluetooth.wipower=false

# Camera
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    persist.camera.aifb=1 \
    ro.camera.dualcam.type=2 \
    ro.camera.filter.version=1 \
    ro.camera.temperature.limit=480

# RIL
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    DEVICE_PROVISIONED=1 \
    gsm.lte.ca.support=1 \
    ril.subscription.types=NV,RUIM

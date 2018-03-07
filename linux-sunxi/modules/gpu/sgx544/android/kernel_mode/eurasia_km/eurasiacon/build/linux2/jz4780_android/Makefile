# Copyright	(C) Imagination Technologies Limited. All rights reserved.
#
# No part of this software, either material or conceptual may be
# copied or distributed, transmitted, transcribed, stored in a
# retrieval system or translated into any human or computer
# language in any form by any means, electronic, mechanical,
# manual or other-wise, or disclosed to third parties without
# the express written permission of: Imagination Technologies
# Limited, HomePark Industrial Estate, Kings Langley,
# Hertfordshire, WD4 8LZ, UK
#
# $Log: $
#

TARGET_DEVICE ?= ci20

override SGXCORE := 540
override SGX_CORE_REV := 130
override SUPPORT_SLC := 1

SGX_DYNAMIC_TIMING_INFO ?= 1
SGX_FAST_DPM_INIT ?= 1

override SUPPORT_LINUX_USING_WORKQUEUES := 1

PVR_SYSTEM := sgx_jz4780
HAL_VARIANT := xb4780

# Force-disable camerahal on this board
override EXCLUDED_APIS := $(EXCLUDED_APIS) camerahal

include ../common/android/paths.mk
include ../common/android/mips.mk
include ../common/android/features.mk

# Switch on USSE EDM status debug by default
PVRSRV_USSE_EDM_STATUS_DEBUG ?= 1
PVRSRV_DUMP_MK_TRACE ?= 1

# Pre-rotate surfaces in non-screen orientations
PVR_ANDROID_USE_WINDOW_TRANSFORM_HINT ?= 1

# Disable legacy displayclass support, we will use DRM
override SUPPORT_PVRSRV_DEVICE_CLASS := 0
override PVR_LDM_DEVICE_TREE := 1
override SUPPORT_DMABUF := 1

PVR_ANDROID_COMPOSERHAL ?= drm

include ../config/core.mk
include ../common/android/extra_config.mk

# Until dri/drm is officially supported, hide the setup in this file
$(eval $(call BothConfigC,SUPPORT_DRI_DRM,))
$(eval $(call BothConfigMake,SUPPORT_DRI_DRM,1))
$(eval $(call KernelConfigC,PVR_DRI_DRM_PLATFORM_DEV,))
$(eval $(call UserConfigC,PVR_DRM_MODESET_DRIVER_NAME,jz4780))
$(eval $(call UserConfigC,PVR_DRM_MODESET_MODULE_NAME,dumb))
$(eval $(call UserConfigMake,PVR_DRM_MODESET_MODULE_NAME,dumb))

# Compile in interception code for USAGE_FB buffers so they use CMA/DMA
$(eval $(call UserConfigMake,SUPPORT_ANDROID_JZ4780,1))

#
# Copyright (C) 2018 GoldenRiver Technology Co., Ltd.
# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

ifeq ($(LOCAL_PATH),)
  $(info Using NDK_HOME: $(NDK_HOME))
  $(info Using NDK_OUT: $(NDK_OUT))
  $(info Using NDK_PROJ_ROOT: $(NDK_PROJ_ROOT))
  LOCAL_PATH = $(NDK_PROJ_ROOT)
endif

NEBULA := true

# Auto modified by export_ree_trusty script.
NEBULA_VERSION_AUTO :=
NEBULA_DISABLE_UREE_AUTO :=

ifeq ($(NEBULA_DISABLE_UREE_AUTO), y)
NEBULA_CONFIG_UREE_SUPPORT := 0
else
NEBULA_CONFIG_UREE_SUPPORT := 1
endif


ifeq ($(NEBULA_VERSION_AUTO),)
NEBULA_VERSION_STR := $(shell git -C $(LOCAL_PATH) describe --always --tags)
else
NEBULA_VERSION_STR := $(NEBULA_VERSION_AUTO)
endif

$(info NEBULA_VERSION_AUTO: $(NEBULA_VERSION_AUTO))
$(info NEBULA_DISABLE_UREE_AUTO: $(NEBULA_DISABLE_UREE_AUTO))
$(info NEBULA_CONFIG_UREE_SUPPORT: $(NEBULA_CONFIG_UREE_SUPPORT))
$(info NEBULA_VERSION_STR: $(NEBULA_VERSION_STR))

ifneq ($(NDK_PROJ_ROOT),)
  ### Nebula ndk build

ifeq ($(V),1)
NOECHO :=
else
NOECHO ?= @
endif

SUBDIRS := $(wildcard $(LOCAL_PATH)/*/.)
all: $(SUBDIRS)
$(SUBDIRS):
	$(NOECHO)if [ -f $@/Android.mk ]; then \
	$(NDK_HOME)/ndk-build -j $(JOBS) V=$(V) \
		NDK_PROJECT_PATH=$@ \
		APP_BUILD_SCRIPT=$@/Android.mk \
		NDK_APPLICATION_MK=$(strip $(if $(wildcard $@/Application.mk),\
				$@/Application.mk,$(LOCAL_PATH)/Application.mk)) \
		NDK_LIBS_OUT=$(NDK_OUT)/$(shell basename $(dir $@))/libs \
		NDK_OUT=$(NDK_OUT)/$(shell basename $(dir $@))/obj \
		NDK_MODULE_PATH=$(LOCAL_PATH) \
		NEBULA=$(NEBULA) \
		NEBULA_CONFIG_UREE_SUPPORT=$(NEBULA_CONFIG_UREE_SUPPORT) \
		NEBULA_VERSION_STR=$(NEBULA_VERSION_STR); \
	fi

.PHONY: all $(SUBDIRS)

else
  ### AOSP build
  include $(call all-makefiles-under,$(LOCAL_PATH))
endif


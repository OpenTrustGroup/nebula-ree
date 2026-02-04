#!/bin/bash
# Copyright 2018 GoldenRiver Technology Co., Ltd. All rights reserved.
# Copyright 2017 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

### build third_party/ree-trusty by Android NDK

# user set vars.
ANDROID_NDK_HOME="/$HOME/works/fuchsia_workspace/nebula/prebuilts/android-ndk-r20"
REE_TRUSTY_OUT="out"

JOBS=`getconf _NPROCESSORS_ONLN` || {
  Cannot get number of processors
  exit 1
}
JOBS=1

set -eo pipefail; [[ "${TRACE}" ]] && set -x

echo "########################################################################"
echo "Building ree-trusty by NDK..."
echo "########################################################################"

readonly REE_TRUSTY_PATH="ree-trusty"
readonly NDK_PROJECT_PATH_="${REE_TRUSTY_PATH}"
readonly APP_BUILD_SCRIPT_="${NDK_PROJECT_PATH_}/Android.mk"
readonly NDK_APPLICATION_MK_="${NDK_PROJECT_PATH_}/Application.mk"
readonly NDK_OUT_LIBS="${REE_TRUSTY_OUT}/libs"
readonly NDK_OUT_OBJ_="${REE_TRUSTY_OUT}/obj"

${ANDROID_NDK_HOME}/ndk-build -j ${JOBS} \
    NDK_PROJECT_PATH=${NDK_PROJECT_PATH_} \
    APP_BUILD_SCRIPT=${APP_BUILD_SCRIPT_} \
    NDK_APPLICATION_MK=${NDK_APPLICATION_MK_} \
    NDK_LIBS_OUT=${NDK_OUT_LIBS} \
    NDK_OUT=${NDK_OUT_OBJ_}

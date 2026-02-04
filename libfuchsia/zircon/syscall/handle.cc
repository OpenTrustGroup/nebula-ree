// Copyright (c) 2021 GoldenRiver Technology Co., Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <zircon/syscalls.h>
#include <zircon/types.h>

#include <object/common.h>
#include <object/handle.h>

#include "lib/nebula/base/auto_lock.h"

zx_status_t zx_handle_close(zx_handle_t handle_value) {
  nebula::base::AutoLock lock(GetHandleTableLock());

  Handle* handle = Handle::GetHandleLocked(handle_value);
  if (!handle)
    return ZX_ERR_BAD_HANDLE;

  delete handle;

  return ZX_OK;
}
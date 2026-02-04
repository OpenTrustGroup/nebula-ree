// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <lib/zx/vmar.h>
#include <zircon/syscalls.h>

namespace zx {

zx_status_t vmar::allocate(size_t offset, size_t size, uint32_t flags, vmar* child,
                           uintptr_t* child_addr) const {
  return ZX_ERR_NOT_SUPPORTED;
}

}  // namespace zx

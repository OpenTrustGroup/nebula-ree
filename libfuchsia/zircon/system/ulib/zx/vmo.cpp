// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <lib/zx/vmo.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <zircon/syscalls.h>

#include <utility>

namespace zx {

namespace {
// Workaround for outdated system headers and runtimes.
static int sys_memfd_create(const char* __name, unsigned int __flags) {
  return syscall(__NR_memfd_create, __name, __flags);
}
}  // namespace

zx_status_t vmo::create(uint64_t size, uint32_t options, vmo* result) {
  vmo vmo(sys_memfd_create("VMO", options));
  if (!vmo.is_valid()) {
    fprintf(stderr, "memfd_create error: %s\n", strerror(errno));
    return ZX_ERR_IO;
  }

  auto status = vmo.set_size(size);
  if (status != ZX_OK)
    return status;

  *result = std::move(vmo);
  return ZX_OK;
}

}  // namespace zx

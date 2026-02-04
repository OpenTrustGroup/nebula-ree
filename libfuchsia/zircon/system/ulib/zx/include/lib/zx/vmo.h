// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <lib/zx/handle.h>
#include <lib/zx/object.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <utility>

namespace zx {

class vmo : public object<vmo> {
 public:
  static constexpr zx_obj_type_t TYPE = ZX_OBJ_TYPE_VMO;

  constexpr vmo() = default;

  explicit vmo(zx_handle_t value) : object(value) {}

  explicit vmo(handle&& h) : object(h.release()) {}

  vmo(vmo&& other) : object(other.release()) {}

  vmo& operator=(vmo&& other) {
    reset(other.release());
    return *this;
  }

  static zx_status_t create(uint64_t size, uint32_t options, vmo* result);

  zx_status_t read(void* data, uint64_t offset, size_t len) const {
    if (!is_valid_range(offset, len))
      return ZX_ERR_OUT_OF_RANGE;

    ssize_t res = lseek(get(), offset, SEEK_SET);
    if (res < 0) {
      fprintf(stderr, "lseek error: %s\n", strerror(errno));
      return ZX_ERR_IO;
    }

    res = ::read(get(), data, len);
    if (res < 0) {
      fprintf(stderr, "read error: %s\n", strerror(errno));
      return ZX_ERR_IO;
    }

    return ZX_OK;
  }

  zx_status_t write(const void* data, uint64_t offset, size_t len) const {
    if (!is_valid_range(offset, len))
      return ZX_ERR_OUT_OF_RANGE;

    auto res = lseek(get(), offset, SEEK_SET);
    if (res < 0) {
      fprintf(stderr, "lseek failed: %s\n", strerror(errno));
      return ZX_ERR_IO;
    }

    res = ::write(get(), data, len);
    if (res < 0) {
      fprintf(stderr, "write failed: %s\n", strerror(errno));
      return ZX_ERR_IO;
    }
    return ZX_OK;
  }

  zx_status_t get_size(uint64_t* size) const {
    struct stat st;
    int r;
    r = ::fstat(get(), &st);
    if (r < 0) {
      fprintf(stderr, "fstat failed: %s\n", strerror(errno));
      return ZX_ERR_IO;
    }
    *size = st.st_size;
    return ZX_OK;
  };

  zx_status_t set_size(uint64_t size) const {
    if (ftruncate(get(), size) < 0) {
      fprintf(stderr, "ftruncate failed: %s\n", strerror(errno));
      return ZX_ERR_IO;
    }
    return ZX_OK;
  }

  zx_status_t clone(uint32_t options, uint64_t offset, uint64_t size, vmo* result) const {
    return ZX_ERR_NOT_SUPPORTED;
  }

  zx_status_t op_range(uint32_t op, uint64_t offset, uint64_t size, void* buffer,
                       size_t buffer_size) const {
    return ZX_ERR_NOT_SUPPORTED;
  }

  zx_status_t set_cache_policy(uint32_t cache_policy) { return ZX_ERR_NOT_SUPPORTED; }

 private:
  bool is_valid_range(uint64_t offset, size_t len) const {
    uint64_t size = 0;
    auto status = get_size(&size);
    if (status != ZX_OK)
      return false;

    return offset + len >= offset && offset + len <= size;
  }
};

using unowned_vmo = const unowned<vmo>;

}  // namespace zx

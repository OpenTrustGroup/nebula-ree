// Copyright 2021 GoldenRiver Technology
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <lib/zx/handle.h>
#include <lib/zx/object.h>
#include <lib/zx/tipc_chan.h>
#include <lib/zx/unix_sock_chan.h>

namespace zx {

class unix_sock : public object<unix_sock> {
 public:
  static constexpr zx_obj_type_t TYPE = ZX_OBJ_TYPE_UNIX_SOCK;

  constexpr unix_sock() = default;

  explicit unix_sock(zx_handle_t value) : object(value) {}

  explicit unix_sock(handle&& h) : object(h.release()) {}

  unix_sock(unix_sock&& other) : object(other.release()) {}

  unix_sock& operator=(unix_sock&& other) {
    reset(other.release());
    return *this;
  }

  static zx_status_t create(const char* path, unix_sock* out_port);

  zx_status_t accept(unix_sock_chan* out);
};

}  // namespace zx
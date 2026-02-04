// Copyright 2021 GoldenRiver Technology
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <lib/zx/handle.h>
#include <lib/zx/object.h>

namespace zx {

class unix_sock_chan : public object<unix_sock_chan> {
 public:
  static constexpr zx_obj_type_t TYPE = ZX_OBJ_TYPE_UNIX_SOCK_CHAN;

  constexpr unix_sock_chan() = default;

  explicit unix_sock_chan(zx_handle_t value) : object(value) {}

  explicit unix_sock_chan(handle&& h) : object(h.release()) {}

  unix_sock_chan(unix_sock_chan&& other) : object(other.release()) {}

  unix_sock_chan& operator=(unix_sock_chan&& other) {
    reset(other.release());
    return *this;
  }

  static zx_status_t connect(const char* path, unix_sock_chan* out_chann);
  static zx_status_t create(int fd, unix_sock_chan* out_chan);

  zx_status_t read(char* buf, int buf_len, int* fds, int max_fds, uint32_t* read_len,
                   uint32_t* fd_num);
  zx_status_t read(char* buf, int buf_len, uint32_t* read_len);
  zx_status_t write(const char* buf, int buf_len, int* fds, int fd_num, uint32_t* written_len);
  zx_status_t write(const char* buf, int buf_len, uint32_t* written_len);
};

}  // namespace zx
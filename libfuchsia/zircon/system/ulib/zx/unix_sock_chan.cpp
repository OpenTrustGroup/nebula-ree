// Copyright 2021 GoldenRiver Technology
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <lib/zx/unix_sock_chan.h>
#include <zircon/syscalls.h>

namespace zx {

zx_status_t unix_sock_chan::connect(const char* path, unix_sock_chan* out_port) {
  zx_handle_t h = ZX_HANDLE_INVALID;
  zx_status_t result = zx_unix_sock_connect(path, &h);
  out_port->reset(h);
  return result;
}

zx_status_t unix_sock_chan::read(char* buf, int buf_len, int* fds, int max_fds, uint32_t* read_len,
                                 uint32_t* fd_num) {
  return zx_unix_sock_read(get(), buf, buf_len, fds, max_fds, read_len, fd_num);
}

zx_status_t unix_sock_chan::read(char* buf, int buf_len, uint32_t* read_len) {
  return zx_unix_sock_read(get(), buf, buf_len, nullptr, 0, read_len, nullptr);
}

zx_status_t unix_sock_chan::write(const char* buf, int buf_len, uint32_t* written_len) {
  return zx_unix_sock_write(get(), buf, buf_len, nullptr, 0, written_len);
}

zx_status_t unix_sock_chan::write(const char* buf, int buf_len, int* fds, int fd_num,
                                  uint32_t* written_len) {
  return zx_unix_sock_write(get(), buf, buf_len, fds, fd_num, written_len);
}

}  // namespace zx
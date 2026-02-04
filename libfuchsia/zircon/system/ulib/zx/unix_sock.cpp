// Copyright 2021 GoldenRiver Technology
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <lib/zx/unix_sock.h>
#include <lib/zx/unix_sock_chan.h>
#include <zircon/syscalls.h>

namespace zx {

zx_status_t unix_sock::create(const char* path, unix_sock* out_port) {
  zx_handle_t h = ZX_HANDLE_INVALID;
  zx_status_t result = zx_unix_sock_create(path, &h);

  out_port->reset(h);
  return result;
}

zx_status_t unix_sock::accept(unix_sock_chan* out_chan) {
  zx_handle_t h = ZX_HANDLE_INVALID;
  zx_status_t result = zx_unix_sock_accept(get(), &h);
  out_chan->reset(h);
  return result;
};

}  // namespace zx
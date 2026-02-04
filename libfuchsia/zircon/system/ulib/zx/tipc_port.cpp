// Copyright 2021 GoldenRiver Technology
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <lib/zx/tipc_chan.h>
#include <lib/zx/tipc_port.h>

#include <zircon/syscalls.h>

namespace zx {

zx_status_t tipc_port::create(uint32_t flags, const char* name, size_t namelen, tipc_port* out_port) {
    zx_handle_t h = ZX_HANDLE_INVALID;
    zx_status_t result = zx_tipc_port_create(name, namelen, flags, &h);

    out_port->reset(h);
    return result;
}

zx_status_t tipc_port::accept(uuid_t* uuid, tipc_chan* out_chan) {
    zx_handle_t h = ZX_HANDLE_INVALID;
    zx_status_t result = zx_tipc_port_accept(get(), uuid, sizeof(*uuid), &h);
    out_chan->reset(h);
    return result;
};

} // namespace zx

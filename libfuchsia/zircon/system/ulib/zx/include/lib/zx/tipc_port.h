// Copyright 2021 GoldenRiver Technology
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <lib/zx/handle.h>
#include <lib/zx/object.h>
#include <lib/zx/tipc_chan.h>
#include <trusty_uuid.h>

namespace zx {

class tipc_port : public object<tipc_port> {
public:
    static constexpr zx_obj_type_t TYPE = ZX_OBJ_TYPE_TIPC_PORT;

    constexpr tipc_port() = default;

    explicit tipc_port(zx_handle_t value)
        : object(value) {}

    explicit tipc_port(handle&& h)
        : object(h.release()) {}

    tipc_port(tipc_port&& other)
        : object(other.release()) {}

    tipc_port& operator=(tipc_port&& other) {
        reset(other.release());
        return *this;
    }

    static zx_status_t create(uint32_t flags, const char* name, size_t namelen, tipc_port* out_port);

    zx_status_t accept(uuid_t* uuid, tipc_chan* out);
};

} // namespace zx
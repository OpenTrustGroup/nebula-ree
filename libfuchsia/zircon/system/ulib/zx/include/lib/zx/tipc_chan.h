// Copyright 2021 GoldenRiver Technology
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <lib/zx/object.h>
#include <lib/zx/handle.h>

namespace zx {

class tipc_chan : public object<tipc_chan> {
public:
    static constexpr zx_obj_type_t TYPE = ZX_OBJ_TYPE_TIPC_CHAN;

    constexpr tipc_chan() = default;

    explicit tipc_chan(zx_handle_t value)
        : object(value) {}

    explicit tipc_chan(handle&& h)
        : object(h.release()) {}

    tipc_chan(tipc_chan&& other)
        : object(other.release()) {}

    tipc_chan& operator=(tipc_chan&& other) {
        reset(other.release());
        return *this;
    }

    static zx_status_t create(uint32_t flags, const char* name, size_t namelen, tipc_chan* out_chan);

    zx_status_t read(void* bytes, uint32_t num_bytes, uint32_t* actual_bytes = nullptr) const {
        return zx_tipc_chan_read(get(), 0u, bytes, nullptr, num_bytes, 0u,
                                 actual_bytes, nullptr);
    }

    zx_status_t write(const void* bytes, uint32_t num_bytes) const {
        return zx_tipc_chan_write(get(), 0u, bytes, num_bytes, nullptr, 0u);
    }
};

} // namespace zx
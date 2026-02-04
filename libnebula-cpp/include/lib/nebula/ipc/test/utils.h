// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <lib/async-loop/cpp/loop.h>
#include <lib/zx/handle.h>
#include <lib/zx/tipc_chan.h>
#include <lib/zx/tipc_port.h>

#if !defined(__DEFINED_struct_iovec)
#define __DEFINED_struct_iovec
#include <trusty_ipc.h>
#undef __DEFINED_struct_iovec
#endif

#include <string>

namespace nebula {
namespace ipc {
namespace test {

static constexpr char kPortName[] = "__nebula_ipc_test_temporarily__";

static inline zx::tipc_port CreateLocalPort(const std::string& name) {
  const uint32_t kFlags = IPC_PORT_ALLOW_TA_CONNECT;
  zx::tipc_port port;
  zx::tipc_port::create(kFlags, name.c_str(), name.length(), &port);
  return port;
}

static inline zx::tipc_chan CreateLocalChan(const std::string& name) {
#ifdef __Fuchsia__
  const uint32_t kFlags = IPC_CONNECT_ASYNC;
#else
  const uint32_t kFlags = IPC_CONNECT_UNIX_SOCK;
#endif
  zx::tipc_chan chan;
  zx::tipc_chan::create(kFlags, name.c_str(), name.length(), &chan);
  return chan;
}

static inline zx_status_t CreateLocalTipcChanPair(zx::tipc_chan* out_ch0, zx::tipc_chan* out_ch1, const std::string port_name = kPortName) {
  zx::tipc_port port = CreateLocalPort(port_name);
  if (!port.is_valid())
    return ZX_ERR_IO;

  zx::tipc_chan ch0 = CreateLocalChan(port_name);
  if (!ch0.is_valid())
    return ZX_ERR_IO;

  uuid_t uuid;
  zx::tipc_chan ch1;
  zx_status_t status = port.accept(&uuid, &ch1);
  if (status != ZX_OK)
    return status;

  *out_ch0 = std::move(ch0);
  *out_ch1 = std::move(ch1);

  return ZX_OK;
}

}  // namespace test
}  // namespace ipc
}  // namespace nebula
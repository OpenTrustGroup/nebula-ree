// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <lib/async/wait.h>
#include <lib/zx/tipc_port.h>

#include <functional>

#include "lib/nebula/ipc/port_message_handler.h"

namespace nebula {
namespace ipc {

class PortMessageReader final : async_wait_t {
 public:
  explicit PortMessageReader(PortMessageHandler* message_handler = nullptr);
  ~PortMessageReader();

  PortMessageReader(const PortMessageReader&) = delete;
  PortMessageReader& operator=(const PortMessageReader&) = delete;

  zx_status_t Bind(zx::tipc_port port, async_t* async = nullptr);
  zx::tipc_port Unbind();

  bool is_bound() const { return port_.is_valid(); }

  const zx::tipc_port& port() const { return port_; }

  void set_message_handler(PortMessageHandler* message_handler) {
    message_handler_ = message_handler;
  }

  void set_error_handler(std::function<void()> error_handler) { error_handler_ = error_handler; }

 private:
  static async_wait_result_t CallHandler(async_t* async, async_wait_t* wait, zx_status_t status,
                                         const zx_packet_signal_t* signal);
  async_wait_result_t OnHandleReady(async_t* async, zx_status_t status,
                                    const zx_packet_signal_t* signal);
  zx_status_t AcceptAndDispatchMessage();
  void NotifyError();

  zx::tipc_port port_;
  async_t* async_;
  PortMessageHandler* message_handler_;
  std::function<void()> error_handler_;
};

}  // namespace ipc
}  // namespace nebula
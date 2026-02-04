// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include "lib/async/cpp/loop.h"
#include "lib/nebula/base/macros.h"
#include "lib/nebula/ipc/chan_message_handler.h"
#include "lib/nebula/ipc/chan_message_reader.h"
#include "lib/nebula/rpc/rpc_completion_queue.h"
#include "lib/nebula/rpc/rpc_message_handler.h"

namespace nebula {
namespace rpc {

using nebula::ipc::ChanMessageHandler;
using nebula::ipc::ChanMessageReader;

class RpcMessageReader : public ChanMessageHandler, public RpcMessageHandler {
 public:
  RpcMessageReader(RpcMessageHandler* message_handler = nullptr);
  virtual ~RpcMessageReader() = default;

  zx_status_t Bind(zx::tipc_chan chan, async_t* async = nullptr);

  void set_message_handler(RpcMessageHandler* message_handler) {
    message_handler_ = message_handler;
  }

  virtual bool is_valid_message(const std::string& raw_bytes) = 0;

  zx_status_t OnRawMessage(std::string message) override;

  void OnRpcMessage(RpcMessage message) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(RpcMessageReader);

  async_t* async_;
  ChanMessageReader message_reader_;
  RpcCompletionQueue completion_queue_;
  RpcMessageHandler* message_handler_;
};

class RpcRequestReader : public RpcMessageReader {
 public:
  RpcRequestReader(RpcMessageHandler* message_handler = nullptr)
      : RpcMessageReader(message_handler) {}
  bool is_valid_message(const std::string& message) final {
    return message.size() >= sizeof(rpc_message_header_t) &&
           (!(((rpc_message_header_t*)&message[0])->flags & RPC_FLAG_RESP_BIT) ||
            ((((rpc_message_header_t*)&message[0])->flags & RPC_FLAG_ACK_BIT) &&
             (((rpc_message_header_t*)&message[0])->flags & RPC_FLAG_STOP_BIT)));
  }
};

class RpcResponseReader : public RpcMessageReader {
 public:
  RpcResponseReader(RpcMessageHandler* message_handler = nullptr)
      : RpcMessageReader(message_handler) {}
  bool is_valid_message(const std::string& message) final {
    return message.size() >= sizeof(rpc_message_header_t) &&
           ((((rpc_message_header_t*)&message[0])->flags & RPC_FLAG_RESP_BIT) ||
            ((((rpc_message_header_t*)&message[0])->flags & RPC_FLAG_ACK_BIT) &&
             (((rpc_message_header_t*)&message[0])->flags & RPC_FLAG_STOP_BIT)));
  }
};

}  // namespace rpc
}  // namespace nebula

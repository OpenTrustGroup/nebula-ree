// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <lib/zx/tipc_chan.h>
#include <lib/zx/tipc_port.h>
#include <trusty_ipc.h>

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "lib/async-loop/cpp/loop.h"
#include "lib/nebula/base/auto_lock.h"
#include "lib/nebula/base/macros.h"
#include "lib/nebula/base/mutex.h"
#include "lib/nebula/base/status.h"
#include "lib/nebula/base/thread_annotations.h"
#include "lib/nebula/rpc/rpc_message_handler.h"
#include "lib/nebula/rpc/rpc_message_reader.h"
#include "lib/nebula/rpc/rpc_message_writer.h"

namespace nebula {
namespace rpc {

using nebula::base::AutoLock;
using nebula::base::Mutex;
using nebula::base::Status;
using nebula::rpc::RpcMessage;
using nebula::rpc::RpcMessageHandler;
using nebula::rpc::RpcRequestReader;
using nebula::rpc::RpcResponseWriter;

class RpcServer : public RpcMessageHandler {
 public:
  RpcServer();

  // The argument is |call_result|, which is RPC status or/and data.
  using RpcCallResponder = std::function<void(std::string)>;

  // The first argument is |call_argument|, which is RPC input data.
  // The second argument is callback for replying |call_result| over RPC.
  using RpcCallListener = std::function<void(std::string, RpcCallResponder)>;

  // The argument is |call_argument|, which is RPC input data.
  using RpcSendListener = std::function<void(std::string)>;

  Status RegisterCallListener(uint32_t ordinal, RpcCallListener callback);
  Status RegisterSendListener(uint32_t ordinal, RpcSendListener callback);

  void UnregisterCallListener(uint32_t ordinal);
  void UnregisterSendListener(uint32_t ordinal);

  void OnRpcMessage(RpcMessage message) override;

  void set_request_reader(RpcRequestReader* reader) { rpc_reader_ = reader; };
  void set_response_writer(RpcResponseWriter* writer) { rpc_writer_ = writer; }

  using CallFallbackHandler = std::function<void(RpcMessage, RpcCallResponder)>;
  void set_call_fallback_handler(CallFallbackHandler handler) {
    call_fallback_handler_ = std::move(handler);
  }

  using SendFallbackHandler = std::function<void(RpcMessage)>;
  void set_send_fallback_handler(SendFallbackHandler handler) {
    send_fallback_handler_ = std::move(handler);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(RpcServer);

  Status Reply(zx_txid_t txid, uint32_t ordinal, const std::string& payload);
  RpcCallListener FindCallback(uint32_t ordinal);

  void OnRpcCall(RpcMessage message, RpcCallResponder responder);
  void OnRpcSend(RpcMessage message);

  // Try to activate non-zero txid, return true only if it can be activated,
  // otherwise, return false if it's been activated already.
  bool TryActivateTxid(uint32_t txid);

  void DeactivateTxid(uint32_t txid);

  RpcRequestReader* rpc_reader_;
  RpcResponseWriter* rpc_writer_;

  Mutex mu_;
  std::unordered_map<uint32_t, RpcCallListener> call_providers_ GUARDED_BY(mu_);
  std::unordered_map<uint32_t, RpcSendListener> send_providers_ GUARDED_BY(mu_);

  CallFallbackHandler call_fallback_handler_;
  SendFallbackHandler send_fallback_handler_;

  std::unordered_set<uint32_t> activated_txid_ GUARDED_BY(mu_);
};

}  // namespace rpc
}  // namespace nebula
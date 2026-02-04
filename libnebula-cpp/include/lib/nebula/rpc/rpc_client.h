// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <zircon/types.h>

#include <atomic>
#include <functional>
#include <string>

#include "lib/nebula/base/macros.h"
#include "lib/nebula/base/mutex.h"
#include "lib/nebula/rpc/rpc_message_handler.h"
#include "lib/nebula/rpc/rpc_message_reader.h"
#include "lib/nebula/rpc/rpc_message_writer.h"

namespace nebula {
namespace rpc {

using nebula::base::AutoLock;
using nebula::base::Mutex;

class RpcClient : public RpcMessageHandler {
 public:
  RpcClient();

  // The first argument is |call_status|, which is IPC call status.
  // The second argument is |call_result|, which is RPC status or/and data.
  using CallResponder = std::function<void(zx_status_t, std::string)>;

  // The first argument is |send_status|, which is IPC call status.
  // using SendResponder = std::function<void(zx_status_t)>;

  void Call(uint32_t ordinal, const std::string& bytes_in, CallResponder);
  zx_status_t Send(uint32_t ordinal, const std::string& bytes_in);

  void OnRpcMessage(RpcMessage message) override;

  void set_response_reader(RpcResponseReader* reader) { reader_ = reader; }
  void set_request_writer(RpcRequestWriter* writer) { writer_ = writer; }

 private:
  DISALLOW_COPY_AND_ASSIGN(RpcClient);

  zx_txid_t GetNextTxid();
  void Queue(zx_txid_t txid, CallResponder responder);
  CallResponder Dequeue(zx_txid_t txid);

  RpcResponseReader* reader_;
  RpcRequestWriter* writer_;

  Mutex mu_;
  std::unordered_map<uint32_t, CallResponder> responder_map_ GUARDED_BY(mu_);
  std::atomic<zx_txid_t> next_txid_;
};

}  // namespace rpc
}  // namespace nebula
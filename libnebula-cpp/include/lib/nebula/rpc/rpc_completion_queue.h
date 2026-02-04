// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <lib/async/loop.h>
#include <lib/async/receiver.h>

#include <list>
#include <mutex>
#include <unordered_map>

#include "lib/nebula/base/macros.h"
#include "lib/nebula/base/thread_annotations.h"
#include "lib/nebula/base/auto_lock.h"
#include "lib/nebula/base/mutex.h"
#include "lib/nebula/rpc/message.h"
#include "lib/nebula/rpc/rpc_message_handler.h"

namespace nebula {
namespace rpc {

class RpcCompletionQueue final : async_receiver_t {
 public:
  RpcCompletionQueue(RpcMessageHandler* handler = nullptr);

  void Bind(async_t* async);
  void Queue(RpcMessage message);

  void set_message_handler(RpcMessageHandler* handler) { handler_ = handler; }

 private:
  struct MessagePackets {
    std::list<RpcMessage> messages;
    size_t data_size;
    bool empty() const { return messages.empty(); }
  };

  static void CallHandler(async_t* async, async_receiver_t* receiver, zx_status_t status,
                          const zx_packet_user_t* data);

  RpcMessage* BuildCompleteMessage(const MessagePackets& packets);

  async_t* async_;
  RpcMessageHandler* handler_;
  nebula::base::Mutex mu_;
  std::unordered_map<zx_txid_t, MessagePackets> message_queue_ GUARDED_BY(mu_);
};

}  // namespace rpc
}  // namespace nebula
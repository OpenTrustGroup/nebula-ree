// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <lib/async/cpp/receiver.h>
#include <lib/async/wait.h>
#include <lib/zx/tipc_chan.h>

#include <deque>
#include <functional>
#include <mutex>
#include <string>

#include "lib/nebula/ipc/chan_unblock_waiter.h"

namespace nebula {
namespace ipc {

class ChanMessageWriter final : async_receiver_t {
 public:
  ChanMessageWriter();
  ~ChanMessageWriter();

  ChanMessageWriter(const ChanMessageWriter&) = delete;
  ChanMessageWriter& operator=(const ChanMessageWriter&) = delete;

  zx_status_t Bind(zx::tipc_chan chan, async_t* async = nullptr);
  zx::tipc_chan Unbind();

  bool is_bound() const { return chan_.is_valid(); }

  const zx::tipc_chan& chan() const { return chan_; }

  // Queue the |message| into queue and returned immediately.
  // The actual write happened when async loop runs.
  zx_status_t WriteLater(std::string message);

  bool is_pending() { return message_queue_.size(); }

 private:
  static void CallHandler(async_t* async, async_receiver_t* receiver, zx_status_t status,
                          const zx_packet_user_t* data);
  void OnHandleReady(async_t* async, zx_status_t status, const zx_packet_user_t* data);

  zx_status_t Queue(async_t* async, const zx_packet_user_t* data);

  ChanUnblockWaiter waiter_;

  zx::tipc_chan chan_;
  async_t* async_;

  std::mutex mu_;
  std::deque<std::string> message_queue_;
};

}  // namespace ipc
}  // namespace nebula
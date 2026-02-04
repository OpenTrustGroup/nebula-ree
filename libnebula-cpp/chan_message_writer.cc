// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include "lib/nebula/ipc/chan_message_writer.h"

#include <lib/async/default.h>
#include <trusty_ipc.h>

#include <type_traits>

#include "lib/nebula/base/logging.h"

namespace nebula {
namespace ipc {

ChanMessageWriter::ChanMessageWriter()
    : async_receiver_t{{ASYNC_STATE_INIT}, &ChanMessageWriter::CallHandler, 0u, {}},
      waiter_([this] { Queue(async_, nullptr); }),
      async_(nullptr) {}

ChanMessageWriter::~ChanMessageWriter() = default;

zx_status_t ChanMessageWriter::Bind(zx::tipc_chan chan, async_t* async) {
  if (is_bound())
    Unbind();
  if (!chan)
    return ZX_OK;
  chan_ = std::move(chan);
  if (async) {
    async_ = async;
  } else {
    async_ = async_get_default();
  }
  zx::tipc_chan borrowed(chan_.get());
  zx_status_t status = waiter_.Bind(std::move(borrowed), async);
  if (status != ZX_OK)
    Unbind();
  return status;
}

zx::tipc_chan ChanMessageWriter::Unbind() {
  if (!is_bound())
    return zx::tipc_chan();

  __UNUSED zx_handle_t borrowed = waiter_.Unbind().release();

  async_ = nullptr;
  zx::tipc_chan chan = std::move(chan_);
  return chan;
}

zx_status_t ChanMessageWriter::Queue(async_t* async, const zx_packet_user_t* data) {
  return async_queue_packet(async, this, data);
}

// static
void ChanMessageWriter::CallHandler(async_t* async, async_receiver_t* receiver, zx_status_t status,
                                    const zx_packet_user_t* data) {
  static_cast<ChanMessageWriter*>(receiver)->OnHandleReady(async, status, data);
}

void ChanMessageWriter::OnHandleReady(async_t* async, zx_status_t status,
                                      const zx_packet_user_t* data) {
  std::unique_lock<std::mutex> lock(mu_);
  while (!message_queue_.empty()) {
    auto& message = message_queue_.front();
    zx_status_t result = chan_.write(message.data(), message.size());
    if (result != ZX_OK) {
      if (result != ZX_ERR_SHOULD_WAIT)
        LOG(ERROR) << "Failed to write: " << result;
      break;
    }
    message_queue_.pop_front();
  }
}

zx_status_t ChanMessageWriter::WriteLater(std::string message) {
  if (!is_bound())
    return ZX_ERR_UNAVAILABLE;

  std::unique_lock<std::mutex> lock(mu_);
  message_queue_.push_back(std::move(message));
  if (message_queue_.size() == 1) {
    return Queue(async_, nullptr);
  }
  return ZX_OK;
}

}  // namespace ipc
}  // namespace nebula

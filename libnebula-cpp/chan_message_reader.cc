// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include "lib/nebula/ipc/chan_message_reader.h"

#include <lib/async/default.h>
#include <trusty_ipc.h>

#include <type_traits>

#include "lib/nebula/base/logging.h"

namespace nebula {
namespace ipc {

constexpr zx_signals_t kSignals = IPC_HANDLE_POLL_MSG | IPC_HANDLE_POLL_HUP;
constexpr size_t kPageSize = 4096;

ChanMessageReader::ChanMessageReader(ChanMessageHandler* message_handler)
    : async_wait_t{{ASYNC_STATE_INIT},
                   &ChanMessageReader::CallHandler,
                   ZX_HANDLE_INVALID,
                   kSignals,
                   0u,
                   {}},
      async_(nullptr),
      message_handler_(message_handler) {}

ChanMessageReader::~ChanMessageReader() {
  if (async_)
    async_cancel_wait(async_, this);
}

zx_status_t ChanMessageReader::Bind(zx::tipc_chan chan, async_t* async) {
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
  this->object = chan_.get();
  zx_status_t status = async_begin_wait(async_, this);
  if (status != ZX_OK)
    Unbind();
  return status;
}

zx::tipc_chan ChanMessageReader::Unbind() {
  if (!is_bound())
    return zx::tipc_chan();
  async_cancel_wait(async_, this);
  this->object = ZX_HANDLE_INVALID;
  async_ = nullptr;
  zx::tipc_chan chan = std::move(chan_);
  if (message_handler_)
    message_handler_->OnChannelGone();
  return chan;
}

// static
async_wait_result_t ChanMessageReader::CallHandler(async_t* async, async_wait_t* wait,
                                                   zx_status_t status,
                                                   const zx_packet_signal_t* signal) {
  return static_cast<ChanMessageReader*>(wait)->OnHandleReady(async, status, signal);
}

async_wait_result_t ChanMessageReader::OnHandleReady(async_t* async, zx_status_t status,
                                                     const zx_packet_signal_t* signal) {
  if (status != ZX_OK) {
    NotifyError();
    return ASYNC_WAIT_FINISHED;
  }

  if (signal->observed & IPC_HANDLE_POLL_MSG) {
    for (uint64_t i = 0; i < signal->count; i++) {
      zx_status_t result = ReadAndDispatchMessage();
      if (result == ZX_ERR_SHOULD_WAIT)
        return ASYNC_WAIT_AGAIN;
      if (result != ZX_OK)
        return ASYNC_WAIT_FINISHED;
    }
    return ASYNC_WAIT_AGAIN;
  }

  DCHECK(signal->observed & IPC_HANDLE_POLL_HUP);
  // Notice that we don't notify an error until we've drained all the messages
  // out of the channel.
  NotifyError();
  return ASYNC_WAIT_FINISHED;
}

zx_status_t ChanMessageReader::ReadAndDispatchMessage() {
  std::string message(kPageSize, 0);
  uint32_t actual = 0;
  zx_status_t status = chan_.read(&message[0], message.size(), &actual);
  if (status == ZX_ERR_SHOULD_WAIT)
    return status;
  if (status != ZX_OK) {
    NotifyError();
    return status;
  }
  if (!message_handler_)
    return ZX_OK;
  message.resize(actual);
  status = message_handler_->OnRawMessage(std::move(message));
  if (status != ZX_OK)
    NotifyError();
  return status;
}

void ChanMessageReader::NotifyError() {
  Unbind();
  if (error_handler_)
    error_handler_();
}

}  // namespace ipc
}  // namespace nebula

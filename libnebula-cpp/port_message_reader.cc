// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include "lib/nebula/ipc/port_message_reader.h"

#include <lib/async/default.h>
#include <trusty_ipc.h>

#include <type_traits>

namespace nebula {
namespace ipc {

constexpr zx_signals_t kSignals = IPC_HANDLE_POLL_READY;

PortMessageReader::PortMessageReader(PortMessageHandler* message_handler)
    : async_wait_t{{ASYNC_STATE_INIT},
                   &PortMessageReader::CallHandler,
                   ZX_HANDLE_INVALID,
                   kSignals,
                   0u,
                   {}},
      async_(nullptr),
      message_handler_(message_handler) {}

PortMessageReader::~PortMessageReader() {
  if (async_)
    async_cancel_wait(async_, this);
}

zx_status_t PortMessageReader::Bind(zx::tipc_port port, async_t* async) {
  if (is_bound())
    Unbind();
  if (!port)
    return ZX_OK;
  port_ = std::move(port);
  if (async) {
    async_ = async;
  } else {
    async_ = async_get_default();
  }
  this->object = port_.get();
  zx_status_t status = async_begin_wait(async_, this);
  if (status != ZX_OK)
    Unbind();
  return status;
}

zx::tipc_port PortMessageReader::Unbind() {
  if (!is_bound())
    return zx::tipc_port();
  async_cancel_wait(async_, this);
  this->object = ZX_HANDLE_INVALID;
  async_ = nullptr;
  zx::tipc_port port = std::move(port_);
  if (message_handler_)
    message_handler_->OnPortGone();
  return port;
}

// static
async_wait_result_t PortMessageReader::CallHandler(async_t* async, async_wait_t* wait,
                                                   zx_status_t status,
                                                   const zx_packet_signal_t* signal) {
  return static_cast<PortMessageReader*>(wait)->OnHandleReady(async, status, signal);
}

async_wait_result_t PortMessageReader::OnHandleReady(async_t* async, zx_status_t status,
                                                     const zx_packet_signal_t* signal) {
  if (status != ZX_OK) {
    NotifyError();
    return ASYNC_WAIT_FINISHED;
  }

  if (signal->observed & IPC_HANDLE_POLL_READY) {
    zx_status_t result = AcceptAndDispatchMessage();
    if (result != ZX_OK)
      return ASYNC_WAIT_FINISHED;
    return ASYNC_WAIT_AGAIN;
  }

  // Notice that we don't notify an error until we've drained all the messages
  // out of the channel.
  NotifyError();
  return ASYNC_WAIT_FINISHED;
}

zx_status_t PortMessageReader::AcceptAndDispatchMessage() {
  uuid_t uuid;
  zx::tipc_chan chan;
  zx_status_t status = port_.accept(&uuid, &chan);
  if (status != ZX_OK) {
    NotifyError();
    return status;
  }
  if (!message_handler_)
    return ZX_OK;
  status = message_handler_->OnNewChannel(uuid, std::move(chan));
  if (status != ZX_OK)
    NotifyError();
  return status;
}

void PortMessageReader::NotifyError() {
  Unbind();
  if (error_handler_)
    error_handler_();
}

}  // namespace ipc
}  // namespace nebula
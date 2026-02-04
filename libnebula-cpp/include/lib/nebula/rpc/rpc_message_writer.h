// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include "lib/nebula/base/macros.h"
#include "lib/nebula/ipc/chan_message_writer.h"
#include "lib/nebula/rpc/message.h"

namespace nebula {
namespace rpc {

using namespace nebula::ipc;

class RpcMessageWriter {
 public:
  RpcMessageWriter() {}
  virtual ~RpcMessageWriter() = default;

  zx_status_t Bind(zx::tipc_chan chan, async_t* async = nullptr);

  virtual zx_status_t Write(const RpcMessage& message);
  virtual zx_status_t Write(uint32_t txid, uint32_t ordinal, const std::string& payload);

  // Indicate the outgoing rpc message should set response bit or not.
  virtual bool has_response_feature() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(RpcMessageWriter);

  ChanMessageWriter chan_writer_;
};

class RpcRequestWriter : public RpcMessageWriter {
 public:
  bool has_response_feature() final { return false; }
};

class RpcResponseWriter : public RpcMessageWriter {
 public:
  bool has_response_feature() final { return true; }
};

}  // namespace rpc
}  // namespace nebula
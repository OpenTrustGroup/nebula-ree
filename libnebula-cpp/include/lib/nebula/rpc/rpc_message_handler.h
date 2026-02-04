// Copyright 2021 The GoldenRiver Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include "lib/nebula/rpc/message.h"

namespace nebula {
namespace rpc {

class RpcMessageHandler {
 public:
  virtual ~RpcMessageHandler();

  virtual void OnRpcMessage(RpcMessage message) = 0;
};

}  // namespace rpc
}  // namespace nebula
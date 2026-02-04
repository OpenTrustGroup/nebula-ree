// Copyright 2022 GoldenRiver Technology Co., Ltd.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <lib/nebula/base/logging.h>
#include <lib/nebula/base/unique_fd.h>
#include "tap.h"

class NetBase {
 protected:
  explicit NetBase(std::string name);

  int set_backend(int fd, uint8_t queue_index, Tap *tap);
  Tap tap_;
  std::string ip_;
  std::string netmask_;
};

class RprocNet : public NetBase {
 public:
  RprocNet(std::string name);

 private:
  nebula::base::unique_fd fd_;
};

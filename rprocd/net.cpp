// Copyright 2022 GoldenRiver Technology Co., Ltd.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net.h"
#include "vhost.h"

static std::string DefaultIP = "192.168.1.1";
static std::string DefaultNetmask = "255.255.255.0";
static constexpr char kRprocPath[] = "/dev/rproc-net";
static constexpr char kVnetNumQueues = 2;

NetBase::NetBase(std::string name)
    : tap_(name),
      ip_(DefaultIP), netmask_(DefaultNetmask) {}

int NetBase::set_backend(int fd, uint8_t queue_index, Tap* tap) {
  vhost_vring_file args = {
      .index = queue_index,
      .fd = tap->raw_descriptor(),
  };

  int ret = ioctl(fd, VHOST_NET_SET_BACKEND, &args);
  if (ret < 0) {
    LOG(ERROR) << "failed to set backend: " << strerror(errno);
    return ret;
  }

  return 0;
}

RprocNet::RprocNet(std::string name)
  : NetBase(name), fd_(open(kRprocPath, O_RDWR)) {

  for (uint8_t idx = 0; idx < kVnetNumQueues; idx++) {
    auto status = set_backend(fd_, idx, &tap_);
    CHECK(status == 0);
  }
  tap_.set_ip_addr(ip_, netmask_);
}

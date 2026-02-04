// Copyright 2022 GoldenRiver Technology Co., Ltd.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string.h>
#include <sys/ioctl.h>

#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include "lib/nebula/base/unique_fd.h"
#include "lib/nebula/base/logging.h"

static constexpr char kTunPath[] = "/dev/net/tun";

#define IFF_NAPI    0x0010
#define IFF_NAPI_FRAGS  0x0020

class Tap {
 public:
  Tap(std::string name) : name_(name), tap_fd_(open(kTunPath, O_RDWR)) {
    CHECK(tap_fd_.ok()) << "failed to open " << kTunPath;

    struct ifreq ifr;
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI | IFF_VNET_HDR | IFF_NAPI | IFF_NAPI_FRAGS;
    strncpy(ifr.ifr_name, name_.c_str(), IFNAMSIZ);

    /* try to create the device */
    auto ret = ioctl(tap_fd_.get(), TUNSETIFF, &ifr);
    CHECK(ret == 0) << "failed to create tap device: " << strerror(errno);
  }

  int raw_descriptor() { return tap_fd_.get(); }

  int set_vnet_header_size(size_t size) {
    int ret = ioctl(tap_fd_.get(), TUNSETVNETHDRSZ, &size);
    if (ret < 0) {
      LOG(ERROR) << "failed to set vnet header size: " << strerror(errno);
      return ret;
    }
    return 0;
  }

  void set_ip_addr(std::string ip, std::string netmask) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    struct ifreq ifr = {};
    strncpy(ifr.ifr_name, name_.c_str(), IFNAMSIZ);

    struct sockaddr_in inet_addr;
    inet_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &inet_addr.sin_addr);
    memcpy(&ifr.ifr_addr, &inet_addr, sizeof(struct sockaddr));
    int ret = ioctl(sockfd, SIOCSIFADDR, &ifr);
    CHECK(ret == 0) << strerror(errno);

    struct sockaddr_in netmask_addr;
    netmask_addr.sin_family = AF_INET;
    inet_pton(AF_INET, netmask.c_str(), &netmask_addr.sin_addr);
    memcpy(&ifr.ifr_netmask, &netmask_addr, sizeof(struct sockaddr));
    ret = ioctl(sockfd, SIOCSIFNETMASK, &ifr);
    CHECK(ret == 0) << strerror(errno);

    ifr.ifr_flags |= IFF_BROADCAST | IFF_UP;
    ret = ioctl(sockfd, SIOCSIFFLAGS, &ifr);
    CHECK(ret == 0) << strerror(errno);

    close(sockfd);
  }

 private:
  std::string name_;
  nebula::base::unique_fd tap_fd_;
};

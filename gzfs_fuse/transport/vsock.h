// SPDX-License-Identifier: NebulaProprietary
/*
* Copyright (c) 2025 MediaTek Inc.
* Copyright 2025 GoldenRiver Technologies Co., Ltd. All rights reserved.
*/

#ifndef TRANSPORT_VSOCK_H_
#define TRANSPORT_VSOCK_H_

#include <stdint.h>
#include <mutex>

#include "protocol.h"

namespace transport {

class VmSocket
{
public:
  VmSocket();
  ~VmSocket();

  int init(void);
  void shutdown(void);
  int send_request(request_t *req, void *req_data,
                    response_t *resp, void *resp_buf, size_t resp_buf_len);
  int submit_request(request_t* req, void* req_data,
                   response_t* resp, void* resp_buf, size_t resp_buf_len);

private:
  int do_recv(response_t* resp, void* resp_buf, size_t resp_buf_len);
  int do_send(const request_t* req, const void* req_data);
  int vsock_connect(uint32_t cid, uint32_t port);

  std::mutex request_mutex_;
  int socket_fd;
  context_t ctx;
};

} // namespace transport

#endif // TRANSPORT_VSOCK_H_

// SPDX-License-Identifier: NebulaProprietary
/*
* Copyright (c) 2025 MediaTek Inc.
* Copyright 2025 GoldenRiver Technologies Co., Ltd. All rights reserved.
*/

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/vm_sockets.h>
#include <errno.h>

#include "vsock.h"

namespace transport {

namespace {
constexpr int32_t YOCTO_CID   = 2;// yocto CID
__attribute__((unused)) constexpr int32_t ANDROID_CID = 3;   // android CID
__attribute__((unused)) constexpr int32_t TBOX_CID    = 4;   // tbox CID
constexpr int32_t PORT        = 5022;    // service port

}  // namespace

int VmSocket::do_send(const request_t* req, const void* req_data) {
  ssize_t ret;
  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]sent req header: opcode=%d, seq=%d, length=%d\n", __PRETTY_FUNCTION__,
                                __LINE__, req->opcode, req->seq, req->length);
  ret = send(socket_fd, req, sizeof(request_t), 0);
  if (ret < 0) {
    fuse_log(FUSE_LOG_ERR, "[%s:%d]send request header fail: %s\n", __PRETTY_FUNCTION__,
                                                __LINE__, strerror(errno));
    return ret;
  } else if (ret != sizeof(request_t)) {
    fuse_log(FUSE_LOG_ERR, "[%s:%d]send request header incomplete, expected=%zu, actual=%zd\n",
                              __PRETTY_FUNCTION__, __LINE__, sizeof(request_t), ret);
    return -EIO;
  }

  if (req->length > 0 && req_data) {
    ret = send(socket_fd, req_data, req->length, 0);
    if (ret < 0) {
      fuse_log(FUSE_LOG_ERR, "[%s:%d]send request data fail: %s\n", __PRETTY_FUNCTION__, __LINE__, strerror(errno));
      return ret;
    } else if (ret != req->length) {
      fuse_log(FUSE_LOG_ERR, "[%s:%d]send request data incomplete, expected=%u, actual=%zd\n",
                                      __PRETTY_FUNCTION__, __LINE__, req->length, ret);
      return -EIO;
    }
  }

  return 0;
}

int VmSocket::do_recv(response_t* resp, void* resp_buf, size_t resp_buf_len) {
  ssize_t ret;

  ret = recv(socket_fd, resp, sizeof(response_t), MSG_WAITALL);
  if (ret < 0) {
    fuse_log(FUSE_LOG_ERR, "[%s:%d]receive response header fail: %s\n", __PRETTY_FUNCTION__, __LINE__, strerror(errno));
    return ret;
  } else if (ret != sizeof(response_t)) {
    fuse_log(FUSE_LOG_ERR, "[%s:%d]receive response header incomplete, expected=%zu, actual=%zd\n",
                              __PRETTY_FUNCTION__, __LINE__, sizeof(response_t), ret);
    return -EIO;
  }

  if (resp->length > 0 && resp_buf) {
    if (resp->length > resp_buf_len) {
      fuse_log(FUSE_LOG_ERR, "[%s:%d]response buffer too small, expected=%zu, actual=%u\n",
                                __PRETTY_FUNCTION__, __LINE__, resp_buf_len, resp->length);
      return -EMSGSIZE;
    }
    ret = recv(socket_fd, resp_buf, resp->length, MSG_WAITALL);
    if (ret < 0) {
      fuse_log(FUSE_LOG_ERR, "[%s:%d]receive response data fail: %s\n",__PRETTY_FUNCTION__, __LINE__, strerror(errno));
      return ret;
    } else if (ret != resp->length) {
      fuse_log(FUSE_LOG_ERR, "[%s:%d]receive response data incomplete, expected=%u, actual=%zd\n",
                                  __PRETTY_FUNCTION__, __LINE__, resp->length, ret);
      return -EIO;
    }
  }

  return 0;
}

int VmSocket::submit_request(request_t* req, void* req_data,
                   response_t* resp, void* resp_buf, size_t resp_buf_len) {
  int ret = 0;

  std::unique_lock<std::mutex> lock(request_mutex_);

  ret = do_send(req, req_data);
  if (ret < 0) {
    goto out;
  }

  ret = do_recv(resp, resp_buf, resp_buf_len);
  if (ret < 0) {
    goto out;
  }

out:
  lock.unlock();
  return ret;
}

int VmSocket::send_request(request_t *req, void *req_data,
                    response_t *resp, void *resp_buf,
                    size_t resp_buf_len) {
  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]sent req header: opcode=%d, seq=%d, length=%d\n",
                                __PRETTY_FUNCTION__, __LINE__, req->opcode, req->seq, req->length);
  if (send(socket_fd, req, sizeof(request_t), 0) != sizeof(request_t)) {
    fuse_log(FUSE_LOG_ERR, "[%s:%d]send request header fail: %s\n", __PRETTY_FUNCTION__,
                                                __LINE__, strerror(errno));
    return -1;
  }

  if (req->length > 0 && req_data) {
    fuse_log(FUSE_LOG_DEBUG, "[%s:%d]sent req data: length=%d\n",
                                __PRETTY_FUNCTION__, __LINE__, req->length);
    if (send(socket_fd, req_data, req->length, 0) != req->length) {
      return -1;
    }
  }

  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]start receive response\n", __PRETTY_FUNCTION__, __LINE__);
  if (recv(socket_fd, resp, sizeof(response_t), MSG_WAITALL) != sizeof(response_t)) {
    return -1;
  }

  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]receive response header: opcode=%d, seq=%d, result=%d, length=%d\n",
                                  __PRETTY_FUNCTION__, __LINE__, resp->opcode, resp->seq, resp->result, resp->length);
  if (resp->length > 0 && resp_buf) {
    if (resp->length > resp_buf_len) {
      fuse_log(FUSE_LOG_ERR, "[%s:%d]error : response buffer too small, expected=%zu, actual=%u\n",
                   __PRETTY_FUNCTION__, __LINE__, resp_buf_len, resp->length);
      return -1;
    }
    fuse_log(FUSE_LOG_DEBUG, "[%s:%d]start receive response data: length=%d\n",__PRETTY_FUNCTION__, __LINE__,  resp->length);
    if (recv(socket_fd, resp_buf, resp->length, MSG_WAITALL) != resp->length) {
      return -1;
    }
  }

  return 0;
}

int VmSocket::vsock_connect(uint32_t cid, uint32_t port) {
  int fd = socket(AF_VSOCK, SOCK_STREAM, 0);
  if (fd < 0) {
    fuse_log(FUSE_LOG_DEBUG, "[%s:%d]Failed to create VSOCK errno=%d\n", __PRETTY_FUNCTION__, __LINE__, errno);
    return -1;
  }

  struct sockaddr_vm addr = {
    .svm_family = AF_VSOCK,
    .svm_port = port,
    .svm_cid = cid,
  };

  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    fuse_log(FUSE_LOG_DEBUG, "[%s:%d]Failed to connect to vsock cid=%d port=%d, errno=%d\n",
                                __PRETTY_FUNCTION__, __LINE__, cid, port, errno);
    close(fd);
    return -1;
  }

  return fd;
}

int VmSocket::init(void) {
  int ret = 0;

  socket_fd = vsock_connect(ctx.server_cid, ctx.server_port);
  if (socket_fd < 0) {
      fuse_log(FUSE_LOG_DEBUG, "[%s:%d]failed to connect to gzfs vsock service\n",__PRETTY_FUNCTION__, __LINE__);
      return -EIO;
  }

  return ret;
}

void VmSocket::shutdown(void) {
  fuse_log(FUSE_LOG_INFO, "[%s:%d]\n",__PRETTY_FUNCTION__, __LINE__);
  if (socket_fd > 0) {
    close(socket_fd);
  }
}

VmSocket::VmSocket()
    : socket_fd(-1) {
  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]Constructor\n",__PRETTY_FUNCTION__, __LINE__);
  ctx = {
    .server_cid =   YOCTO_CID,
    .server_port =  PORT,
  };
}

VmSocket::~VmSocket() {
  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]Destructor\n",__PRETTY_FUNCTION__, __LINE__);
}

}  // namespace transport

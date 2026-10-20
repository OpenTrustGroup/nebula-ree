// SPDX-License-Identifier: NebulaProprietary
/*
* Copyright (c) 2025 MediaTek Inc.
* Copyright 2025 GoldenRiver Technologies Co., Ltd. All rights reserved.
*/

#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "interface.h"
#include "../transport/vsock.h"

static void make_full_path(char *full, const char *path) {
  snprintf(full, MAX_PATH, "%s", path);
}

int vsock_read(const char *path, char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi) {
  (void)fi;
  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]begin\n", __PRETTY_FUNCTION__, __LINE__);

  request_t req = {
    .opcode = OP_READ,
    .seq = OP_READ,
  };

  read_req_t req_data;
  make_full_path(req_data.path, path);
  req_data.offset = offset;
  req_data.size = size;
  req.length = sizeof(read_req_t);

  response_t resp;
  char *resp_buf = (char *)malloc(sizeof(response_t) + size);
  if (!resp_buf) {
    fuse_log(FUSE_LOG_ERR, "[%s:%d]Failed to malloc errno=%d,size=%zu\n", __PRETTY_FUNCTION__,
                                __LINE__, errno, size);
    return -ENOMEM;
  }
  transport::VmSocket* vsock = nullptr;

  struct fuse_context *cxt = fuse_get_context();
  if (cxt) {
    vsock = reinterpret_cast<transport::VmSocket*>(cxt->private_data);
    if (vsock->submit_request(&req, &req_data, &resp, resp_buf, sizeof(response_t) + size) < 0) {
      free(resp_buf);
      return -EIO;
    }
  }

  if (resp.result != 0) {
    free(resp_buf);
    return resp.result;
  }

  read_resp_t *read_resp = (read_resp_t *)resp_buf;
  size_t copy_size = (read_resp->size < size) ? read_resp->size : size;
  memcpy(buf, read_resp->data, copy_size);

  free(resp_buf);
  return copy_size;
}

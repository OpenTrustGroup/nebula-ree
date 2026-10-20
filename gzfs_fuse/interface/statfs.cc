// SPDX-License-Identifier: NebulaProprietary
/*
* Copyright (c) 2025 MediaTek Inc.
* Copyright 2025 GoldenRiver Technologies Co., Ltd. All rights reserved.
*/

#include <cstdio>
#include <cerrno>
#include <cstring>

#include "interface.h"
#include "../transport/vsock.h"

int vsock_statfs(const char *path, struct statvfs *stbuf) {
  (void)path;
  fuse_log(FUSE_LOG_INFO, "[%s:%d]begin\n", __PRETTY_FUNCTION__, __LINE__);

  request_t req = {
    .opcode = OP_STATFS,
    .seq = OP_STATFS,
  };

  response_t resp;
  statfs_t resp_data;
  transport::VmSocket* vsock = nullptr;

  struct fuse_context *cxt = fuse_get_context();
  if (cxt) {
    vsock = reinterpret_cast<transport::VmSocket*>(cxt->private_data);
    if (vsock->submit_request(&req, NULL, &resp, &resp_data, sizeof(resp_data)) < 0) {
      return -EIO;
    }
  }

  if (resp.result != 0) {
    return resp.result;
  }

  memset(stbuf, 0, sizeof(struct statvfs));
  stbuf->f_bsize = resp_data.bsize;
  stbuf->f_frsize = resp_data.bsize;
  stbuf->f_blocks = resp_data.blocks;
  stbuf->f_bfree = resp_data.bfree;
  stbuf->f_bavail = resp_data.bavail;
  stbuf->f_files = resp_data.files;
  stbuf->f_ffree = resp_data.ffree;
  stbuf->f_namemax = resp_data.namelen;

  return 0;
}

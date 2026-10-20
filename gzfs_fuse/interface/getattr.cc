// SPDX-License-Identifier: NebulaProprietary
/*
* Copyright (c) 2025 MediaTek Inc.
* Copyright 2025 GoldenRiver Technologies Co., Ltd. All rights reserved.
*/

#include <cstdio>
#include <cerrno>

#include "interface.h"
#include "../transport/vsock.h"

static int convert_stat_fixed_to_stat(const struct stat_fixed *src, struct stat *dst) {
  dst->st_dev = src->st_dev;
  dst->st_ino = src->st_ino;
  dst->st_mode = src->st_mode;
  if (sizeof(dst->st_nlink) == sizeof(uint32_t)) {
    if (src->st_nlink > UINT32_MAX) {
      return -EOVERFLOW;
    }
    dst->st_nlink = (uint32_t)src->st_nlink;
  } else {
    dst->st_nlink = src->st_nlink;
  }
  dst->st_uid = src->st_uid;
  dst->st_gid = src->st_gid;
  dst->st_rdev = src->st_rdev;
  dst->st_size = src->st_size;
  if (sizeof(dst->st_blksize) == sizeof(int32_t)) {
    if (src->st_blksize > INT32_MAX) {
      return -EOVERFLOW;
    }
    dst->st_blksize = (int32_t)src->st_blksize;
  } else {
    dst->st_blksize = src->st_blksize;
  }
  dst->st_blksize = src->st_blksize;
  dst->st_blocks = src->st_blocks;
  dst->st_atim = src->st_atim;
  dst->st_mtim = src->st_mtim;
  dst->st_ctim = src->st_ctim;

  return 0;
}

static void make_full_path(char *full, const char *path) {
  snprintf(full, MAX_PATH, "%s", path);
}

int vsock_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
  (void)fi;
  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]begin\n", __PRETTY_FUNCTION__, __LINE__);

  request_t req = {
    .opcode = OP_GETATTR,
    .seq = OP_GETATTR,
  };

  getattr_req_t req_data;
  make_full_path(req_data.path, path);
  req.length = sizeof(getattr_req_t);

  response_t resp;
  getattr_resp_t resp_data;
  transport::VmSocket* vsock = nullptr;

  struct fuse_context *cxt = fuse_get_context();
  if (cxt) {
    vsock = reinterpret_cast<transport::VmSocket*>(cxt->private_data);
    //SPDLOG_DEBUG("vsock={}", fmt::ptr(vsock));
    if (vsock->submit_request(&req, &req_data, &resp, &resp_data, sizeof(resp_data)) < 0) {
      fuse_log(FUSE_LOG_ERR, "[%s:%d]submit_request fail,errno=%d\n",
                                    __PRETTY_FUNCTION__, __LINE__, errno);
      return -EIO;
    }
  }

  if (resp.result != 0) {
    return resp.result;
  }

  return convert_stat_fixed_to_stat(&resp_data.st, st);
}

// SPDX-License-Identifier: NebulaProprietary
/*
* Copyright (c) 2025 MediaTek Inc.
* Copyright 2025 GoldenRiver Technologies Co., Ltd. All rights reserved.
*/

#include <cstdio>
#include <cerrno>

#include "interface.h"

static void make_full_path(char *full, const char *path) {
  snprintf(full, MAX_PATH, "%s", path);
}

int vsock_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi,
                        enum fuse_readdir_flags flags) {
  (void)offset;
  (void)fi;
  (void)flags;
  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]begin\n", __PRETTY_FUNCTION__, __LINE__);
  request_t req = {
    .opcode = OP_READDIR,
    .seq = OP_READDIR,
  };

  readdir_req_t req_data;
  make_full_path(req_data.path, path);
  req.length = sizeof(readdir_req_t);

  response_t resp;
  char resp_buf[8192];
  transport::VmSocket* vsock = nullptr;

  struct fuse_context *cxt = fuse_get_context();
  if (cxt) {
    vsock = reinterpret_cast<transport::VmSocket*>(cxt->private_data);
    if (vsock->submit_request(&req, &req_data, &resp, resp_buf, sizeof(resp_buf)) < 0) {
      return -EIO;
    }
  }

  if (resp.result != 0) {
    return resp.result;
  }

  //filler(buf, ".", NULL, 0);
  //filler(buf, "..", NULL, 0);
  filler(buf, ".", NULL, 0, (enum fuse_fill_dir_flags)0);
  filler(buf, "..", NULL, 0, (enum fuse_fill_dir_flags)0);

  char *ptr = resp_buf;
  uint32_t remaining = resp.length;

  while (remaining >= sizeof(dirent_t)) {
    dirent_t *de = (dirent_t *)ptr;

    size_t entry_size = sizeof(dirent_t);

    filler(buf, de->name, NULL, 0, (enum fuse_fill_dir_flags)0);

    ptr += entry_size;
    remaining -= entry_size;
  }

  return 0;
}

// SPDX-License-Identifier: NebulaProprietary
/*
* Copyright (c) 2025 MediaTek Inc.
* Copyright 2025 GoldenRiver Technologies Co., Ltd. All rights reserved.
*/

#include <cstdio>
#include <cerrno>

#include "interface.h"

int vsock_open(const char *path, struct fuse_file_info *fi) {
  (void)path;
  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]begin\n", __PRETTY_FUNCTION__, __LINE__);
  if ((fi->flags & O_ACCMODE) != O_RDONLY) {
    return -EACCES;
  }
  return 0;
}

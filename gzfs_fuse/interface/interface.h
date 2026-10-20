// SPDX-License-Identifier: NebulaProprietary
/*
* Copyright (c) 2025 MediaTek Inc.
* Copyright 2025 GoldenRiver Technologies Co., Ltd. All rights reserved.
*/

#ifndef INTERFACE_INTERFACE_H_
#define INTERFACE_INTERFACE_H_

#include <fuse.h>

#include "../transport/protocol.h"
#include "../transport/vsock.h"

#ifdef __cplusplus
extern "C" {
#endif

int vsock_getattr(const char *path, struct stat *st, struct fuse_file_info *fi);
int vsock_open(const char *path, struct fuse_file_info *fi);
int vsock_read(const char *path, char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi);
int vsock_statfs(const char *path, struct statvfs *stbuf);
int vsock_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi,
                        enum fuse_readdir_flags flags);

#ifdef __cplusplus
}
#endif

#endif // INTERFACE_INTERFACE_H_

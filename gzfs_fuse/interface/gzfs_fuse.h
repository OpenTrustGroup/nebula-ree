// SPDX-License-Identifier: NebulaProprietary
/*
* Copyright (c) 2025 MediaTek Inc.
* Copyright 2025 GoldenRiver Technologies Co., Ltd. All rights reserved.
*/

#ifndef INTERFACE_GZFS_FUSE_H_
#define INTERFACE_GZFS_FUSE_H_

#include <fuse.h>
#include "../transport/vsock.h"

using transport::VmSocket;

namespace interface {

class GzfsFuse
{
public:
  GzfsFuse();
  ~GzfsFuse();

  int init(int argc, char **argv, VmSocket* vsock);
  void shutdown(void);
private:
  struct fuse_operations gzfs_ops;
  VmSocket* vsock_;
};

} // namespace interface

#endif // INTERFACE_GZFS_FUSE_H_

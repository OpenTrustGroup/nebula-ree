// SPDX-License-Identifier: NebulaProprietary
/*
* Copyright (c) 2025 MediaTek Inc.
* Copyright 2025 GoldenRiver Technologies Co., Ltd. All rights reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <linux/vm_sockets.h>
#include <errno.h>

#include "gzfs_fuse.h"
#include "interface.h"

#include <fuse_lowlevel.h>

namespace interface {

namespace {
__attribute__((unused)) void *gzfs_init(struct fuse_conn_info *conn,
               struct fuse_config *cfg) {
  (void)conn;
  (void)cfg;
  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]\n", __PRETTY_FUNCTION__, __LINE__);
  return NULL;
}

__attribute__((unused)) void gzfs_destroy(void *private_data) {
  (void)private_data;
  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]\n", __PRETTY_FUNCTION__, __LINE__);
}
}  // namespace

GzfsFuse::GzfsFuse() {
  fuse_log_enable_syslog("gzfs_d", LOG_PID | LOG_CONS, LOG_DAEMON);
  fuse_log(FUSE_LOG_INFO, "[%s:%d]Constructor, fuse_pkgversion=%s\n", __PRETTY_FUNCTION__,
                            __LINE__, fuse_pkgversion());

  memset(&gzfs_ops, 0, sizeof(gzfs_ops));
  //gzfs_ops.init = gzfs_init;
  //gzfs_ops.destroy = gzfs_destroy;
  gzfs_ops.open = vsock_open;
  gzfs_ops.read = vsock_read;
  gzfs_ops.getattr = vsock_getattr;
  gzfs_ops.statfs  = vsock_statfs;
  gzfs_ops.readdir = vsock_readdir;
}

GzfsFuse::~GzfsFuse() {
  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]Destructor\n", __PRETTY_FUNCTION__, __LINE__);
}

int GzfsFuse::init(int32_t argc, char** argv, VmSocket* vsock) {
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  struct fuse_cmdline_opts opts;
  int ret = 1;

  if (fuse_parse_cmdline(&args, &opts) != 0) {
    fuse_log(FUSE_LOG_ERR, "[%s:%d]failed to parse option\n", __PRETTY_FUNCTION__, __LINE__);
    return 1;
  }

  if (opts.show_help) {
    printf("usage: %s [options] <mountpoint>\n\n", argv[0]);
    fuse_cmdline_help();
    fuse_lowlevel_help();
    ret = 0;
    goto err_out1;
  } else if (opts.show_version) {
    printf("FUSE library version %s\n", fuse_pkgversion());
    fuse_lowlevel_version();
    ret = 0;
    goto err_out1;
  }
  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]opts.mountpoint ret=%s\n", __PRETTY_FUNCTION__,
                                  __LINE__, opts.mountpoint);
  if(opts.mountpoint == NULL) {
    printf("usage: %s [options] <mountpoint>\n", argv[0]);
    printf("       %s --help\n", argv[0]);
    ret = 1;
    goto err_out1;
  }

  fuse_opt_add_arg(&args, "-f");
  fuse_opt_add_arg(&args, "-s");
  fuse_opt_add_arg(&args, opts.mountpoint);

  vsock_ = vsock;
  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]vsock_=%p\n", __PRETTY_FUNCTION__, __LINE__, vsock_);

  ret = fuse_main(args.argc, args.argv, &gzfs_ops, vsock_);
  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]fuse_main ret=%d\n", __PRETTY_FUNCTION__, __LINE__, ret);

err_out1:
  free(opts.mountpoint);
  fuse_opt_free_args(&args);
  return ret;
}

void GzfsFuse::shutdown(void) {
  fuse_log(FUSE_LOG_INFO, "[%s:%d]\n", __PRETTY_FUNCTION__, __LINE__);
  fuse_log_close_syslog();
}

}  // namespace interface

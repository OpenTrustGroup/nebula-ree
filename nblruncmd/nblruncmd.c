// Copyright 2018 GoldenRiver Technologies Co., Ltd. All rights reserved.
// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zircon/boot/netboot.h>

#include "netprotocol.h"
#include "version.h"

static const size_t kMaxMsgSizeInBytes = MAXSIZE;  // netprotocol.h
static int g_nbdev_fd = -1;
static volatile sig_atomic_t g_should_stop = 0;
static const int kShellCmdArgStart = 1;
static const int kShellCmdArgStop = 0;
static bool opt_thyp = false;
static bool opt_version = false;

static ssize_t netboot_shell_cmd(int devfd,
                                 const void* bytes,
                                 size_t num_bytes,
                                 uint32_t arg) {
  msg nbmsg;
  nbmsg.hdr.magic = NB_MAGIC;
  nbmsg.hdr.cookie = 0x11224455;
  nbmsg.hdr.cmd = NB_SHELL_CMD;
  nbmsg.hdr.arg = arg;
  num_bytes = num_bytes > kMaxMsgSizeInBytes ? kMaxMsgSizeInBytes : num_bytes;
  memcpy(nbmsg.data, bytes, num_bytes);
  return write(devfd, &nbmsg, sizeof(nbmsg));
}

static void sigint_handler(int sig) {
  netboot_shell_cmd(g_nbdev_fd, NULL, 0, kShellCmdArgStop);
}

static int parse_argv(int argc, char** argv, char* buffer, size_t buffer_len) {
  // Skip program name
  argc--;
  argv++;

  if (strncmp(*argv, "-v", 2) == 0) {
    opt_version = true;
    return 0;
  }

  if (strncmp(*argv, "-t", 2) == 0) {
    opt_thyp = true;
    argc--;
    argv++;
  }

  int bytes_remaining = buffer_len - 1;
  int bytes_copied = 0;
  while (0 < bytes_remaining && 0 < argc) {
    size_t len = strnlen(*argv, bytes_remaining - 1);
    strncpy(buffer + bytes_copied, *argv, len);
    strncpy(buffer + bytes_copied + len, " ", 1);
    bytes_copied += len + 1;
    bytes_remaining -= bytes_copied;
    argc--;
    argv++;
  }
  *(buffer + bytes_copied - 1) = 0;
  return 0;
}

int readmsg_and_handle(int devfd) {
  char bytes[2048] = {};
  ssize_t bytes_read = 0;
  bytes_read = read(devfd, bytes, sizeof(bytes));
  if (bytes_read < 0) {
    fprintf(stderr, "%s: failed to read msg (%d)\n", __func__, errno);
    return -1;
  }

  if (bytes_read >= sizeof(struct nbmsg_t) &&
      ((struct nbmsg_t*)bytes)->magic == NB_MAGIC &&
      ((struct nbmsg_t*)bytes)->cookie == 0x11224455 &&
      ((struct nbmsg_t*)bytes)->cmd == NB_ACK) {
    g_should_stop = true;
    return ((struct nbmsg_t*)bytes)->arg;
  }
  // gtest will output non-printable/ascii chars.
  // print char by char and filter by isascii()
  // instead of printf("%s") for pretty output.
  ssize_t bytes_write = 0;
  while (bytes_write < bytes_read) {
    if (isascii(bytes[bytes_write]))
      printf("%c", bytes[bytes_write]);
    bytes_write++;
  }
  return 0;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: nblruncmd [-t] <command>\n");
    fprintf(stderr, " -t   send command to t-hyp\n");
    return -1;
  }

  char buffer[1024];
  parse_argv(argc, argv, buffer, sizeof(buffer));

  if (opt_version) {
    printf(VERSION_FORMAT, VERSION_MAJOR, VERSION_MINOR, NEBULA_VERSION_STR);
    return 0;
  }

  int srvfd;
  if (opt_thyp) {
    srvfd = netboot_open(NBLSVC_NB_SERVER_PORT, TIPC_HYP_DEVNAME, NULL, false);
  } else {
    srvfd = netboot_open(NBLSVC_NB_SERVER_PORT, TIPC_TEE_DEVNAME, NULL, false);
  }

  if (srvfd < 0) {
    if (errno == ETIMEDOUT) {
      fprintf(stderr, "connect to '%s' timed out\n", NBLSVC_NB_SERVER_PORT);
    }
    return -1;
  }
  g_nbdev_fd = srvfd;

  signal(SIGINT, sigint_handler);
  netboot_shell_cmd(srvfd, buffer, sizeof(buffer), kShellCmdArgStart);

  int retcode = 0;
  while (!g_should_stop) {
    retcode = readmsg_and_handle(srvfd);
    if (retcode != 0)
      break;
  }

  close(srvfd);
  return retcode;
}

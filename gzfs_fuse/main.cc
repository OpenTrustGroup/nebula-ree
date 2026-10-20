// SPDX-License-Identifier: NebulaProprietary
/*
* Copyright (c) 2025 MediaTek Inc.
* Copyright 2025 GoldenRiver Technologies Co., Ltd. All rights reserved.
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <assert.h>
#include <memory>
#include <execinfo.h>
#include <cxxabi.h>

#include "transport/vsock.h"
#include "interface/gzfs_fuse.h"

using interface::GzfsFuse;
using transport::VmSocket;

#define BT_BUF_SIZE 100

static void dump_backtrace(int sig) {
  void *buffer[BT_BUF_SIZE];
  char **strings;
  int nptrs;

  fuse_log(FUSE_LOG_INFO, "[%s:%d]recv sig=%d %s\n", __PRETTY_FUNCTION__,
                                  __LINE__, sig, strsignal(sig));

  nptrs = backtrace(buffer, BT_BUF_SIZE);
  fuse_log(FUSE_LOG_INFO, "[%s:%d]backtrace() returned %d addresses\n", __PRETTY_FUNCTION__,
                                  __LINE__, nptrs);

  strings = backtrace_symbols(buffer, nptrs);
  if (strings == NULL) {
    fuse_log(FUSE_LOG_ERR, "[%s:%d]backtrace_symbols fail,errno=%d %s\n", __PRETTY_FUNCTION__,
                                    __LINE__, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  for (int j = 0; j < nptrs; j++) {
    char* line = strings[j];
    char* open_paren = strchr(line, '(');
    char* close_paren = strchr(line, ')');

    if (open_paren && close_paren && open_paren < close_paren) {
      *close_paren = '\0';
      char* plus_pos = strchr(open_paren + 1, '+');

      std::string mangled;
      if (plus_pos) {
        *plus_pos = '\0';
        mangled = open_paren + 1;
        *plus_pos = '+';
      } else {
        mangled = open_paren + 1;
      }
      *close_paren = ')';

      int status = 0;
      char* demangled = abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &status);

      if (status == 0 && demangled) {
        std::string new_line;
        new_line.append(line, open_paren - line + 1);
        new_line += demangled;
        if (plus_pos) {
          new_line += plus_pos;
        }
        new_line += close_paren;

        fuse_log(FUSE_LOG_INFO, "[%s:%d]%s\n", __PRETTY_FUNCTION__,
                                  __LINE__, new_line.c_str());
        free(demangled);
      } else {
        fuse_log(FUSE_LOG_INFO, "[%s:%d]%s\n", __PRETTY_FUNCTION__,
                                  __LINE__, line);
      }
    } else {
      fuse_log(FUSE_LOG_INFO, "[%s:%d]%s\n", __PRETTY_FUNCTION__,
                                  __LINE__, line);
    }
  }

  free(strings);
  exit(EXIT_FAILURE);
}

static void handle_signal(int signo, siginfo_t* info, void *arg) {
  (void)info;
  (void)arg;
  fuse_log(FUSE_LOG_INFO, "[%s:%d]recv sig=%d %s\n", __PRETTY_FUNCTION__,
                                  __LINE__, signo, strsignal(signo));
  if ((SIGSEGV == signo) || (SIGABRT == signo)) {
    dump_backtrace(signo);
  }
}

static void init_sighandler(void) {
  struct sigaction action;
  memset(&action, 0, sizeof(action));

  sigemptyset(&action.sa_mask);
  action.sa_sigaction = handle_signal;
  action.sa_flags = SA_RESTART | SA_SIGINFO;

  sigaction(SIGINT, &action, NULL);
  sigaction(SIGILL, &action, NULL);
  sigaction(SIGTRAP, &action, NULL);
  sigaction(SIGABRT, &action, NULL);
  sigaction(SIGBUS, &action, NULL);
  sigaction(SIGFPE, &action, NULL);
  sigaction(SIGSEGV, &action, NULL);
  sigaction(SIGQUIT, &action, NULL);
  signal(SIGPIPE, SIG_IGN);
}

__attribute__((unused)) static void mask_signals(void) {
  sigset_t sigset;

  sigemptyset(&sigset);
  sigaddset(&sigset, SIGTERM);
  sigaddset(&sigset, SIGINT);
  pthread_sigmask(SIG_BLOCK, &sigset, NULL);
  //sigprocmask(SIG_BLOCK, &sigset, nullptr);
}

__attribute__((unused)) static void wait_signals(void) {
  int sig;
  sigset_t newmask;

  sigemptyset(&newmask);
  sigaddset(&newmask, SIGTERM);
  sigaddset(&newmask, SIGINT);

  sigwait(&newmask, &sig);
  printf("recv signal {%#X} {%s}\n", sig, strsignal(sig));
}

int main(int32_t argc, char** argv) {
  int ret = 0;

  init_sighandler();

  printf("gzfs fuse start\n");

  std::unique_ptr<VmSocket> vsock;
  vsock = std::make_unique<VmSocket>();

  ret = vsock->init();
  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]vsock->init ret=%d\n", __PRETTY_FUNCTION__, __LINE__, ret);

  GzfsFuse gzfs;
  ret = gzfs.init(argc, argv, vsock.get());
  fuse_log(FUSE_LOG_DEBUG, "[%s:%d]gzfs.init ret=%d\n", __PRETTY_FUNCTION__, __LINE__, ret);

  gzfs.shutdown();
  vsock->shutdown();
  printf("gzfs fuse return, %d\n", ret);
  return ret;
}

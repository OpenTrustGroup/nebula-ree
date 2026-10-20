// SPDX-License-Identifier: NebulaProprietary
/*
* Copyright (c) 2025 MediaTek Inc.
* Copyright 2025 GoldenRiver Technologies Co., Ltd. All rights reserved.
*/

#ifndef TRANSPORT_PROTOCOL_H
#define TRANSPORT_PROTOCOL_H

#include <stdint.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PATH 1024
#define MAX_DATA 65536

typedef struct {
    uint32_t server_cid;
    uint32_t server_port;
} context_t;

typedef enum {
    OP_GETATTR = 1,
    OP_READDIR,
    OP_OPEN,
    OP_READ,
    OP_STATFS,
} opcode_t;

typedef struct {
    uint32_t opcode;
    uint32_t seq;
    uint32_t length;
} request_t;

typedef struct {
    uint32_t opcode;
    uint32_t seq;
    int32_t  result;
    uint32_t length;
} response_t;

typedef struct {
    char path[MAX_PATH];
} getattr_req_t;

struct stat_fixed {
    uint64_t st_dev;
    uint64_t st_ino;
    uint32_t st_mode;
    uint64_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_rdev;
    uint64_t __pad;
    uint64_t st_size;
    int64_t st_blksize;
    int32_t __pad2;
    int64_t st_blocks;
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
    uint32_t __unused[2];
};

typedef struct {
    struct stat_fixed st;
} getattr_resp_t;

typedef struct {
    char path[MAX_PATH];
} readdir_req_t;

typedef struct {
    uint64_t ino;
    uint32_t type;      // DT_REG, DT_DIR
    char name[256];
} dirent_t;

typedef struct {
    char path[MAX_PATH];
    int64_t offset;
    uint32_t size;
} read_req_t;

typedef struct {
    uint32_t size;
    char data[0];
} read_resp_t;

typedef struct {
    uint64_t blocks;
    uint64_t bfree;
    uint64_t bavail;
    uint64_t files;
    uint64_t ffree;
    uint32_t bsize;
    uint32_t namelen;
} statfs_t;

#ifdef __cplusplus
}
#endif

#endif //TRANSPORT_PROTOCOL_H

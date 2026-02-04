/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _LIB_TIPC_H
#define _LIB_TIPC_H

#include <stdint.h>
#include <sys/uio.h>
#include <trusty/trusty_ipc_ioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SHRMEM_COUNT 8

int tipc_connect(const char *dev_name, const char *srv_name);
int tipc_close(int fd);

/* Helper functions to handle memory references */
struct tipc_memref {
    uint32_t shr_mmap;
    uintptr_t shr_base;
    uint32_t shr_size;
    uint32_t data_off;
    uint32_t data_size;
    uint32_t page_size;
    struct tipc_shmem shmem;
};

int tipc_memref_alloc(struct tipc_memref *mr, size_t size, uint32_t flags);
int tipc_memref_alloc2(int fd, struct tipc_memref *mr, size_t size, uint32_t flags);
void tipc_memref_free(struct tipc_memref *mr);

int tipc_memref_init(struct tipc_memref *mr, uint32_t flags,
                     const void *shr_base, uint32_t shr_size,
                     uint32_t data_size, uint32_t data_off, uint32_t page_size);

void tipc_memref_prepare(struct tipc_memref *mr, uint32_t *phsize,
                         uint32_t *phoff, void **aux_pages);

void tipc_memref_finish(const struct tipc_memref *mr, uint32_t size);

/* Wrappers to send/recv messages */
int tipc_send_msg(int fd, const struct iovec *iov, unsigned int iov_cnt,
                  const struct tipc_memref *mrefv, unsigned int mrefv_cnt);

int tipc_recv_msg(int fd, const struct iovec *iov, unsigned int iovcnt);

int tipc_query_ion_handle(int fd, int ion_fd, uint32_t* out_handle);

#ifdef __cplusplus
}
#endif

#endif

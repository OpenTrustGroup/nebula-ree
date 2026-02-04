// Copyright (c) 2021 GoldenRiver Technology Co., Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <errno.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <trusty_uuid.h>
#include <zircon/syscalls/port.h>
#include <zircon/types.h>

__BEGIN_CDECLS;

///////////////////////////////////////////////////////////////////////////////
// syscall/zircon.cc
zx_time_t zx_clock_get(uint32_t clock_id);
zx_status_t zx_nanosleep(zx_time_t deadline);
zx_time_t zx_deadline_after(zx_duration_t nanoseconds);

///////////////////////////////////////////////////////////////////////////////
// syscall/handle.cc
zx_status_t zx_handle_duplicate(zx_handle_t handle_value, zx_rights_t rights, zx_handle_t* out);
zx_status_t zx_handle_replace(zx_handle_t handle_value, zx_rights_t rights, zx_handle_t* out);
zx_status_t zx_handle_close(zx_handle_t handle_value);

///////////////////////////////////////////////////////////////////////////////
// syscall/timer.cc
zx_status_t zx_timer_create(uint32_t options, uint32_t clock_id, zx_handle_t* out);
zx_status_t zx_timer_set(zx_handle_t handle, zx_time_t deadline, zx_duration_t slack);

///////////////////////////////////////////////////////////////////////////////
// syscall/port.cc
zx_status_t zx_port_create(uint32_t options, zx_handle_t* out);
zx_status_t zx_port_queue(zx_handle_t handle, const zx_port_packet_t* in, size_t count);
zx_status_t zx_port_wait(zx_handle_t handle, zx_time_t deadline, zx_port_packet_t* out,
                         size_t count);
zx_status_t zx_port_cancel(zx_handle_t handle, zx_handle_t source, uint64_t key);

///////////////////////////////////////////////////////////////////////////////
// syscall/object.cc
zx_status_t zx_object_wait_one(zx_handle_t handle, zx_signals_t signals, zx_time_t deadline,
                               zx_signals_t* observed);
zx_status_t zx_object_wait_async(zx_handle_t handle, zx_handle_t port_handle, uint64_t key,
                                 zx_signals_t signals, uint32_t options);
zx_status_t zx_object_wait_many(zx_wait_item_t* user_items, size_t count, zx_time_t deadline);
zx_status_t zx_object_signal(zx_handle_t handle, uint32_t clear_mask, uint32_t set_mask);
int zx_object_get_fd(zx_handle_t handle);  // debugging
zx_status_t zx_object_get_cookie(zx_handle_t handle, zx_handle_t hscope, uint64_t* _cookie);
zx_status_t zx_object_set_cookie(zx_handle_t handle, zx_handle_t hscope, uint64_t cookie);
zx_status_t zx_object_signal_peer(zx_handle_t handle_value, uint32_t clear_mask, uint32_t set_mask);
zx_status_t zx_object_get_property(zx_handle_t handle_value, uint32_t property, void* _value,
                                   size_t size);
zx_status_t zx_object_set_property(zx_handle_t handle_value, uint32_t property, const void* _value,
                                   size_t size);
zx_status_t zx_object_get_info(zx_handle_t handle, uint32_t topic, void* _buffer,
                               size_t buffer_size, size_t* _actual, size_t* _avail);
zx_status_t zx_object_get_child(zx_handle_t handle, uint64_t koid, zx_rights_t rights,
                                zx_handle_t* out);

///////////////////////////////////////////////////////////////////////////////
// syscall/event.cc

zx_status_t zx_event_create(uint32_t options, zx_handle_t* out);
zx_status_t zx_event_create_fd(int fd, uint32_t options, zx_handle_t* out);
zx_status_t zx_event_write(zx_handle_t handle_value);

///////////////////////////////////////////////////////////////////////////////
// syscall/trusty.cc
zx_status_t zx_tipc_port_create(const char* _name, size_t name_len, uint32_t flags,
                                zx_handle_t* out);

zx_status_t zx_tipc_port_accept(zx_handle_t handle_value, uuid_t* _uuid, size_t uuid_len,
                                zx_handle_t* out);

zx_status_t zx_tipc_port_connect(const char* _name, size_t name_len, uint32_t flags,
                                 zx_handle_t* out);

zx_status_t zx_tipc_chan_read(zx_handle_t handle_value, uint32_t options, void* bytes,
                              zx_handle_t* handles, uint32_t num_bytes, uint32_t num_handles,
                              uint32_t* actual_bytes, uint32_t* actual_handles);

zx_status_t zx_tipc_chan_write(zx_handle_t handle_value, uint32_t options, const void* bytes,
                               uint32_t num_bytes, const zx_handle_t* user_handles,
                               uint32_t num_handles);

///////////////////////////////////////////////////////////////////////////////
// syscall/unixsock.cc
zx_status_t zx_unix_sock_create(const char* path, zx_handle_t* out);
zx_status_t zx_unix_sock_accept(zx_handle_t handle_value, zx_handle_t* out);
zx_status_t zx_unix_sock_connect(const char* path, zx_handle_t* out);
zx_status_t zx_unix_sock_read(zx_handle_t handle_value, char* buf, int buf_len, int* fds,
                              int max_fds, uint32_t* read_len, uint32_t* fd_num);
zx_status_t zx_unix_sock_write(zx_handle_t handle_value, const char* buf, int buf_len, int* fds,
                               int fd_num, uint32_t* written_len);

__END_CDECLS;
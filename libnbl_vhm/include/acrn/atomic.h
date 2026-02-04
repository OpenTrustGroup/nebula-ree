/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#define ATOMIC_LOAD(ptr) __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
#define ATOMIC_STORE(ptr, val) __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)

#endif /* _ATOMIC_H_ */

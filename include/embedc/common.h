/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _EMBEDC_COMMON_H_
#define _EMBEDC_COMMON_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#if defined(__ZEPHYR__)
#include <zephyr/kernel.h>
#endif /* __ZEPHYR__ */

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* MIN */

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /* MAX */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif /* ARRAY_SIZE */

#ifndef BIT
#define BIT(n) (1u << (n))
#endif /* BIT */

#endif /* _EMBEDC_COMMON_H_ */
/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _EMBEDC_URL_INTERNAL_H_
#define _EMBEDC_URL_INTERNAL_H_

#include <embedc-url/parser.h>

#define GET 		ROUTE_GET
#define POST 		ROUTE_POST
#define PUT 		ROUTE_PUT
#define DELETE 		ROUTE_DELETE
#define METHODS_MASK 	ROUTE_METHODS_MASK

#define ARG_UINT 	ROUTE_ARG_UINT 	
#define ARG_HEX 	ROUTE_ARG_HEX 
#define ARG_STR 	ROUTE_ARG_STR 
#define ARG_MASK 	ROUTE_ARG_MASK 

#define IS_LEAF		ROUTE_IS_LEAF
#define IS_LEAF_MASK	ROUTE_IS_LEAF_MASK

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

#endif /* _EMBEDC_URL_INTERNAL_H_ */
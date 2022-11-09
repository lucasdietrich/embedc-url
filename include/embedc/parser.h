/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _EMBEDC_PARSER_H_
#define _EMBEDC_PARSER_H_

#include <embedc/common.h>

/*____________________________________________________________________________*/

/* HTTP query string parser */

struct query_arg
{
	char *key;
	char *value;
};

/**
 * @brief Parse a query string and store the key-value pairs in the args array
 * 
 * Leading question mark "?" is optional, but arguments will only be
 * parsed after the last question mark found in the string.
 * 
 * @param url 
 * @param qargs 
 * @param alen 
 * @return int 
 */
int query_args_parse(char *url, struct query_arg qargs[], size_t alen);

char *query_arg_get(struct query_arg qargs[], size_t alen, const char *key);

static inline bool query_arg_is_set(const char *key, struct query_arg qargs[], size_t alen)
{
	return query_arg_get(qargs, alen, key) != NULL;
}

/**
 * @brief Parse a query string, but only return the value for the given key
 * 
 * @param url 
 * @param key 
 * @return char* 
 */
char *query_args_parse_find(char *url, const char *key);

/*____________________________________________________________________________*/

/* HTTP routes tree structure, functions and parser */

struct route_part
{
	const char *str;
	size_t len;
};

#define ROUTE_GET		(1u << 0u)
#define ROUTE_POST		(1u << 1u)
#define ROUTE_PUT		(1u << 2u)
#define ROUTE_DELETE		(1u << 3u)
#define ROUTE_METHODS_MASK	(ROUTE_GET | ROUTE_POST | ROUTE_PUT | ROUTE_DELETE)

#define ROUTE_ARG_UINT		(1u << 4u)
#define ROUTE_ARG_HEX		(1u << 5u)
#define ROUTE_ARG_STR		(1u << 6u)
#define ROUTE_ARG_MASK		(ROUTE_ARG_UINT | ROUTE_ARG_HEX | ROUTE_ARG_STR)

#define ROUTE_IS_LEAF		(1u << 7u)
#define ROUTE_IS_LEAF_MASK	(1u << 7u)

struct route_descr
{
	uint32_t flags;

	struct route_part part;
	union {
		struct {
			const struct route_descr *list;
			size_t count;
		} children;
		struct {
			void (*resp_handler)(void);
			void (*req_handler)(void);
		};
	};

	uint32_t user_data;
};

#define LEAF(_p, _fl, _rp, _rq, _u) \
	{ \
		.flags = _fl | IS_LEAF, \
		.part = { \
			.str = _p, \
			.len = sizeof(_p) - 1u, \
		}, \
		.req_handler = (void (*)(void))_rq, \
		.resp_handler = (void (*)(void))_rp, \
		.user_data = (uint32_t)_u, \
	}

#define SECTION(_p, _fl, _ls, _cc, _u) \
	{ \
		.flags = _fl, \
		.part = { \
			.str = _p, \
			.len = sizeof(_p) - 1u, \
		}, \
		.children = { \
			.list = _ls, \
			.count = _cc, \
		}, \
		.user_data = (uint32_t)_u, \
	}

typedef int (*route_parser_cb_t)(struct route_part *s,
				 void *user_data);

int route_parse(char *url,
		route_parser_cb_t cb,
		void *user_data);

/**
 * @brief Callback for route_iterate
 *
 * @param[in] descr Route descriptor
 * @param[in] depth Current depth
 * @param[in] user_data User data
 */
typedef bool (*route_tree_iter_cb_t)(const struct route_descr *descr,
				     const struct route_descr *parents[],
				     size_t depth,
				     void *user_data);

     /**
      * @brief Iterate over routes tree, call callback for each node (leaf or section)
      *
      * @param root Root of routes tree
      * @param size Size of routes tree
      * @param cb Callback to call for each node
      * @param user_data User data to pass to callback
      * @return int Number of routes processed (leaf), negative value on error
      */
int route_tree_iterate(const struct route_descr *root,
		       size_t size,
		       route_tree_iter_cb_t cb,
		       void *user_data);

struct route_parse_result
{
	uint32_t depth;

	const struct route_descr *descr;

	union {
		uint32_t uint;
		int32_t sint;
		char *str;
		void *arg;
	};
};

/**
 * @brief Parse route and fill route_parse_result array
 *
 * @param root Root of routes tree
 * @param size Size of routes tree
 * @param url URL to parse
 * @param results array of route_parse_result
 * @param results_size size of results array
 * @return const struct route_descr*
 */
const struct route_descr *route_tree_resolve(const struct route_descr *root,
					     size_t size,
					     char *url,
					     uint32_t flags,
					     uint32_t mask,
					     struct route_parse_result *results,
					     size_t *results_count,
					     char **query_string);

const int route_build_url(char *url,
			  size_t url_size,
			  const struct route_descr **parents,
			  size_t count);

int route_results_find_arg(const struct route_parse_result *results,
			  size_t count,
			  const struct route_descr *search,
			   uint32_t arg_flags,
			   void **arg);

int route_results_get_arg(const struct route_parse_result *results,
			  size_t count,
			  uint32_t index,
			  uint32_t arg_flags,
			  void **arg);

static inline int
route_results_find_uint(const struct route_parse_result *results,
			size_t count,
			const struct route_descr *search,
			uint32_t *uint)
{
	return route_results_find_arg(
		results, count, search, ROUTE_ARG_UINT, (void **)uint);
}

static inline int
route_results_find_hex(const struct route_parse_result *results,
		       size_t count,
		       const struct route_descr *search,
		       char *hex)
{
	return route_results_find_arg(
		results, count, search, ROUTE_ARG_HEX, (void **)hex);
}

static inline int
route_results_find_number(const struct route_parse_result *results,
			  size_t count,
			  const struct route_descr *search,
			  uint32_t *n)
{
	return route_results_find_arg(
		results,
		count,
		search,
		ROUTE_ARG_UINT | ROUTE_ARG_HEX,
		(void **)n
	);
}

static inline int
route_results_find_str(const struct route_parse_result *results,
		       size_t count,
		       const struct route_descr *search,
		       char *str)
{
	return route_results_find_arg(
		results, count, search, ROUTE_ARG_STR, (void **)str);
}

static inline int
route_results_get_uint(const struct route_parse_result *results,
		       size_t count,
		       uint32_t index,
		       uint32_t *uint)
{
	return route_results_get_arg(
		results, count, index, ROUTE_ARG_UINT, (void **)uint);
}

static inline int
route_results_get_hex(const struct route_parse_result *results,
		      size_t count,
		      uint32_t index,
		      char *hex)
{
	return route_results_get_arg(
		results, count, index, ROUTE_ARG_HEX, (void **)hex);
}

static inline int
route_results_get_number(const struct route_parse_result *results,
			 size_t count,
			 uint32_t index,
			 uint32_t *n)
{
	return route_results_get_arg(
		results,
		count,
		index,
		ROUTE_ARG_UINT | ROUTE_ARG_HEX,
		(void **)n
	);
}

static inline int
route_results_get_str(const struct route_parse_result *results,
		      size_t count,
		      uint32_t index,
		      char *str)
{
	return route_results_get_arg(
		results, count, index, ROUTE_ARG_STR, (void **)str);
}

#endif /* _EMBEDC_PARSER_H_ */
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

#define GET		(1u << 0u)
#define POST		(1u << 1u)
#define PUT		(1u << 2u)
#define DELETE		(1u << 3u)

#define METHODS_MASK	(GET | POST | PUT | DELETE)

#define ARG_INT		(1u << 11u)
#define ARG_HEX		(1u << 12u)
#define ARG_STR		(1u << 13u)
#define ARG_UINT	(1u << 14u)

#define STREAM		(1u << 5u)

#define CONTENT_TYPE_TEXT_PLAIN 		(1 << 6u)
#define CONTENT_TYPE_TEXT_HTML 			(1 << 7u)
#define CONTENT_TYPE_APPLICATION_JSON 		(1 << 8u)
#define CONTENT_TYPE_MULTIPART_FORM_DATA 	(1 << 9u)
#define CONTENT_TYPE_APPLICATION_OCTET_STREAM 	(1 << 10u)   

#define IS_LEAF		(1u << 31u)

#define IS_LEAF_MASK	(1u << 31u)

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
};

#define LEAF(_p, _fl, _rq, _rp) \
	{ \
		.flags = _fl | IS_LEAF, \
		.part = { \
			.str = _p, \
			.len = sizeof(_p) - 1u, \
		}, \
		.req_handler = _rq, \
		.resp_handler = _rp, \
	}

#define SECTION(_p, _fl, _ls, _cc) \
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
					     size_t *results_count);

#endif /* _EMBEDC_PARSER_H_ */
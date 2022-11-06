/*
 * Copyright (c) 2022 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <embedc/parser.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <embedc/parser_internal.h>

#ifndef CONFIG_EMBEDC_UTILS_ROUTES_ITER_MAX_DEPTH
#define CONFIG_EMBEDC_UTILS_ROUTES_ITER_MAX_DEPTH 10u
#endif /* CONFIG_EMBEDC_UTILS_ROUTES_ITER_MAX_DEPTH */

int query_args_parse(char *url, struct query_arg qargs[], size_t alen)
{
	if (!url || (!qargs && alen))
		return -EINVAL;
	
	size_t count = 0u;

	/* Temporary query argument.
	 * To keep track of current state if qargs is not long enough */
	struct query_arg zarg;
	struct query_arg *arg = &qargs[0u];
	arg->key = url;
	arg->value = NULL;

	char *chr = url;

	do {
		switch (*chr) {
		case '?':
			/* Reset parsing */
			if (count) {
				arg = &qargs[0u];
				arg->value = NULL;
				count = 0u;
			}

			*chr = '\0';
			arg->key = chr + 1u;
			break;
		case '&':
			*chr = '\0';
			/* If previous argument is not valid, ignore it */
			if (!arg || *arg->key != '\0') {
				count++;
				arg = (count < alen) ? &qargs[count] : &zarg;
				arg->value = NULL; /* In case no value is found */
			}
			arg->key = chr + 1u;
			break;
		case '=':
			*chr = '\0';
			if (arg) {
				arg->value = chr + 1u;
			}
			break;
		default:
			break;
		}

		chr++;
	} while (*chr != '\0');

	/* If last argument is not empty, add it */
	if (arg && *arg->key != '\0') {
		count++;
	}

	return (int)count;
}

/* TODO improve this: don't directly use query_args_parse
 * but try to parse the string with the same method
 * but trying to find the key
 */
char *query_args_parse_find(char *url, const char *key)
{
	struct query_arg qargs[10u];
	char *value = NULL;

	int count = query_args_parse(url, qargs, ARRAY_SIZE(qargs));
	if (count > 0) {
		value = query_arg_get(qargs, MIN(count, (int)ARRAY_SIZE(qargs)), key);
	}

	return value;
}

char *query_arg_get(struct query_arg qargs[], size_t alen, const char *key)
{
	if (!qargs || !key)
		return NULL;

	for (size_t i = 0u; i < alen; i++) {
		struct query_arg *const arg = &qargs[i];
		if (arg->key && strcmp(arg->key, key) == 0) {
			return arg->value;
		}
	}

	return NULL;
}


int route_parse(char *url,
		route_parser_cb_t cb,
		void *user_data)
{
	if (!url || !cb)
		return -EINVAL;

	char *p = url;

	int ret = 0;
	bool zcontinue = true;
	struct route_part s;

	/* Remove leading '/' */
	while (*p == '/')
		p++;

	s.str = p;
	s.len = 0u;

	while (zcontinue) {
		switch (*p) {

		case '?':
		case '\0':
			zcontinue = false;
		case '/':
			*p = '\0'; /* Slice the route */
			zcontinue &= (ret = cb(&s, user_data)) == 0;
			s.str = p + 1u;
			s.len = 0u;
			break;
		default:
			s.len++;
			break;
		}
		p++;
	}

	/* Return number of chars parsed */
	return ret ? ret : p - url;
}

static inline bool is_leaf(const struct route_descr *descr)
{
	return (descr->flags & ROUTE_IS_LEAF_MASK) == ROUTE_IS_LEAF;
}

int route_tree_iterate(const struct route_descr *root,
		       size_t size,
		       route_tree_iter_cb_t cb,
		       void *user_data)
{
	if (!root || !size || !cb)
		return -EINVAL;

	const struct route_descr *parents[CONFIG_EMBEDC_UTILS_ROUTES_ITER_MAX_DEPTH];
	size_t depth = 0u;
	size_t routes_count = 0u;

	const struct route_descr *first = root;
	const struct route_descr *node = first;
	size_t children_count = size;

	while (node) {
		if (cb(node, parents, depth, user_data) == false)
			goto exit;

		if (is_leaf(node)) {
			routes_count++;

			const size_t index = node - first;
			if (index < children_count - 1u) {
				node++;
				continue;
			}

			do {
				if (depth == 0u)
					goto exit;

				node = parents[--depth] + 1u;
				if (depth == 0) {
					first = root;
					children_count = size;
				} else {
					first = parents[depth - 1u]->children.list;
					children_count = parents[depth - 1u]->children.count;
				}
			} while (node - first >= (int)children_count);
		} else {
			if (depth >= CONFIG_EMBEDC_UTILS_ROUTES_ITER_MAX_DEPTH)
				return -EOVERFLOW;

			parents[depth++] = node;
			children_count = node->children.count;
			first = node->children.list;
			node = first;
		}
	}

exit:
	return routes_count;
}

static bool route_part_parse(const struct route_descr *node,
			     const struct route_part *part,
			     void *arg)
{
	if (node->flags & ARG_HEX) {
		return sscanf(part->str, "%x", (unsigned int *)arg) == 1;
	} else if (node->flags & ARG_UINT) {
		return sscanf(part->str, "%u", (unsigned int *)arg) == 1;
	} else if (node->flags & ARG_STR) {
		*(const char **)arg = part->str;
		return true;
	} else {
		if (node->part.len != part->len)
			return false;

		if (strncmp(node->part.str, part->str, part->len))
			return false;

		*(const char **)arg = part->str;
	}

	return true;
}

struct resolve_context {
	/**
	 * @brief Last descriptor being matched
	 */
	const struct route_descr *descr;

	/**
	 * @brief Children count of the last section being matched
	 * 
	 * If route has been found, this value is 0. It can be used to know
	 * whether we expect more parts in the route or not.
	 */
	size_t child_count;

	/**
	 * @brief Current memory block to store matched route part.
	 * 
	 * Note: Should be initialized to the beginning of an array
	 */
	struct route_parse_result *result;

	/**
	 * @brief Remaining number of elements
	 * 
	 * Note: Should be initialized  to the size of the array
	 */
	size_t results_remaining;

	/**
	 * Flags value to match
	 */
	uint32_t flags;

	/**
	 * @brief Flags mask to match
	 */
	uint32_t mask;
};

static inline bool route_found(struct resolve_context *x)
{
	return x->child_count == 0u;
}

static inline void mark_route_found(struct resolve_context *x)
{
	x->child_count = 0u;
}

static inline bool node_matches_flags(const struct route_descr *descr,
				      uint32_t flags,
				      uint32_t mask)
{
	return (descr->flags & mask) == (flags & mask);
}

static int route_tree_resolve_cb(struct route_part *p,
				 void *user_data)
{
	struct resolve_context *x = user_data;

	/* If we found the route but there is more, then we return an error
	 * In order to ignore trailing '/', we just ignore empty parts
	 */
	if ((p->len != 0u) && (route_found(x) == true)) {
		return -ENOENT;
	}

	if (!x->result) {
		return -ENOMEM;
	}

	const struct route_descr *node;
	for (node = x->descr; node < x->descr + x->child_count; node++) {
		if (route_part_parse(node, p, &x->result->arg) == true) {
			bool match = false;

			if (is_leaf(node)) {

				/* Leaf flags should match */
				if (node_matches_flags(node, x->flags, x->mask)) {
					x->descr = node;
					mark_route_found(x);
					match = true;
				}
			} else {
				/* Prepare context for next call */
				x->descr = node->children.list;
				x->child_count = node->children.count;
				match = true;
			}

			if (match) {
				x->result->descr = node;
				x->result = x->results_remaining-- > 0u ? x->result + 1u : NULL;
				return 0;
			}
		}
	}

	return -ENOENT;
}

static const struct route_descr *
find_section_leaf(const struct route_descr *section_first_child,
		  size_t count,
		  uint32_t flags,
		  uint32_t mask)
{
	const struct route_descr *leaf = NULL;

	/* Search for leaf */
	flags |= ROUTE_IS_LEAF;
	mask |= ROUTE_IS_LEAF;

	for (const struct route_descr *node = section_first_child;
	     node < section_first_child + count;
	     node++) {
		/* Find unamed leaf which matches flags */
		if (!node->part.len && node_matches_flags(node, flags, mask)) {
			leaf = node;
			break;
		}
	}

	return leaf;
}

const struct route_descr *route_tree_resolve(const struct route_descr *root,
					     size_t size,
					     char *url,
					     uint32_t flags,
					     uint32_t mask,
					     struct route_parse_result *results,
					     size_t *results_count,
					     char **query_string)
{
	int ret;
	const struct route_descr *leaf = NULL;

	if (!root || !size || !url || !results || !results_count || !*results_count)
		goto exit;

	struct resolve_context x = {
		.descr = root,
		.child_count = size,
		.result = &results[0u],
		.results_remaining = *results_count,
		.flags = flags,
		.mask = mask,
	};

	ret = route_parse(url, route_tree_resolve_cb, &x);

	if ((ret >= 0) && x.descr) {
		if (is_leaf(x.descr) && route_found(&x)) {
			leaf = x.descr;
		} else {
			/**
			 * @brief If we end up on a section, we need to find the
			 * unamed leaf which matches the flags.
			 */
			leaf = find_section_leaf(x.descr, x.child_count, flags, mask);
			if (!x.result) {
				leaf = NULL;
			} else if (leaf) {
				x.result->descr = leaf;
				x.result->str = url + (uint32_t)ret - 1u;
				x.results_remaining--;
			}
		}
	}

	if (leaf) {
		*results_count -= x.results_remaining;

		/* ret necessarily positive at this point */
		if (query_string) {
			*query_string = url + (uint32_t)ret;
		}
	} else {
		*results_count = 0u;
	}

exit:
	return leaf;
}

const int route_build_url(char *url,
			  size_t url_size,
			  const struct route_descr **parents,
			  size_t count)
{
	int ret = -EINVAL;

	if (!url || !url_size)
		goto exit;

	if (count && !parents)
		goto exit;

	url[0u] = '/';
	size_t len = 1u;

	for (size_t i = 0u; i < count; i++) {
		if (parents[i]->part.len > url_size - len - 1u) {
			ret = -ENOMEM;
			goto exit;
		}

		memcpy(&url[len], parents[i]->part.str, parents[i]->part.len);
		len += parents[i]->part.len;
		url[len++] = '/';
	}
	url[len] = '\0';
	ret = (int)len;

exit:
	return ret;
}
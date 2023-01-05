#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <errno.h>

#include <embedc-url/parser.h>
#include <embedc-url/parser_internal.h>

#include "routes.h"

int main(void)
{
	struct route_parse_result results[10u];

	char urls[][128u] = {
		"/test/route_args/23/1/24?test=sdf&fdsf=826h&name=Lucas&&qsfd=86",
		"/credentials/flash?q=23",
		"/devices/caniot/12/endpoint/blc/command",
		"/test/route_args/24232312/-1/43",
		"/devices/caniot/23/attribute/EEff",
		"/test/customSTR/mystr",
		"/test/customSTR/azer/qsd",
		"/files?x=23",
	};

	for (uint32_t i = 0; i < ARRAY_SIZE(results); i++) {
		char *url = urls[i];
		printf("\nP url=%s\n", url);

		size_t results_count = ARRAY_SIZE(results);
		const struct route_descr *leaf =
			route_tree_resolve(routes_root, routes_root_size, url,
					   GET, METHODS_MASK,
					   results, &results_count,
					   NULL);

		printf("leaf=%p (%s)\n", (void *)leaf, leaf ? leaf->part.str : "");

		for (size_t i = 0; i < results_count; i++) {
			if (results[i].descr) {
				printf("results[%lu] = %s\n", i, results[i].descr->part.str);
				if (results[i].descr->flags & ARG_UINT) {
					printf("\tuint=%u\n", results[i].uint);
				} else if (results[i].descr->flags & ARG_HEX) {
					printf("\thex=%x\n", results[i].uint);
				} else if (results[i].descr->flags & ARG_STR) {
					printf("\tstring=%s\n", results[i].str);
				}
			}
		}
	}

	char query_strs[][100u] = {
		"?&&&qsfd",
		"?test&fdsf&&&qsfd",
		"abc?def=ghi&jkl=mno",
		"??abc?def=ghi&jkl=mno",
		"abc?def&&jkl=mno&&=23&&",
		"abc?def&&=mno&",
		"?=&=&===&",
		"aze=23&a=2",
		"a=1&b=2?c=3&d=4",
	};

	for (uint32_t i = 0; i < ARRAY_SIZE(query_strs); i++) {
		char *url = query_strs[i];
		printf("\nQ url=%s\n", url);

		char tmp[100u];
		memcpy(tmp, url, 100u);
		struct query_arg qargs[4u];
		memset(qargs, 0, sizeof(qargs));
		int count = query_args_parse(url, qargs, ARRAY_SIZE(qargs));
		printf("parse_url_query_arg(\"%s\", qargs, %lu) = %d\n",
		       tmp, ARRAY_SIZE(qargs), count);

		for (int i = 0; i < count; i++) {
			printf("%s = %s\n", qargs[i].key, qargs[i].value);
		}
	}

	return 0;
}
#include <stdio.h>
#include <memory.h>

#include <embedc-url/parser.h>

char tests[][100u] = {
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

static void test(char *url)
{
	#define QARGS_SIZE 4

	char tmp[100u];
	memcpy(tmp, url, 100u);
	struct query_arg qargs[QARGS_SIZE];
	memset(qargs, 0, sizeof(qargs));
	int count = query_args_parse(url, qargs, QARGS_SIZE);
	printf("parse_url_query_arg(\"%s\", qargs, %u) = %d\n", 
	       tmp, QARGS_SIZE, count);

	for (int i = 0; i < count; i++) {
		printf("%s = %s\n", qargs[i].key, qargs[i].value);
	}
}

void test_query(void)
{
	for (uint32_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		test(tests[i]);
	}
}
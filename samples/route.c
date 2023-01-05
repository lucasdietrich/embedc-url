#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <errno.h>

#include <common.h>

#include <embedc-url/parser.h>
#include <embedc-url/parser_internal.h>

void web_server_index_html(void){}
void web_server_files_html(void){}
void rest_info(void){}
void rest_flash_credentials_list(void){}
void prometheus_metrics(void){}
void prometheus_metrics_controller(void){}
void prometheus_metrics_demo(void){}
void rest_devices_list(void){}
void rest_room_devices_list(void){}
void rest_xiaomi_records(void){}
void rest_caniot_records(void){}
void rest_ha_stats(void){}
void http_file_upload(void){}
void http_file_download(void){}
void rest_fs_list_lua_scripts(void){}
void rest_fs_remove_lua_script(void){}
void rest_lua_run_script(void){}
void rest_demo_json(void){}
void http_dfu_image_upload_response(void){}
void http_dfu_image_upload(void){}
void http_dfu_status(void){}
void rest_devices_garage_get(void){}
void rest_devices_garage_post(void){}
void rest_devices_caniot_blc0_command(void){}
void rest_devices_caniot_blc1_command(void){}
void rest_devices_caniot_blc_command(void){}
void rest_devices_caniot_telemetry(void){}
void rest_devices_caniot_command(void){}
void rest_devices_caniot_attr_read_write(void){}
void rest_if_can(void){}
void http_test_messaging(void){}
void http_test_streaming(void){}
void http_test_echo(void){}
void http_test_big_payload(void){}
void http_test_headers(void){}
void http_test_payload(void){}

/* ROUTES BEGIN

GET / -> web_server_index_html
GET /index.html -> web_server_index_html
GET /fetch -> web_server_files_html
GET /info -> rest_info
GET /credentials/flash -> rest_flash_credentials_list (CONFIG_CREDS_FLASH)
GET /metrics -> prometheus_metrics
GET /metrics_controller -> prometheus_metrics_controller
GET /metrics_demo -> prometheus_metrics_demo
GET /devices/ -> rest_devices_list
POST /devices/ -> rest_devices_list
GET /room/:u -> rest_room_devices_list
GET /devices/xiaomi -> rest_xiaomi_records
GET /devices/caniot -> rest_caniot_records
GET /ha/stats -> rest_ha_stats
POST /files -> http_file_upload, http_file_upload
GET /files -> http_file_download
GET /files/lua -> rest_fs_list_lua_scripts
DELETE /files/lua -> rest_fs_remove_lua_script
POST /lua/execute -> rest_lua_run_script
GET /demo/json -> rest_demo_json
POST /dfu -> http_dfu_image_upload, http_dfu_image_upload_response (CONFIG_DFU)
GET /dfu -> http_dfu_status (CONFIG_DFU)
GET /devices/garage -> rest_devices_garage_get (CONFIG_CANIOT_CONTROLLER)
POST /devices/garage -> rest_devices_garage_post (CONFIG_CANIOT_CONTROLLER)
POST /devices/caniot/:u/endpoint/blc0/command -> rest_devices_caniot_blc0_command (CONFIG_CANIOT_CONTROLLER)
POST /devices/caniot/:u/endpoint/blc1/command -> rest_devices_caniot_blc1_command (CONFIG_CANIOT_CONTROLLER)
POST /devices/caniot/:u/endpoint/blc/command -> rest_devices_caniot_blc_command (CONFIG_CANIOT_CONTROLLER)
GET /devices/caniot/:u/endpoint/:u/telemetry -> rest_devices_caniot_telemetry (CONFIG_CANIOT_CONTROLLER)
POST /devices/caniot/:u/endpoint/:u/command -> rest_devices_caniot_command (CONFIG_CANIOT_CONTROLLER)
GET /devices/caniot/:u/attribute/:x -> rest_devices_caniot_attr_read_write (CONFIG_CANIOT_CONTROLLER)
PUT /devices/caniot/:u/attribute/:x -> rest_devices_caniot_attr_read_write (CONFIG_CANIOT_CONTROLLER)
POST /if/can/:x -> rest_if_can (CONFIG_CAN_INTERFACE)
POST /test/messaging -> http_test_messaging (CONFIG_HTTP_TEST_SERVER)
POST /test/streaming -> http_test_streaming (CONFIG_HTTP_TEST_SERVER)
POST /test/route_args/:u/:u/:u -> http_test_echo (CONFIG_HTTP_TEST_SERVER)
POST /test/big_payload -> http_test_big_payload (CONFIG_HTTP_TEST_SERVER)
GET /test/headers -> http_test_headers (CONFIG_HTTP_TEST_SERVER)
GET /test/payload -> http_test_payload (CONFIG_HTTP_TEST_SERVER)
GET /test/:s/mystr -> http_test_payload (CONFIG_HTTP_TEST_SERVER)

ROUTES END */

/* ROUTES DEF BEGIN */
#if defined(CONFIG_HTTP_TEST_SERVER)
static const struct route_descr root_test_zs[] = {
	LEAF("mystr", GET, http_test_payload, NULL, 0u),
};
#endif

#if defined(CONFIG_HTTP_TEST_SERVER)
static const struct route_descr root_test_route_args_zu_zu[] = {
	LEAF(":u", POST | ARG_UINT, http_test_echo, NULL, 0u),
};
#endif

#if defined(CONFIG_HTTP_TEST_SERVER)
static const struct route_descr root_test_route_args_zu[] = {
	SECTION(":u", ARG_UINT, root_test_route_args_zu_zu, 
		ARRAY_SIZE(root_test_route_args_zu_zu), 0u),
};
#endif

#if defined(CONFIG_HTTP_TEST_SERVER)
static const struct route_descr root_test_route_args[] = {
	SECTION(":u", ARG_UINT, root_test_route_args_zu, 
		ARRAY_SIZE(root_test_route_args_zu), 0u),
};
#endif

#if defined(CONFIG_HTTP_TEST_SERVER)
static const struct route_descr root_test[] = {
	LEAF("messaging", POST, http_test_messaging, NULL, 0u),
	LEAF("streaming", POST, http_test_streaming, NULL, 0u),
	SECTION("route_args", 0u, root_test_route_args, 
		ARRAY_SIZE(root_test_route_args), 0u),
	LEAF("big_payload", POST, http_test_big_payload, NULL, 0u),
	LEAF("headers", GET, http_test_headers, NULL, 0u),
	LEAF("payload", GET, http_test_payload, NULL, 0u),
	SECTION(":s", ARG_STR, root_test_zs, 
		ARRAY_SIZE(root_test_zs), 0u),
};
#endif

#if defined(CONFIG_CAN_INTERFACE)
static const struct route_descr root_if_can[] = {
	LEAF(":x", POST | ARG_HEX, rest_if_can, NULL, 0u),
};
#endif

#if defined(CONFIG_CAN_INTERFACE)
static const struct route_descr root_if[] = {
	SECTION("can", 0u, root_if_can, 
		ARRAY_SIZE(root_if_can), 0u),
};
#endif

static const struct route_descr root_demo[] = {
	LEAF("json", GET, rest_demo_json, NULL, 0u),
};

static const struct route_descr root_lua[] = {
	LEAF("execute", POST, rest_lua_run_script, NULL, 0u),
};

static const struct route_descr root_files[] = {
	LEAF("", POST, http_file_upload, http_file_upload, 0u),
	LEAF("", GET, http_file_download, NULL, 0u),
	LEAF("lua", GET, rest_fs_list_lua_scripts, NULL, 0u),
	LEAF("lua", DELETE, rest_fs_remove_lua_script, NULL, 0u),
};

static const struct route_descr root_ha[] = {
	LEAF("stats", GET, rest_ha_stats, NULL, 0u),
};

#if defined(CONFIG_CANIOT_CONTROLLER)
static const struct route_descr root_devices_caniot_zu_attribute[] = {
	LEAF(":x", GET | ARG_HEX, rest_devices_caniot_attr_read_write, NULL, 0u),
	LEAF(":x", PUT | ARG_HEX, rest_devices_caniot_attr_read_write, NULL, 0u),
};
#endif

#if defined(CONFIG_CANIOT_CONTROLLER)
static const struct route_descr root_devices_caniot_zu_endpoint_zu[] = {
	LEAF("telemetry", GET, rest_devices_caniot_telemetry, NULL, 0u),
	LEAF("command", POST, rest_devices_caniot_command, NULL, 0u),
};
#endif

#if defined(CONFIG_CANIOT_CONTROLLER)
static const struct route_descr root_devices_caniot_zu_endpoint_blc[] = {
	LEAF("command", POST, rest_devices_caniot_blc_command, NULL, 0u),
};
#endif

#if defined(CONFIG_CANIOT_CONTROLLER)
static const struct route_descr root_devices_caniot_zu_endpoint_blc1[] = {
	LEAF("command", POST, rest_devices_caniot_blc1_command, NULL, 0u),
};
#endif

#if defined(CONFIG_CANIOT_CONTROLLER)
static const struct route_descr root_devices_caniot_zu_endpoint_blc0[] = {
	LEAF("command", POST, rest_devices_caniot_blc0_command, NULL, 0u),
};
#endif

#if defined(CONFIG_CANIOT_CONTROLLER)
static const struct route_descr root_devices_caniot_zu_endpoint[] = {
	SECTION("blc0", 0u, root_devices_caniot_zu_endpoint_blc0, 
		ARRAY_SIZE(root_devices_caniot_zu_endpoint_blc0), 0u),
	SECTION("blc1", 0u, root_devices_caniot_zu_endpoint_blc1, 
		ARRAY_SIZE(root_devices_caniot_zu_endpoint_blc1), 0u),
	SECTION("blc", 0u, root_devices_caniot_zu_endpoint_blc, 
		ARRAY_SIZE(root_devices_caniot_zu_endpoint_blc), 0u),
	SECTION(":u", ARG_UINT, root_devices_caniot_zu_endpoint_zu, 
		ARRAY_SIZE(root_devices_caniot_zu_endpoint_zu), 0u),
};
#endif

#if defined(CONFIG_CANIOT_CONTROLLER)
static const struct route_descr root_devices_caniot_zu[] = {
	SECTION("endpoint", 0u, root_devices_caniot_zu_endpoint, 
		ARRAY_SIZE(root_devices_caniot_zu_endpoint), 0u),
	SECTION("attribute", 0u, root_devices_caniot_zu_attribute, 
		ARRAY_SIZE(root_devices_caniot_zu_attribute), 0u),
};
#endif

#if defined(CONFIG_CANIOT_CONTROLLER)
static const struct route_descr root_devices_caniot[] = {
	LEAF("", GET, rest_caniot_records, NULL, 0u),
	SECTION(":u", ARG_UINT, root_devices_caniot_zu, 
		ARRAY_SIZE(root_devices_caniot_zu), 0u),
};
#endif

static const struct route_descr root_devices[] = {
	LEAF("", GET, rest_devices_list, NULL, 0u),
	LEAF("", POST, rest_devices_list, NULL, 0u),
	LEAF("xiaomi", GET, rest_xiaomi_records, NULL, 0u),
#if defined(CONFIG_CANIOT_CONTROLLER)
	LEAF("garage", GET, rest_devices_garage_get, NULL, 0u),
#endif
#if defined(CONFIG_CANIOT_CONTROLLER)
	LEAF("garage", POST, rest_devices_garage_post, NULL, 0u),
#endif
#if defined(CONFIG_CANIOT_CONTROLLER)
	SECTION("caniot", 0u, root_devices_caniot, 
		ARRAY_SIZE(root_devices_caniot), 0u),
#endif
};

static const struct route_descr root_room[] = {
	LEAF(":u", GET | ARG_UINT, rest_room_devices_list, NULL, 0u),
};

#if defined(CONFIG_CREDS_FLASH)
static const struct route_descr root_credentials[] = {
	LEAF("flash", GET, rest_flash_credentials_list, NULL, 0u),
};
#endif

static const struct route_descr root[] = {
	LEAF("", GET, web_server_index_html, NULL, 0u),
	LEAF("index.html", GET, web_server_index_html, NULL, 0u),
	LEAF("fetch", GET, web_server_files_html, NULL, 0u),
	LEAF("info", GET, rest_info, NULL, 0u),
#if defined(CONFIG_CREDS_FLASH)
	SECTION("credentials", 0u, root_credentials, 
		ARRAY_SIZE(root_credentials), 0u),
#endif
	LEAF("metrics", GET, prometheus_metrics, NULL, 0u),
	LEAF("metrics_controller", GET, prometheus_metrics_controller, NULL, 0u),
	LEAF("metrics_demo", GET, prometheus_metrics_demo, NULL, 0u),
	SECTION("room", 0u, root_room, 
		ARRAY_SIZE(root_room), 0u),
	SECTION("devices", 0u, root_devices, 
		ARRAY_SIZE(root_devices), 0u),
	SECTION("ha", 0u, root_ha, 
		ARRAY_SIZE(root_ha), 0u),
	SECTION("files", 0u, root_files, 
		ARRAY_SIZE(root_files), 0u),
	SECTION("lua", 0u, root_lua, 
		ARRAY_SIZE(root_lua), 0u),
	SECTION("demo", 0u, root_demo, 
		ARRAY_SIZE(root_demo), 0u),
#if defined(CONFIG_DFU)
	LEAF("dfu", POST, http_dfu_image_upload, http_dfu_image_upload_response, 0u),
#endif
#if defined(CONFIG_DFU)
	LEAF("dfu", GET, http_dfu_status, NULL, 0u),
#endif
#if defined(CONFIG_CAN_INTERFACE)
	SECTION("if", 0u, root_if, 
		ARRAY_SIZE(root_if), 0u),
#endif
#if defined(CONFIG_HTTP_TEST_SERVER)
	SECTION("test", 0u, root_test, 
		ARRAY_SIZE(root_test), 0u),
#endif
};

/* ROUTES DEF END */

/* https://stackoverflow.com/questions/12447557/can-we-have-recursive-macros */

bool route_parser_cb(struct route_part *s, void *user_data)
{
	char url[256];
	memcpy(url, s->str, s->len);
	url[s->len] = '\0';

	printf("%s [len=%lu] s=%p u=%p\n", 
	       url, s->len, (void *)s, user_data);

	return true;
}


bool route_descr_parser_cb(const struct route_descr *descr,
			   size_t depth,
			   void *user_data)
{
	ARG_UNUSED(user_data);

	for (size_t i = 0; i < depth; i++) {
		printf("\t");
	}

	if (descr->flags & IS_LEAF) {
		printf("/%s (%x)\n", descr->part.str, descr->flags);
	} else {
		printf("/%s\n", descr->part.str);
	}

	return true;
}

/*
Both "" and "/" should match the root route.
Both "/test/" and "/test" should match "/test"
*/
void test_route(void)
{
	struct route_parse_result results[10u];


	// char *url = "/test/route_args/23/1/24?test=sdf&fdsf=826h&name=Lucas&&qsfd=86";
	// char url[] = "/credentials/flash?q=23";
	// char url[] = "/devices/caniot/12/endpoint/blc/command";
	// char url[] = "/test/route_args/24232312/-1/43";
	// char url[] = "/devices/caniot/23/attribute/EEff";
	// char url[] = "/test/customSTR/mystr";
	// char url[] = "/test/customSTR/azer/qsd";
	char url[] = "/files?x=23";

	// int ret;
	// ret = route_parse(url, route_parser_cb, NULL);
	// ret = route_tree_iterate(root, ARRAY_SIZE(root), route_descr_parser_cb, NULL);
	// printf("ret=%d\n", ret);

	size_t results_count = ARRAY_SIZE(results);
	const struct route_descr *leaf =
		route_tree_resolve(root, ARRAY_SIZE(root), url,
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
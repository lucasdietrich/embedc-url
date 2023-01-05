#include <embedc-url/parser.h>
#include <embedc-url/parser_internal.h>

#include "routes.h"

#define CONFIG_CREDS_FLASH
#define CONFIG_CANIOT_CONTROLLER
#define CONFIG_HTTP_TEST_SERVER
#define CONFIG_CAN_INTERFACE

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

const struct route_descr *const routes_root = root;
const size_t routes_root_size = ARRAY_SIZE(root);
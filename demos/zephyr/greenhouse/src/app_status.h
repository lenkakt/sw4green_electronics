/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_STATUS_H_
#define APP_STATUS_H_

#include <stdbool.h>
#include <zephyr/zbus/zbus.h>

enum app_module {
	APP_MODULE_SENSOR,
	APP_MODULE_CONTROL,
	APP_MODULE_ACTUATOR,
	APP_MODULE_NET,
	APP_MODULE_BACKEND,
	APP_MODULE_UPDATE,
	APP_MODULE_LED,
	APP_MODULE_WATCHDOG,
	APP_MODULE_COUNT,
};

enum app_status_value {
	APP_STATUS_DEBUG,	/* not started or not configured */
	APP_STATUS_OK,
	APP_STATUS_ERROR,
};

struct app_status_event {
	enum app_module module;
	enum app_status_value value;
};

ZBUS_CHAN_DECLARE(app_status_chan);

void app_status_set(enum app_module module, enum app_status_value value);
enum app_status_value app_status_get(enum app_module module);
const char *app_status_module_name(enum app_module module);
const char *app_status_value_name(enum app_status_value value);

/* True when no module reports an error. */
bool app_status_all_ok(void);

#endif /* APP_STATUS_H_ */

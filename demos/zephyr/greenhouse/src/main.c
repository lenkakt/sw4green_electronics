/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_actuator.h"
#include "app_backend.h"
#include "app_control.h"
#include "app_led.h"
#include "app_net.h"
#include "app_sensor.h"
#include "app_status.h"
#include "app_update.h"
#include "app_watchdog.h"

#include <app_version.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(app_main, CONFIG_APP_MAIN_LOG_LEVEL);

ZBUS_CHAN_DEFINE(app_status_chan, struct app_status_event, NULL, NULL,
		 ZBUS_OBSERVERS_EMPTY, ZBUS_MSG_INIT(0));

static atomic_t status[APP_MODULE_COUNT];

static const char *const module_names[APP_MODULE_COUNT] = {
	[APP_MODULE_SENSOR] = "sensor",
	[APP_MODULE_CONTROL] = "control",
	[APP_MODULE_ACTUATOR] = "actuator",
	[APP_MODULE_NET] = "network",
	[APP_MODULE_BACKEND] = "backend",
	[APP_MODULE_UPDATE] = "update",
	[APP_MODULE_LED] = "led",
	[APP_MODULE_WATCHDOG] = "watchdog",
};

void app_status_set(enum app_module module, enum app_status_value value)
{
	if (atomic_set(&status[module], value) == value) {
		return;
	}

	struct app_status_event ev = {.module = module, .value = value};

	if (value == APP_STATUS_ERROR) {
		LOG_ERR("%s: %s", module_names[module], app_status_value_name(value));
	} else {
		LOG_INF("%s: %s", module_names[module], app_status_value_name(value));
	}
	zbus_chan_pub(&app_status_chan, &ev, K_MSEC(100));
}

enum app_status_value app_status_get(enum app_module module)
{
	return atomic_get(&status[module]);
}

const char *app_status_module_name(enum app_module module)
{
	return module_names[module];
}

const char *app_status_value_name(enum app_status_value value)
{
	switch (value) {
	case APP_STATUS_OK:
		return "ok";
	case APP_STATUS_ERROR:
		return "error";
	default:
		return "debug";
	}
}

bool app_status_all_ok(void)
{
	for (int i = 0; i < APP_MODULE_COUNT; i++) {
		if (atomic_get(&status[i]) == APP_STATUS_ERROR) {
			return false;
		}
	}
	return true;
}

static void log_boot(void)
{
	uint32_t cause = 0;

	LOG_INF("greenhouse %s", APP_VERSION_STRING);
	if (hwinfo_get_reset_cause(&cause) == 0) {
		LOG_INF("reset cause 0x%x", cause);
		hwinfo_clear_reset_cause();
	}
}

static int init_full(void)
{
	int rc;

	log_boot();

	/* the watchdog comes first, so it catches a hang in any later init */
	rc = app_watchdog_init();
	if (rc == 0) {
		rc = settings_subsys_init();
	}
	if (rc == 0) {
		rc = settings_load();
	}
	if (rc == 0) {
		rc = app_led_init();
	}
	/* actuators before anything that could ask them to move */
	if (rc == 0) {
		rc = app_actuator_init();
	}
	if (rc == 0) {
		rc = app_sensor_init();
	}
	if (rc == 0) {
		rc = app_control_init();
	}
	if (rc == 0) {
		rc = app_net_init();
	}
	if (rc == 0) {
		rc = app_backend_init();
	}
#if defined(CONFIG_BOOTLOADER_MCUBOOT)
	if (rc == 0) {
		rc = app_update_init();
	}
#endif
	return rc;
}

int main(void)
{
	int rc = init_full();

	if (rc != 0) {
		/* no threads started: the watchdog resets us if it is running */
		LOG_ERR("init failed: %d", rc);
		return rc;
	}

	app_watchdog_start();
	app_led_start();
	app_actuator_start();
	app_sensor_start();
	app_control_start();
	app_net_start();
	app_backend_start();
#if defined(CONFIG_BOOTLOADER_MCUBOOT)
	app_update_start();
#endif
	return 0;
}

/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_sensor.h"
#include "app_config.h"
#include "app_status.h"
#include "app_watchdog.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(app_sensor, CONFIG_APP_SENSOR_LOG_LEVEL);

ZBUS_CHAN_DEFINE(sensor_data_chan, struct sensor_data, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

static struct {
	int dry_mv;
	int wet_mv;
} soil = {CONFIG_APP_SOIL_DRY_MV, CONFIG_APP_SOIL_WET_MV};

static int sensor_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *arg)
{
	if (strcmp(name, "soil") != 0 || len != sizeof(soil)) {
		return -ENOENT;
	}
	return MIN(read_cb(arg, &soil, sizeof(soil)), 0);
}

SETTINGS_STATIC_HANDLER_DEFINE(app_sensor, "sensor", NULL, sensor_settings_set, NULL, NULL);

int app_sensor_soil_calibrate(int dry_mv, int wet_mv)
{
	if (dry_mv <= wet_mv) {
		return -EINVAL;
	}
	soil.dry_mv = dry_mv;
	soil.wet_mv = wet_mv;
	return settings_save_one("sensor/soil", &soil, sizeof(soil));
}

void app_sensor_soil_calibration(int *dry_mv, int *wet_mv)
{
	*dry_mv = soil.dry_mv;
	*wet_mv = soil.wet_mv;
}

static void sensor_thread(void *a, void *b, void *c)
{
	while (true) {
		struct sensor_data data = {0};
		int rc = sensor_backend_read(&data, soil.dry_mv, soil.wet_mv);

		if (rc != 0 || data.valid != (SENSOR_VALID_TEMP | SENSOR_VALID_HUM |
					      SENSOR_VALID_LIGHT | SENSOR_VALID_SOIL)) {
			LOG_ERR_RATELIMIT("incomplete reading, valid 0x%x", data.valid);
			app_status_set(APP_MODULE_SENSOR, APP_STATUS_ERROR);
		} else {
			app_status_set(APP_MODULE_SENSOR, APP_STATUS_OK);
		}

		zbus_chan_pub(&sensor_data_chan, &data, K_MSEC(500));
		app_watchdog_feed(APP_MODULE_SENSOR);

		/* sleep in slices, so the watchdog sees us during long intervals */
		for (int left = CONFIG_APP_SENSOR_SAMPLE_INTERVAL_S; left > 0;
		     left -= APP_HEARTBEAT_S) {
			k_sleep(K_SECONDS(MIN(left, APP_HEARTBEAT_S)));
			app_watchdog_feed(APP_MODULE_SENSOR);
		}
	}
}

K_THREAD_DEFINE(sensor_tid, APP_THREAD_STACK_SIZE, sensor_thread, NULL, NULL, NULL,
		APP_THREAD_PRIORITY, 0, SYS_FOREVER_MS);

int app_sensor_init(void)
{
	int rc = sensor_backend_init();

	if (rc != 0) {
		/* keep running: control handles invalid data, and a repair can fix it */
		LOG_ERR("sensor backend init failed: %d", rc);
		app_status_set(APP_MODULE_SENSOR, APP_STATUS_ERROR);
	}
	return 0;
}

void app_sensor_start(void)
{
	k_thread_start(sensor_tid);
}

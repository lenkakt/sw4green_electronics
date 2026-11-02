/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_control.h"
#include "app_config.h"
#include "app_status.h"
#include "app_watchdog.h"

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(app_control, CONFIG_APP_CONTROL_LOG_LEVEL);

ZBUS_CHAN_DEFINE(actuator_cmd_chan, struct actuator_cmd, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

ZBUS_SUBSCRIBER_DEFINE(control_sub, 4);
ZBUS_CHAN_ADD_OBS(sensor_data_chan, control_sub, 2);

static struct control_cfg cfg = {
	.vent_on_temp_dc = CONFIG_APP_CTRL_VENT_ON_TEMP_DC,
	.vent_off_temp_dc = CONFIG_APP_CTRL_VENT_OFF_TEMP_DC,
	.vent_on_hum_dpct = CONFIG_APP_CTRL_VENT_ON_HUM_DPCT,
	.vent_off_hum_dpct = CONFIG_APP_CTRL_VENT_OFF_HUM_DPCT,
	.water_on_soil_dpct = CONFIG_APP_CTRL_WATER_ON_SOIL_DPCT,
	.water_off_soil_dpct = CONFIG_APP_CTRL_WATER_OFF_SOIL_DPCT,
	.water_max_light_lux = CONFIG_APP_CTRL_WATER_MAX_LIGHT_LUX,
	.water_max_run_s = CONFIG_APP_CTRL_WATER_MAX_RUN_S,
	.water_min_pause_s = CONFIG_APP_CTRL_WATER_MIN_PAUSE_S,
};

#define FIELD(n) {#n, offsetof(struct control_cfg, n)}
static const struct {
	const char *name;
	size_t offset;
} fields[] = {
	FIELD(vent_on_temp_dc),    FIELD(vent_off_temp_dc),  FIELD(vent_on_hum_dpct),
	FIELD(vent_off_hum_dpct),  FIELD(water_on_soil_dpct), FIELD(water_off_soil_dpct),
	FIELD(water_max_light_lux), FIELD(water_max_run_s),   FIELD(water_min_pause_s),
};

const struct control_cfg *app_control_cfg(void)
{
	return &cfg;
}

int app_control_set(const char *name, int value)
{
	for (size_t i = 0; i < ARRAY_SIZE(fields); i++) {
		if (strcmp(fields[i].name, name) == 0) {
			*(int *)((char *)&cfg + fields[i].offset) = value;
			return settings_save_one("control/cfg", &cfg, sizeof(cfg));
		}
	}
	return -ENOENT;
}

static int control_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *arg)
{
	if (strcmp(name, "cfg") != 0 || len != sizeof(cfg)) {
		return -ENOENT;
	}
	return MIN(read_cb(arg, &cfg, sizeof(cfg)), 0);
}

SETTINGS_STATIC_HANDLER_DEFINE(app_control, "control", NULL, control_settings_set, NULL, NULL);

static void publish_if_changed(const struct control_state *st)
{
	static struct actuator_cmd last;
	static bool published;

	if (published && last.valve == st->valve && last.fan == st->fan) {
		return;
	}

	last = (struct actuator_cmd){
		.valve = st->valve,
		.fan = st->fan,
		.valve_reason = st->valve_reason,
		.fan_reason = st->fan_reason,
	};
	published = zbus_chan_pub(&actuator_cmd_chan, &last, K_MSEC(500)) == 0;
	if (!published) {
		LOG_ERR("command not delivered");
	}
}

static void control_thread(void *a, void *b, void *c)
{
	struct control_state st = {0};
	struct sensor_data data;
	bool have_data = false;

	app_status_set(APP_MODULE_CONTROL, APP_STATUS_OK);

	while (true) {
		const struct zbus_channel *chan;
		/* while watering, wake up often enough to enforce the longest run */
		k_timeout_t wait = st.valve ? K_SECONDS(1) : K_SECONDS(APP_HEARTBEAT_S);

		if (zbus_sub_wait(&control_sub, &chan, wait) == 0 &&
		    zbus_chan_read(chan, &data, K_MSEC(500)) == 0) {
			have_data = true;
		}
		if (have_data) {
			control_eval(&cfg, &st, &data, k_uptime_get());
			publish_if_changed(&st);
		}
		app_watchdog_feed(APP_MODULE_CONTROL);
	}
}

K_THREAD_DEFINE(control_tid, APP_THREAD_STACK_SIZE, control_thread, NULL, NULL, NULL,
		APP_THREAD_PRIORITY, 0, SYS_FOREVER_MS);

int app_control_init(void)
{
	return 0;
}

void app_control_start(void)
{
	k_thread_start(control_tid);
}

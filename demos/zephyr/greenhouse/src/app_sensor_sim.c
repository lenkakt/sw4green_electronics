/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_actuator.h"
#include "app_sensor.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/zbus/zbus.h>

/* Soil dries over time, and responds to the simulated valve. */
static atomic_t valve_on;
static atomic_t soil_dpct = ATOMIC_INIT(400);

static void actuator_cb(const struct zbus_channel *chan)
{
	const struct actuator_state *st = zbus_chan_const_msg(chan);

	if (st->id == ACTUATOR_VALVE) {
		atomic_set(&valve_on, st->on);
	}
}

ZBUS_LISTENER_DEFINE(sim_listener, actuator_cb);
ZBUS_CHAN_ADD_OBS(actuator_state_chan, sim_listener, 1);

/* 0 at midnight, 1000 at noon */
static int daylight(void)
{
	int day_ms = CONFIG_APP_SENSOR_SIM_DAY_S * 1000;
	int t = (int)(k_uptime_get() % day_ms);
	int half = day_ms / 2;

	return (t < half ? t : day_ms - t) / (half / 1000);
}

int sensor_backend_init(void)
{
	return 0;
}

int sensor_backend_read(struct sensor_data *data, int dry_mv, int wet_mv)
{
	int sun = daylight();
	int soil = atomic_get(&soil_dpct) + (atomic_get(&valve_on) ? 40 : -5);

	soil = CLAMP(soil, 0, 1000);
	atomic_set(&soil_dpct, soil);

	data->temp_dc = 150 + sun * 200 / 1000;
	data->humidity_dpct = 900 - sun * 500 / 1000;
	data->light_lux = sun * 50;
	data->soil_dpct = soil;
	data->soil_raw_mv = dry_mv - (dry_mv - wet_mv) * soil / 1000;
	data->valid = SENSOR_VALID_TEMP | SENSOR_VALID_HUM | SENSOR_VALID_LIGHT |
		      SENSOR_VALID_SOIL;
	return 0;
}

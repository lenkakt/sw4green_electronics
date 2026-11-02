/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_actuator.h"
#include "app_config.h"
#include "app_control.h"
#include "app_status.h"
#include "app_watchdog.h"

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(app_actuator, CONFIG_APP_ACTUATOR_LOG_LEVEL);

#define PULSE_MAX_S 30

ZBUS_CHAN_DEFINE(actuator_state_chan, struct actuator_state, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

ZBUS_SUBSCRIBER_DEFINE(actuator_sub, 4);
ZBUS_CHAN_ADD_OBS(actuator_cmd_chan, actuator_sub, 1);

#if !defined(CONFIG_APP_ACTUATOR_SIMULATE)
static const struct gpio_dt_spec pins[ACTUATOR_COUNT] = {
	[ACTUATOR_VALVE] = GPIO_DT_SPEC_GET(DT_ALIAS(water_valve), gpios),
	[ACTUATOR_FAN] = GPIO_DT_SPEC_GET(DT_ALIAS(vent_fan), gpios),
};
#endif

static bool is_on[ACTUATOR_COUNT];
static int64_t on_since[ACTUATOR_COUNT];
static K_MUTEX_DEFINE(actuator_lock);

const char *app_actuator_name(enum actuator_id id)
{
	return id == ACTUATOR_VALVE ? "valve" : "fan";
}

static int output_set(enum actuator_id id, bool on)
{
#if defined(CONFIG_APP_ACTUATOR_SIMULATE)
	return 0;
#else
	return gpio_pin_set_dt(&pins[id], on);
#endif
}

static void actuator_set(enum actuator_id id, bool on, enum control_reason reason)
{
	struct actuator_state state = {.id = id, .on = on, .reason = reason};

	k_mutex_lock(&actuator_lock, K_FOREVER);
	if (is_on[id] == on) {
		k_mutex_unlock(&actuator_lock);
		return;
	}
	if (output_set(id, on) != 0) {
		app_status_set(APP_MODULE_ACTUATOR, APP_STATUS_ERROR);
		LOG_ERR("%s: output failed", app_actuator_name(id));
		k_mutex_unlock(&actuator_lock);
		return;
	}
	is_on[id] = on;
	if (on) {
		on_since[id] = k_uptime_get();
	} else {
		state.run_s = (uint32_t)((k_uptime_get() - on_since[id]) / 1000);
	}
	k_mutex_unlock(&actuator_lock);

	LOG_INF("%s %s (%s)", app_actuator_name(id), on ? "on" : "off",
		control_reason_name(reason));
	zbus_chan_pub(&actuator_state_chan, &state, K_MSEC(500));
}

static void pulse_end(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(pulse_work, pulse_end);
static enum actuator_id pulse_id;

static void pulse_end(struct k_work *work)
{
	actuator_set(pulse_id, false, REASON_MANUAL);
}

int app_actuator_pulse(enum actuator_id id, int seconds)
{
	if (seconds < 1 || seconds > PULSE_MAX_S) {
		return -EINVAL;
	}
	pulse_id = id;
	actuator_set(id, true, REASON_MANUAL);
	k_work_reschedule(&pulse_work, K_SECONDS(seconds));
	return 0;
}

static void actuator_thread(void *a, void *b, void *c)
{
	while (true) {
		const struct zbus_channel *chan;
		struct actuator_cmd cmd;

		if (zbus_sub_wait(&actuator_sub, &chan, K_SECONDS(APP_HEARTBEAT_S)) == 0 &&
		    zbus_chan_read(chan, &cmd, K_MSEC(500)) == 0) {
			actuator_set(ACTUATOR_VALVE, cmd.valve, cmd.valve_reason);
			actuator_set(ACTUATOR_FAN, cmd.fan, cmd.fan_reason);
		}
		app_watchdog_feed(APP_MODULE_ACTUATOR);
	}
}

K_THREAD_DEFINE(actuator_tid, APP_THREAD_STACK_SIZE, actuator_thread, NULL, NULL, NULL,
		APP_THREAD_PRIORITY, 0, SYS_FOREVER_MS);

int app_actuator_init(void)
{
#if !defined(CONFIG_APP_ACTUATOR_SIMULATE)
	for (int i = 0; i < ACTUATOR_COUNT; i++) {
		if (!gpio_is_ready_dt(&pins[i]) ||
		    gpio_pin_configure_dt(&pins[i], GPIO_OUTPUT_INACTIVE) != 0) {
			LOG_ERR("%s: pin not ready", app_actuator_name(i));
			app_status_set(APP_MODULE_ACTUATOR, APP_STATUS_ERROR);
			return 0;
		}
	}
#endif
	app_status_set(APP_MODULE_ACTUATOR, APP_STATUS_OK);
	return 0;
}

void app_actuator_start(void)
{
	k_thread_start(actuator_tid);
}

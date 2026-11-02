/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_actuator.h"
#include "app_backend.h"
#include "app_control.h"
#include "app_device.h"
#include "app_sensor.h"
#include "app_status.h"
#include "app_watchdog.h"

#include <app_version.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/shell/shell.h>
#include <zephyr/zbus/zbus.h>

static int cmd_status(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "greenhouse %s, device %s", APP_VERSION_STRING, app_device_id());
	for (int i = 0; i < APP_MODULE_COUNT; i++) {
		shell_print(sh, "%-10s %s", app_status_module_name(i),
			    app_status_value_name(app_status_get(i)));
	}
	return 0;
}
SHELL_CMD_REGISTER(app_status, NULL, "Show the health of each module", cmd_status);

static int cmd_device(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		int rc = app_device_set_id(argv[1]);

		if (rc != 0) {
			shell_error(sh, "failed: %d", rc);
			return rc;
		}
	}
	shell_print(sh, "device id: %s", app_device_id());
	return 0;
}
SHELL_CMD_ARG_REGISTER(app_device, NULL, "Show or set the device ID: [new_id]", cmd_device, 1, 1);

static int cmd_sensor_read(const struct shell *sh, size_t argc, char **argv)
{
	struct sensor_data d;
	int rc = zbus_chan_read(&sensor_data_chan, &d, K_MSEC(500));

	if (rc != 0) {
		return rc;
	}
	shell_print(sh, "temperature %d (0.1 C) %s", d.temp_dc,
		    d.valid & SENSOR_VALID_TEMP ? "" : "INVALID");
	shell_print(sh, "humidity    %d (0.1 %%) %s", d.humidity_dpct,
		    d.valid & SENSOR_VALID_HUM ? "" : "INVALID");
	shell_print(sh, "light       %d lux %s", d.light_lux,
		    d.valid & SENSOR_VALID_LIGHT ? "" : "INVALID");
	shell_print(sh, "soil        %d (0.1 %%), raw %d mV %s", d.soil_dpct, d.soil_raw_mv,
		    d.valid & SENSOR_VALID_SOIL ? "" : "INVALID");
	return 0;
}

static int cmd_sensor_soil(const struct shell *sh, size_t argc, char **argv)
{
	int dry;
	int wet;

	if (argc == 3) {
		int rc = app_sensor_soil_calibrate(atoi(argv[1]), atoi(argv[2]));

		if (rc != 0) {
			shell_error(sh, "failed: %d (dry must be above wet)", rc);
			return rc;
		}
	}
	app_sensor_soil_calibration(&dry, &wet);
	shell_print(sh, "dry %d mV, wet %d mV", dry, wet);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sensor_cmds,
	SHELL_CMD(read, NULL, "Show the latest reading", cmd_sensor_read),
	SHELL_CMD_ARG(soil, NULL, "Show or set calibration: [dry_mv wet_mv]", cmd_sensor_soil, 1, 2),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(app_sensor, &sensor_cmds, "Sensors", NULL);

static int cmd_control_show(const struct shell *sh, size_t argc, char **argv)
{
	const struct control_cfg *c = app_control_cfg();

	shell_print(sh, "vent_on_temp_dc     %d", c->vent_on_temp_dc);
	shell_print(sh, "vent_off_temp_dc    %d", c->vent_off_temp_dc);
	shell_print(sh, "vent_on_hum_dpct    %d", c->vent_on_hum_dpct);
	shell_print(sh, "vent_off_hum_dpct   %d", c->vent_off_hum_dpct);
	shell_print(sh, "water_on_soil_dpct  %d", c->water_on_soil_dpct);
	shell_print(sh, "water_off_soil_dpct %d", c->water_off_soil_dpct);
	shell_print(sh, "water_max_light_lux %d", c->water_max_light_lux);
	shell_print(sh, "water_max_run_s     %d", c->water_max_run_s);
	shell_print(sh, "water_min_pause_s   %d", c->water_min_pause_s);
	return 0;
}

static int cmd_control_set(const struct shell *sh, size_t argc, char **argv)
{
	int rc = app_control_set(argv[1], atoi(argv[2]));

	if (rc != 0) {
		shell_error(sh, "failed: %d", rc);
	}
	return rc;
}

SHELL_STATIC_SUBCMD_SET_CREATE(control_cmds,
	SHELL_CMD(show, NULL, "Show the thresholds", cmd_control_show),
	SHELL_CMD_ARG(set, NULL, "Set a threshold: <name> <value>", cmd_control_set, 3, 0),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(app_control, &control_cmds, "Control rules", NULL);

static int cmd_actuator_pulse(const struct shell *sh, size_t argc, char **argv)
{
	enum actuator_id id;

	if (strcmp(argv[1], "valve") == 0) {
		id = ACTUATOR_VALVE;
	} else if (strcmp(argv[1], "fan") == 0) {
		id = ACTUATOR_FAN;
	} else {
		shell_error(sh, "valve or fan");
		return -EINVAL;
	}

	int rc = app_actuator_pulse(id, atoi(argv[2]));

	if (rc != 0) {
		shell_error(sh, "failed: %d (1 to 30 s)", rc);
	}
	return rc;
}

SHELL_STATIC_SUBCMD_SET_CREATE(actuator_cmds,
	SHELL_CMD_ARG(pulse, NULL, "Test an output: <valve|fan> <seconds>", cmd_actuator_pulse, 3, 0),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(app_actuator, &actuator_cmds, "Actuators", NULL);

static int cmd_backend_show(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "host %s, queued %d, dropped %u", app_backend_host(), app_backend_queued(),
		    app_backend_dropped());
	return 0;
}

static int cmd_backend_host(const struct shell *sh, size_t argc, char **argv)
{
	return app_backend_set_host(argv[1]);
}

static int cmd_backend_token(const struct shell *sh, size_t argc, char **argv)
{
	return app_backend_set_token(argv[1]);
}

SHELL_STATIC_SUBCMD_SET_CREATE(backend_cmds,
	SHELL_CMD(show, NULL, "Show the backend state", cmd_backend_show),
	SHELL_CMD_ARG(host, NULL, "Set the InfluxDB host: <host>", cmd_backend_host, 2, 0),
	SHELL_CMD_ARG(token, NULL, "Set the InfluxDB token: <token>", cmd_backend_token, 2, 0),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(app_backend, &backend_cmds, "Backend upload", NULL);

static int cmd_watchdog_status(const struct shell *sh, size_t argc, char **argv)
{
	for (int i = 0; i < APP_MODULE_COUNT; i++) {
		if (app_watchdog_is_tracked(i)) {
			shell_print(sh, "%-10s last seen %u s ago", app_status_module_name(i),
				    app_watchdog_age_s(i));
		}
	}
	return 0;
}

static int cmd_watchdog_stop(const struct shell *sh, size_t argc, char **argv)
{
	for (int i = 0; i < APP_MODULE_COUNT; i++) {
		if (strcmp(argv[1], app_status_module_name(i)) == 0) {
			app_watchdog_stop_feeding(i);
			shell_print(sh, "%s no longer feeds the watchdog: expect a reset", argv[1]);
			return 0;
		}
	}
	shell_error(sh, "unknown module");
	return -EINVAL;
}

SHELL_STATIC_SUBCMD_SET_CREATE(watchdog_cmds,
	SHELL_CMD(status, NULL, "Show each thread's last heartbeat", cmd_watchdog_status),
	SHELL_CMD_ARG(stop, NULL, "Test: simulate a hung thread: <module>", cmd_watchdog_stop, 2, 0),
	SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(app_watchdog, &watchdog_cmds, "Watchdog", NULL);

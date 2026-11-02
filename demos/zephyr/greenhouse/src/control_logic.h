/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONTROL_LOGIC_H_
#define CONTROL_LOGIC_H_

#include <stdbool.h>
#include <stdint.h>
#include "app_sensor.h"

enum control_reason {
	REASON_NONE,
	REASON_SOIL_DRY,
	REASON_SOIL_WET,
	REASON_MAX_RUN,
	REASON_SENSOR_ERROR,
	REASON_HOT_OR_HUMID,
	REASON_COOL_AND_DRY,
	REASON_MANUAL,
};

struct control_cfg {
	int vent_on_temp_dc;
	int vent_off_temp_dc;
	int vent_on_hum_dpct;
	int vent_off_hum_dpct;
	int water_on_soil_dpct;
	int water_off_soil_dpct;
	int water_max_light_lux;
	int water_max_run_s;
	int water_min_pause_s;
};

/* Starts with everything off. */
struct control_state {
	bool valve;
	bool fan;
	enum control_reason valve_reason;
	enum control_reason fan_reason;
	int64_t valve_on_ms;
	int64_t valve_off_ms;
	bool valve_ran;
};

void control_eval(const struct control_cfg *cfg, struct control_state *st,
		  const struct sensor_data *data, int64_t now_ms);

const char *control_reason_name(enum control_reason reason);

#endif /* CONTROL_LOGIC_H_ */

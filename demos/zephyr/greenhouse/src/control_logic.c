/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "control_logic.h"

static void eval_fan(const struct control_cfg *cfg, struct control_state *st,
		     const struct sensor_data *d)
{
	bool temp_ok = d->valid & SENSOR_VALID_TEMP;
	bool hum_ok = d->valid & SENSOR_VALID_HUM;

	if (!temp_ok && !hum_ok) {
		st->fan = false;
		st->fan_reason = REASON_SENSOR_ERROR;
		return;
	}

	bool hot = temp_ok && d->temp_dc >= cfg->vent_on_temp_dc;
	bool humid = hum_ok && d->humidity_dpct >= cfg->vent_on_hum_dpct;
	bool cool = !temp_ok || d->temp_dc <= cfg->vent_off_temp_dc;
	bool dry = !hum_ok || d->humidity_dpct <= cfg->vent_off_hum_dpct;

	if (hot || humid) {
		st->fan = true;
		st->fan_reason = REASON_HOT_OR_HUMID;
	} else if (cool && dry) {
		st->fan = false;
		st->fan_reason = REASON_COOL_AND_DRY;
	}
}

static void valve_off(struct control_state *st, enum control_reason reason, int64_t now_ms)
{
	if (st->valve) {
		st->valve_off_ms = now_ms;
		st->valve_ran = true;
	}
	st->valve = false;
	st->valve_reason = reason;
}

static void eval_valve(const struct control_cfg *cfg, struct control_state *st,
		       const struct sensor_data *d, int64_t now_ms)
{
	/* a stuck-open valve is the dangerous failure, so this comes first */
	if (st->valve && now_ms - st->valve_on_ms >= cfg->water_max_run_s * 1000LL) {
		valve_off(st, REASON_MAX_RUN, now_ms);
		return;
	}

	if (!(d->valid & SENSOR_VALID_SOIL)) {
		valve_off(st, REASON_SENSOR_ERROR, now_ms);
		return;
	}

	if (st->valve) {
		if (d->soil_dpct >= cfg->water_off_soil_dpct) {
			valve_off(st, REASON_SOIL_WET, now_ms);
		}
		return;
	}

	bool paused = st->valve_ran &&
		      now_ms - st->valve_off_ms < cfg->water_min_pause_s * 1000LL;
	/* a broken light sensor must not stop the watering */
	bool sunny = (d->valid & SENSOR_VALID_LIGHT) &&
		     d->light_lux > cfg->water_max_light_lux;

	if (d->soil_dpct <= cfg->water_on_soil_dpct && !paused && !sunny) {
		st->valve = true;
		st->valve_on_ms = now_ms;
		st->valve_reason = REASON_SOIL_DRY;
	}
}

void control_eval(const struct control_cfg *cfg, struct control_state *st,
		  const struct sensor_data *data, int64_t now_ms)
{
	eval_fan(cfg, st, data);
	eval_valve(cfg, st, data, now_ms);
}

const char *control_reason_name(enum control_reason reason)
{
	switch (reason) {
	case REASON_SOIL_DRY:
		return "soil_dry";
	case REASON_SOIL_WET:
		return "soil_wet";
	case REASON_MAX_RUN:
		return "max_run";
	case REASON_SENSOR_ERROR:
		return "sensor_error";
	case REASON_HOT_OR_HUMID:
		return "hot_or_humid";
	case REASON_COOL_AND_DRY:
		return "cool_and_dry";
	case REASON_MANUAL:
		return "manual";
	default:
		return "none";
	}
}

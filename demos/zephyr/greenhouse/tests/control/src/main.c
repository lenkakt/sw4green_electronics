/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "control_logic.h"

static const struct control_cfg cfg = {
	.vent_on_temp_dc = 300,
	.vent_off_temp_dc = 260,
	.vent_on_hum_dpct = 850,
	.vent_off_hum_dpct = 700,
	.water_on_soil_dpct = 350,
	.water_off_soil_dpct = 550,
	.water_max_light_lux = 30000,
	.water_max_run_s = 120,
	.water_min_pause_s = 600,
};

#define ALL_VALID (SENSOR_VALID_TEMP | SENSOR_VALID_HUM | SENSOR_VALID_LIGHT | SENSOR_VALID_SOIL)

static struct sensor_data sample(int temp, int hum, int lux, int soil)
{
	return (struct sensor_data){
		.temp_dc = temp, .humidity_dpct = hum, .light_lux = lux,
		.soil_dpct = soil, .valid = ALL_VALID,
	};
}

ZTEST(control, test_dry_soil_starts_watering)
{
	struct control_state st = {0};
	struct sensor_data d = sample(200, 500, 100, 300);

	control_eval(&cfg, &st, &d, 1000);
	zassert_true(st.valve);
	zassert_equal(st.valve_reason, REASON_SOIL_DRY);
}

ZTEST(control, test_no_watering_in_strong_sun)
{
	struct control_state st = {0};
	struct sensor_data d = sample(200, 500, 40000, 300);

	control_eval(&cfg, &st, &d, 1000);
	zassert_false(st.valve);
}

ZTEST(control, test_broken_light_sensor_still_waters)
{
	struct control_state st = {0};
	struct sensor_data d = sample(200, 500, 40000, 300);

	d.valid &= ~SENSOR_VALID_LIGHT;
	control_eval(&cfg, &st, &d, 1000);
	zassert_true(st.valve);
}

ZTEST(control, test_wet_soil_stops_watering)
{
	struct control_state st = {.valve = true, .valve_on_ms = 0};
	struct sensor_data d = sample(200, 500, 100, 560);

	control_eval(&cfg, &st, &d, 1000);
	zassert_false(st.valve);
	zassert_equal(st.valve_reason, REASON_SOIL_WET);
}

ZTEST(control, test_hysteresis_keeps_watering)
{
	struct control_state st = {.valve = true, .valve_on_ms = 0};
	struct sensor_data d = sample(200, 500, 100, 450);

	control_eval(&cfg, &st, &d, 1000);
	zassert_true(st.valve);
}

ZTEST(control, test_max_run_stops_watering)
{
	struct control_state st = {.valve = true, .valve_on_ms = 0};
	struct sensor_data d = sample(200, 500, 100, 100);

	control_eval(&cfg, &st, &d, 120 * 1000);
	zassert_false(st.valve);
	zassert_equal(st.valve_reason, REASON_MAX_RUN);
}

ZTEST(control, test_pause_after_run)
{
	struct control_state st = {.valve = true, .valve_on_ms = 0};
	struct sensor_data d = sample(200, 500, 100, 100);
	int64_t stop = 120 * 1000;

	control_eval(&cfg, &st, &d, stop);
	control_eval(&cfg, &st, &d, stop + 599 * 1000);
	zassert_false(st.valve, "must pause");
	control_eval(&cfg, &st, &d, stop + 600 * 1000);
	zassert_true(st.valve, "pause is over");
}

ZTEST(control, test_soil_sensor_error_closes_valve)
{
	struct control_state st = {.valve = true, .valve_on_ms = 0};
	struct sensor_data d = sample(200, 500, 100, 100);

	d.valid &= ~SENSOR_VALID_SOIL;
	control_eval(&cfg, &st, &d, 1000);
	zassert_false(st.valve);
	zassert_equal(st.valve_reason, REASON_SENSOR_ERROR);
}

ZTEST(control, test_fan_hysteresis)
{
	struct control_state st = {0};
	struct sensor_data d = sample(310, 500, 100, 500);

	control_eval(&cfg, &st, &d, 1000);
	zassert_true(st.fan, "too hot");

	d.temp_dc = 280;
	control_eval(&cfg, &st, &d, 2000);
	zassert_true(st.fan, "between the limits");

	d.temp_dc = 250;
	control_eval(&cfg, &st, &d, 3000);
	zassert_false(st.fan, "cool again");
}

ZTEST(control, test_fan_on_humidity)
{
	struct control_state st = {0};
	struct sensor_data d = sample(200, 900, 100, 500);

	control_eval(&cfg, &st, &d, 1000);
	zassert_true(st.fan);
}

ZTEST(control, test_fan_off_without_data)
{
	struct control_state st = {.fan = true};
	struct sensor_data d = {.valid = 0};

	control_eval(&cfg, &st, &d, 1000);
	zassert_false(st.fan);
}

ZTEST_SUITE(control, NULL, NULL, NULL, NULL, NULL);

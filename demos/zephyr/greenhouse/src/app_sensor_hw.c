/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_sensor.h"

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app_sensor, CONFIG_APP_SENSOR_LOG_LEVEL);

#define SOIL_NODE DT_ALIAS(soil_sensor)
#define SOIL_SETTLE_MS 100
#define SOIL_SAMPLES 4

static const struct device *const env = DEVICE_DT_GET(DT_ALIAS(env_sensor));
static const struct device *const light = DEVICE_DT_GET(DT_ALIAS(light_sensor));
static const struct adc_dt_spec soil_adc = ADC_DT_SPEC_GET(SOIL_NODE);
static const struct gpio_dt_spec soil_power = GPIO_DT_SPEC_GET(SOIL_NODE, power_gpios);

int sensor_backend_init(void)
{
	int rc = 0;

	if (!device_is_ready(env)) {
		LOG_ERR("env sensor not ready");
		rc = -ENODEV;
	}
	if (!device_is_ready(light)) {
		LOG_ERR("light sensor not ready");
		rc = -ENODEV;
	}
	if (!adc_is_ready_dt(&soil_adc) || !gpio_is_ready_dt(&soil_power)) {
		LOG_ERR("soil sensor not ready");
		return -ENODEV;
	}
	if (adc_channel_setup_dt(&soil_adc) != 0 ||
	    gpio_pin_configure_dt(&soil_power, GPIO_OUTPUT_INACTIVE) != 0) {
		LOG_ERR("soil sensor setup failed");
		rc = -EIO;
	}
	return rc;
}

static void read_env(struct sensor_data *d)
{
	struct sensor_value v;

	if (!device_is_ready(env) || sensor_sample_fetch(env) != 0) {
		return;
	}
	if (sensor_channel_get(env, SENSOR_CHAN_AMBIENT_TEMP, &v) == 0) {
		d->temp_dc = v.val1 * 10 + v.val2 / 100000;
		d->valid |= SENSOR_VALID_TEMP;
	}
	if (sensor_channel_get(env, SENSOR_CHAN_HUMIDITY, &v) == 0) {
		d->humidity_dpct = v.val1 * 10 + v.val2 / 100000;
		d->valid |= SENSOR_VALID_HUM;
	}
}

static void read_light(struct sensor_data *d)
{
	struct sensor_value v;

	if (device_is_ready(light) && sensor_sample_fetch(light) == 0 &&
	    sensor_channel_get(light, SENSOR_CHAN_LIGHT, &v) == 0) {
		d->light_lux = v.val1;
		d->valid |= SENSOR_VALID_LIGHT;
	}
}

static int read_soil_mv(void)
{
	int16_t raw;
	struct adc_sequence seq = {
		.buffer = &raw,
		.buffer_size = sizeof(raw),
	};
	int32_t sum = 0;

	adc_sequence_init_dt(&soil_adc, &seq);
	for (int i = 0; i < SOIL_SAMPLES; i++) {
		int rc = adc_read_dt(&soil_adc, &seq);

		if (rc != 0) {
			return rc;
		}
		sum += raw;
	}

	int32_t mv = sum / SOIL_SAMPLES;
	int rc = adc_raw_to_millivolts_dt(&soil_adc, &mv);

	return rc < 0 ? rc : mv;
}

static void read_soil(struct sensor_data *d, int dry_mv, int wet_mv)
{
	gpio_pin_set_dt(&soil_power, 1);
	k_msleep(SOIL_SETTLE_MS);
	int mv = read_soil_mv();

	gpio_pin_set_dt(&soil_power, 0);
	if (mv < 0) {
		return;
	}

	/* below 100 mV the sensor is disconnected, not very wet */
	d->soil_raw_mv = mv;
	if (mv < 100) {
		return;
	}
	d->soil_dpct = CLAMP((dry_mv - mv) * 1000 / (dry_mv - wet_mv), 0, 1000);
	d->valid |= SENSOR_VALID_SOIL;
}

int sensor_backend_read(struct sensor_data *data, int dry_mv, int wet_mv)
{
	read_env(data);
	read_light(data);
	read_soil(data, dry_mv, wet_mv);
	return 0;
}

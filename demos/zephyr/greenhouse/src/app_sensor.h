/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_SENSOR_H_
#define APP_SENSOR_H_

#include <stdint.h>
#include <zephyr/zbus/zbus.h>

#define SENSOR_VALID_TEMP BIT(0)
#define SENSOR_VALID_HUM BIT(1)
#define SENSOR_VALID_LIGHT BIT(2)
#define SENSOR_VALID_SOIL BIT(3)

/* Integers on purpose: the ESP32-C6 has no FPU. */
struct sensor_data {
	int16_t temp_dc;	/* 0.1 C */
	int16_t humidity_dpct;	/* 0.1 % */
	int32_t light_lux;
	int16_t soil_dpct;	/* 0.1 % */
	int16_t soil_raw_mv;
	uint8_t valid;
};

ZBUS_CHAN_DECLARE(sensor_data_chan);

int app_sensor_init(void);
void app_sensor_start(void);
int app_sensor_soil_calibrate(int dry_mv, int wet_mv);
void app_sensor_soil_calibration(int *dry_mv, int *wet_mv);

/* Implemented by app_sensor_hw.c or app_sensor_sim.c. */
int sensor_backend_init(void);
int sensor_backend_read(struct sensor_data *data, int dry_mv, int wet_mv);

#endif /* APP_SENSOR_H_ */

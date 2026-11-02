/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_ACTUATOR_H_
#define APP_ACTUATOR_H_

#include "control_logic.h"
#include <zephyr/zbus/zbus.h>

enum actuator_id {
	ACTUATOR_VALVE,
	ACTUATOR_FAN,
	ACTUATOR_COUNT,
};

struct actuator_state {
	enum actuator_id id;
	bool on;
	enum control_reason reason;
	uint32_t run_s;		/* length of the run that just ended */
};

ZBUS_CHAN_DECLARE(actuator_state_chan);

int app_actuator_init(void);
void app_actuator_start(void);
const char *app_actuator_name(enum actuator_id id);

/* Bring-up test: on for a few seconds, then off again. */
int app_actuator_pulse(enum actuator_id id, int seconds);

#endif /* APP_ACTUATOR_H_ */

/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_CONTROL_H_
#define APP_CONTROL_H_

#include "control_logic.h"
#include <zephyr/zbus/zbus.h>

struct actuator_cmd {
	bool valve;
	bool fan;
	enum control_reason valve_reason;
	enum control_reason fan_reason;
};

ZBUS_CHAN_DECLARE(actuator_cmd_chan);

int app_control_init(void);
void app_control_start(void);
const struct control_cfg *app_control_cfg(void);
int app_control_set(const char *name, int value);

#endif /* APP_CONTROL_H_ */

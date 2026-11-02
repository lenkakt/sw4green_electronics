/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_WATCHDOG_H_
#define APP_WATCHDOG_H_

#include "app_status.h"
#include <stdint.h>

int app_watchdog_init(void);
void app_watchdog_start(void);
void app_watchdog_feed(enum app_module module);
uint32_t app_watchdog_age_s(enum app_module module);
bool app_watchdog_is_tracked(enum app_module module);

/* Test: pretend a thread has hung. */
void app_watchdog_stop_feeding(enum app_module module);

#endif /* APP_WATCHDOG_H_ */

/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_update.h"
#include "app_config.h"
#include "app_status.h"
#include "app_watchdog.h"

#include <zephyr/dfu/mcuboot.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_update, CONFIG_APP_UPDATE_LOG_LEVEL);

/* Unconfirmed images roll back on the next reset, so health decides. */
static void update_thread(void *a, void *b, void *c)
{
	int healthy_s = 0;

	while (!boot_is_img_confirmed()) {
		k_sleep(K_SECONDS(APP_HEARTBEAT_S));
		app_watchdog_feed(APP_MODULE_UPDATE);

		healthy_s = app_status_all_ok() ? healthy_s + APP_HEARTBEAT_S : 0;
		if (healthy_s >= CONFIG_APP_UPDATE_CONFIRM_S) {
			int rc = boot_write_img_confirmed();

			LOG_INF("new image confirmed: %d", rc);
			app_status_set(APP_MODULE_UPDATE, rc == 0 ? APP_STATUS_OK : APP_STATUS_ERROR);
		}
	}

	app_status_set(APP_MODULE_UPDATE, APP_STATUS_OK);
	while (true) {
		k_sleep(K_SECONDS(APP_HEARTBEAT_S));
		app_watchdog_feed(APP_MODULE_UPDATE);
	}
}

K_THREAD_DEFINE(update_tid, APP_THREAD_STACK_SIZE, update_thread, NULL, NULL, NULL,
		APP_THREAD_PRIORITY + 2, 0, SYS_FOREVER_MS);

int app_update_init(void)
{
	return 0;
}

void app_update_start(void)
{
	k_thread_start(update_tid);
}

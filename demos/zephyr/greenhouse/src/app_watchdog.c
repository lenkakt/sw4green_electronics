/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_watchdog.h"
#include "app_config.h"

#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_REGISTER(app_watchdog, CONFIG_APP_WATCHDOG_LOG_LEVEL);

#define WDT_NODE DT_ALIAS(watchdog0)
#define WDT_TIMEOUT_MS 60000
#define CHECK_PERIOD_S 10

#if DT_NODE_HAS_STATUS_OKAY(WDT_NODE)
static const struct device *const wdt = DEVICE_DT_GET(WDT_NODE);
#else
static const struct device *const wdt;
#endif

static int wdt_channel = -1;
static int64_t last_seen[APP_MODULE_COUNT];
static atomic_t tracked;
static atomic_t stopped;

void app_watchdog_feed(enum app_module module)
{
	if (atomic_test_bit(&stopped, module)) {
		return;
	}
	atomic_set_bit(&tracked, module);
	last_seen[module] = k_uptime_get();
}

bool app_watchdog_is_tracked(enum app_module module)
{
	return atomic_test_bit(&tracked, module);
}

uint32_t app_watchdog_age_s(enum app_module module)
{
	return (uint32_t)((k_uptime_get() - last_seen[module]) / 1000);
}

void app_watchdog_stop_feeding(enum app_module module)
{
	atomic_set_bit(&stopped, module);
}

static bool all_alive(void)
{
	for (int i = 0; i < APP_MODULE_COUNT; i++) {
		if (app_watchdog_is_tracked(i) && app_watchdog_age_s(i) > CONFIG_APP_WATCHDOG_STALE_S) {
			LOG_ERR("%s hung for %u s", app_status_module_name(i), app_watchdog_age_s(i));
			return false;
		}
	}
	return true;
}

static void watchdog_thread(void *a, void *b, void *c)
{
	while (true) {
		k_sleep(K_SECONDS(CHECK_PERIOD_S));
		if (!all_alive()) {
			break;
		}
		if (wdt_channel >= 0) {
			wdt_feed(wdt, wdt_channel);
		}
	}

	app_status_set(APP_MODULE_WATCHDOG, APP_STATUS_ERROR);
	LOG_PANIC();
	/* if the hardware does not do it first */
	k_sleep(K_MSEC(WDT_TIMEOUT_MS + 5000));
	sys_reboot(SYS_REBOOT_COLD);
}

K_THREAD_DEFINE(watchdog_tid, APP_THREAD_STACK_SIZE, watchdog_thread, NULL, NULL, NULL,
		APP_THREAD_PRIORITY - 1, 0, SYS_FOREVER_MS);

int app_watchdog_init(void)
{
	struct wdt_timeout_cfg cfg = {
		.window.max = WDT_TIMEOUT_MS,
		.flags = WDT_FLAG_RESET_SOC,
	};

	if (wdt == NULL || !device_is_ready(wdt)) {
		LOG_WRN("no hardware watchdog");
		app_status_set(APP_MODULE_WATCHDOG, APP_STATUS_DEBUG);
		return 0;
	}

	wdt_channel = wdt_install_timeout(wdt, &cfg);
	if (wdt_channel < 0 || wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG) != 0) {
		LOG_ERR("hardware watchdog setup failed");
		wdt_channel = -1;
		app_status_set(APP_MODULE_WATCHDOG, APP_STATUS_ERROR);
		return 0;
	}

	app_status_set(APP_MODULE_WATCHDOG, APP_STATUS_OK);
	return 0;
}

void app_watchdog_start(void)
{
	k_thread_start(watchdog_tid);
}

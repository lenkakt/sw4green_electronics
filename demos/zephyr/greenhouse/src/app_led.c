/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_led.h"
#include "app_config.h"
#include "app_status.h"
#include "app_watchdog.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#define LED_NODE DT_ALIAS(led0)
#define HAS_LED (IS_ENABLED(CONFIG_APP_LED) && DT_NODE_HAS_STATUS_OKAY(LED_NODE))

#if HAS_LED
#define BLINK_OK_MS 2000
#define BLINK_ERROR_MS 200

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
static atomic_t blink_ms = ATOMIC_INIT(BLINK_OK_MS);

static void status_cb(const struct zbus_channel *chan)
{
	atomic_set(&blink_ms, app_status_all_ok() ? BLINK_OK_MS : BLINK_ERROR_MS);
}

ZBUS_LISTENER_DEFINE(led_listener, status_cb);
ZBUS_CHAN_ADD_OBS(app_status_chan, led_listener, 1);

static void led_thread(void *a, void *b, void *c)
{
	while (true) {
		gpio_pin_toggle_dt(&led);
		app_watchdog_feed(APP_MODULE_LED);
		k_msleep(atomic_get(&blink_ms));
	}
}

K_THREAD_DEFINE(led_tid, 768, led_thread, NULL, NULL, NULL, APP_THREAD_PRIORITY + 2, 0,
		SYS_FOREVER_MS);

int app_led_init(void)
{
	if (!gpio_is_ready_dt(&led)) {
		app_status_set(APP_MODULE_LED, APP_STATUS_ERROR);
		return 0;
	}
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	app_status_set(APP_MODULE_LED, APP_STATUS_OK);
	return 0;
}

void app_led_start(void)
{
	k_thread_start(led_tid);
}
#else
int app_led_init(void)
{
	return 0;
}

void app_led_start(void)
{
}
#endif

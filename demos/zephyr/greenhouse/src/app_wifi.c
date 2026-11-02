/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_config.h"
#include "app_net.h"
#include "app_status.h"
#include "app_watchdog.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_credentials.h>

LOG_MODULE_REGISTER(app_net, CONFIG_APP_NET_LOG_LEVEL);

#define WIFI_RETRY_S 30

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ip_cb;
static K_SEM_DEFINE(net_changed, 0, 1);
static atomic_t connected;

static void wifi_event(struct net_mgmt_event_callback *cb, uint64_t event, struct net_if *iface)
{
	const struct wifi_status *status = cb->info;

	if (event == NET_EVENT_WIFI_CONNECT_RESULT) {
		if (status->status != 0) {
			LOG_WRN("connect failed: %d", status->status);
			atomic_set(&connected, 0);
		} else {
			atomic_set(&connected, 1);
		}
	} else if (event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
		atomic_set(&connected, 0);
		app_status_set(APP_MODULE_NET, APP_STATUS_ERROR);
	}
	k_sem_give(&net_changed);
}

static void ip_event(struct net_mgmt_event_callback *cb, uint64_t event, struct net_if *iface)
{
	app_status_set(APP_MODULE_NET, APP_STATUS_OK);
	k_sem_give(&net_changed);
}

static void power_save_on(struct net_if *iface)
{
	struct wifi_ps_params params = {
		.enabled = WIFI_PS_ENABLED,
		.type = WIFI_PS_PARAM_STATE,
	};

	if (net_mgmt(NET_REQUEST_WIFI_PS, iface, &params, sizeof(params)) != 0) {
		LOG_WRN("Wi-Fi power save not set");
	}
}

static void net_thread(void *a, void *b, void *c)
{
	struct net_if *iface = net_if_get_first_wifi();
	bool ps_set = false;

	while (true) {
		if (!atomic_get(&connected)) {
			ps_set = false;
			if (wifi_credentials_is_empty()) {
				/* wifi cred add, then this picks it up */
				app_status_set(APP_MODULE_NET, APP_STATUS_DEBUG);
			} else if (net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0) != 0) {
				LOG_WRN_RATELIMIT("connect request failed");
			}
		} else if (!ps_set) {
			power_save_on(iface);
			ps_set = true;
		}

		app_watchdog_feed(APP_MODULE_NET);
		k_sem_take(&net_changed, K_SECONDS(MIN(WIFI_RETRY_S, APP_HEARTBEAT_S)));
	}
}

K_THREAD_DEFINE(net_tid, APP_NET_STACK_SIZE, net_thread, NULL, NULL, NULL, APP_THREAD_PRIORITY, 0,
		SYS_FOREVER_MS);

int app_net_init(void)
{
	net_mgmt_init_event_callback(&wifi_cb, wifi_event,
				     NET_EVENT_WIFI_CONNECT_RESULT |
					     NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_cb);
	net_mgmt_init_event_callback(&ip_cb, ip_event, NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&ip_cb);
	return 0;
}

void app_net_start(void)
{
	k_thread_start(net_tid);
}

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

LOG_MODULE_REGISTER(app_net, CONFIG_APP_NET_LOG_LEVEL);

static struct net_mgmt_event_callback mgmt_cb;

static void net_event(struct net_mgmt_event_callback *cb, uint64_t event, struct net_if *iface)
{
	if (event == NET_EVENT_IPV4_ADDR_ADD) {
		app_status_set(APP_MODULE_NET, APP_STATUS_OK);
	} else if (event == NET_EVENT_IPV4_ADDR_DEL) {
		app_status_set(APP_MODULE_NET, APP_STATUS_ERROR);
	}
}

static void net_thread(void *a, void *b, void *c)
{
	net_dhcpv4_start(net_if_get_default());

	while (true) {
		app_watchdog_feed(APP_MODULE_NET);
		k_sleep(K_SECONDS(APP_HEARTBEAT_S));
	}
}

K_THREAD_DEFINE(net_tid, APP_THREAD_STACK_SIZE, net_thread, NULL, NULL, NULL,
		APP_THREAD_PRIORITY, 0, SYS_FOREVER_MS);

int app_net_init(void)
{
	net_mgmt_init_event_callback(&mgmt_cb, net_event,
				     NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL);
	net_mgmt_add_event_callback(&mgmt_cb);
	return 0;
}

void app_net_start(void)
{
	k_thread_start(net_tid);
}

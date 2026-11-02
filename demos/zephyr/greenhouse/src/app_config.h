/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#define APP_THREAD_STACK_SIZE 1536
#define APP_NET_STACK_SIZE 3072
#define APP_BACKEND_STACK_SIZE 4096
#define APP_THREAD_PRIORITY 7

/* no thread may block longer than this, see app_watchdog */
#define APP_HEARTBEAT_S 30

#endif /* APP_CONFIG_H_ */

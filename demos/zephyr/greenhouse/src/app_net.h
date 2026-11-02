/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_NET_H_
#define APP_NET_H_

/* app_wifi.c or app_eth.c. Reports through APP_MODULE_NET. */
int app_net_init(void);
void app_net_start(void);

#endif /* APP_NET_H_ */

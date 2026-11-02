/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_BACKEND_H_
#define APP_BACKEND_H_

#include <stdint.h>

int app_backend_init(void);
void app_backend_start(void);
const char *app_backend_host(void);
int app_backend_set_host(const char *host);
int app_backend_set_token(const char *token);
int app_backend_queued(void);
uint32_t app_backend_dropped(void);

#endif /* APP_BACKEND_H_ */

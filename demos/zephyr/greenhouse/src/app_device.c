/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_device.h"

#include <errno.h>
#include <string.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>

static char device_id[17] = CONFIG_APP_DEVICE_ID;

static int device_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	if (strcmp(name, "id") != 0 || len >= sizeof(device_id)) {
		return -ENOENT;
	}

	int rc = read_cb(cb_arg, device_id, len);

	if (rc >= 0) {
		device_id[rc] = '\0';
	}
	return rc < 0 ? rc : 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(app_device, "device", NULL, device_set, NULL, NULL);

const char *app_device_id(void)
{
	return device_id;
}

int app_device_set_id(const char *id)
{
	size_t len = strlen(id);

	if (len == 0 || len >= sizeof(device_id)) {
		return -EINVAL;
	}

	strcpy(device_id, id);
	return settings_save_one("device/id", id, len);
}

/*
 * Copyright (c) 2026 Technical University of Liberec
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_backend.h"
#include "app_actuator.h"
#include "app_config.h"
#include "app_device.h"
#include "app_sensor.h"
#include "app_status.h"
#include "app_watchdog.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/sntp.h>
#include <zephyr/pm/policy.h>
#include <zephyr/settings/settings.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(app_backend, CONFIG_APP_BACKEND_LOG_LEVEL);

#define POINT_LEN 160
#define BODY_LEN 1536
#define HTTP_TIMEOUT_MS 5000
#define NTP_TIMEOUT_MS 3000
#define NTP_RESYNC_MS (24LL * 3600 * 1000)
#define NTP_RETRY_MS (5LL * 60 * 1000)
#define UPLOAD_RETRY_MS (30 * 1000)

ZBUS_MSG_SUBSCRIBER_DEFINE(backend_sub);
ZBUS_CHAN_ADD_OBS(sensor_data_chan, backend_sub, 3);
ZBUS_CHAN_ADD_OBS(actuator_state_chan, backend_sub, 3);

struct point {
	int64_t uptime_ms;
	char line[POINT_LEN];
};

static struct point queue[CONFIG_APP_BACKEND_QUEUE_SIZE];
static int queue_head;
static int queue_count;
static uint32_t dropped;
static K_MUTEX_DEFINE(queue_lock);

static char host[64] = CONFIG_APP_BACKEND_HOST;
static char token[128] = CONFIG_APP_BACKEND_DEV_TOKEN;
static char body[BODY_LEN];
static uint8_t recv_buf[256];
static int http_status;

/* epoch_ms = uptime_ms + offset; 0 means no time yet */
static int64_t epoch_offset_ms;
static int64_t ntp_next_ms;

static int backend_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *arg)
{
	char *dst;
	size_t size;

	if (strcmp(name, "host") == 0) {
		dst = host;
		size = sizeof(host);
	} else if (strcmp(name, "token") == 0) {
		dst = token;
		size = sizeof(token);
	} else {
		return -ENOENT;
	}
	if (len >= size) {
		return -EINVAL;
	}

	int rc = read_cb(arg, dst, len);

	if (rc >= 0) {
		dst[rc] = '\0';
	}
	return MIN(rc, 0);
}

SETTINGS_STATIC_HANDLER_DEFINE(app_backend, "backend", NULL, backend_settings_set, NULL, NULL);

const char *app_backend_host(void)
{
	return host;
}

int app_backend_set_host(const char *new_host)
{
	if (strlen(new_host) >= sizeof(host)) {
		return -EINVAL;
	}
	strcpy(host, new_host);
	return settings_save_one("backend/host", host, strlen(host));
}

int app_backend_set_token(const char *new_token)
{
	if (strlen(new_token) >= sizeof(token)) {
		return -EINVAL;
	}
	strcpy(token, new_token);
	return settings_save_one("backend/token", token, strlen(token));
}

int app_backend_queued(void)
{
	return queue_count;
}

uint32_t app_backend_dropped(void)
{
	return dropped;
}

static void queue_push(const char *line)
{
	k_mutex_lock(&queue_lock, K_FOREVER);
	if (queue_count == ARRAY_SIZE(queue)) {
		queue_head = (queue_head + 1) % ARRAY_SIZE(queue);
		queue_count--;
		dropped++;
		LOG_WRN_RATELIMIT("queue full, dropped %u", dropped);
	}

	struct point *p = &queue[(queue_head + queue_count) % ARRAY_SIZE(queue)];

	p->uptime_ms = k_uptime_get();
	strncpy(p->line, line, sizeof(p->line) - 1);
	p->line[sizeof(p->line) - 1] = '\0';
	queue_count++;
	k_mutex_unlock(&queue_lock);
}

static int put_tenths(char *buf, size_t size, const char *name, int value)
{
	const char *sign = value < 0 ? "-" : "";
	int abs_value = value < 0 ? -value : value;

	return snprintk(buf, size, "%s=%s%d.%d,", name, sign, abs_value / 10, abs_value % 10);
}

static void queue_sensor(const struct sensor_data *d)
{
	char line[POINT_LEN];
	int n = snprintk(line, sizeof(line), "environment,device_id=%s ", app_device_id());
	int start = n;

	if (d->valid & SENSOR_VALID_TEMP) {
		n += put_tenths(line + n, sizeof(line) - n, "temperature_c", d->temp_dc);
	}
	if (d->valid & SENSOR_VALID_HUM) {
		n += put_tenths(line + n, sizeof(line) - n, "humidity_pct", d->humidity_dpct);
	}
	if (d->valid & SENSOR_VALID_LIGHT) {
		n += snprintk(line + n, sizeof(line) - n, "light_lux=%di,", d->light_lux);
	}
	if (d->valid & SENSOR_VALID_SOIL) {
		n += put_tenths(line + n, sizeof(line) - n, "soil_moisture_pct", d->soil_dpct);
	}
	if (n == start) {
		return;
	}
	line[n - 1] = '\0';	/* trailing comma */
	queue_push(line);
}

static void queue_actuator(const struct actuator_state *s)
{
	char line[POINT_LEN];

	snprintk(line, sizeof(line),
		 "actuator_event,device_id=%s,actuator=%s state=%di,reason=\"%s\",run_s=%ui",
		 app_device_id(), app_actuator_name(s->id), s->on ? 1 : 0,
		 control_reason_name(s->reason), s->run_s);
	queue_push(line);
}

static void sync_time(void)
{
	struct sntp_time ts;

	if (k_uptime_get() < ntp_next_ms) {
		return;
	}

	int rc = sntp_simple(CONFIG_APP_BACKEND_NTP_SERVER, NTP_TIMEOUT_MS, &ts);

	if (rc == 0) {
		epoch_offset_ms = (int64_t)ts.seconds * 1000 - k_uptime_get();
		ntp_next_ms = k_uptime_get() + NTP_RESYNC_MS;
		LOG_INF("time synced");
	} else {
		LOG_WRN_RATELIMIT("no NTP: %d", rc);
		ntp_next_ms = k_uptime_get() + NTP_RETRY_MS;
	}
}

static int response_cb(struct http_response *rsp, enum http_final_call final, void *user_data)
{
	if (final == HTTP_DATA_FINAL) {
		http_status = rsp->http_status_code;
	}
	return 0;
}

static int connect_backend(void)
{
	char port[8];
	struct zsock_addrinfo hints = {.ai_family = AF_INET, .ai_socktype = SOCK_STREAM};
	struct zsock_addrinfo *res;

	snprintk(port, sizeof(port), "%d", CONFIG_APP_BACKEND_PORT);
	int rc = zsock_getaddrinfo(host, port, &hints, &res);

	if (rc != 0) {
		LOG_ERR_RATELIMIT("cannot resolve %s: %d", host, rc);
		return -EHOSTUNREACH;
	}

	int sock = zsock_socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (sock >= 0 && zsock_connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
		zsock_close(sock);
		sock = -errno;
	}
	zsock_freeaddrinfo(res);
	return sock;
}

static int post(int length)
{
	static char auth[sizeof(token) + 32];
	static const char url[] = "/api/v2/write?org=" CONFIG_APP_BACKEND_ORG
				  "&bucket=" CONFIG_APP_BACKEND_BUCKET "&precision=s";
	const char *headers[] = {auth, "Content-Type: text/plain\r\n", NULL};
	char port[8];
	struct http_request req = {
		.method = HTTP_POST,
		.url = url,
		.protocol = "HTTP/1.1",
		.host = host,
		.port = port,
		.header_fields = headers,
		.payload = body,
		.payload_len = length,
		.response = response_cb,
		.recv_buf = recv_buf,
		.recv_buf_len = sizeof(recv_buf),
	};

	snprintk(port, sizeof(port), "%d", CONFIG_APP_BACKEND_PORT);
	snprintk(auth, sizeof(auth), "Authorization: Token %s\r\n", token);

	int sock = connect_backend();

	if (sock < 0) {
		return sock;
	}

	http_status = 0;
	int rc = http_client_req(sock, &req, HTTP_TIMEOUT_MS, NULL);

	zsock_close(sock);
	if (rc < 0) {
		return rc;
	}
	return http_status >= 200 && http_status < 300 ? 0 : -EBADMSG;
}

/*
 * Without a clock, InfluxDB stamps points on receipt, so points of one series
 * sent together would overwrite each other. Send them one at a time then.
 */
static int build_body(int *points)
{
	int len = 0;
	int count = 0;

	k_mutex_lock(&queue_lock, K_FOREVER);
	while (count < queue_count) {
		const struct point *p = &queue[(queue_head + count) % ARRAY_SIZE(queue)];
		char stamp[24] = "";

		if (epoch_offset_ms != 0) {
			snprintk(stamp, sizeof(stamp), " %lld",
				 (p->uptime_ms + epoch_offset_ms) / 1000);
		}

		int n = snprintk(body + len, sizeof(body) - len, "%s%s\n", p->line, stamp);

		if (n >= (int)sizeof(body) - len) {
			break;
		}
		len += n;
		count++;
		if (epoch_offset_ms == 0) {
			break;
		}
	}
	k_mutex_unlock(&queue_lock);

	*points = count;
	return len;
}

static void queue_drop(int points)
{
	k_mutex_lock(&queue_lock, K_FOREVER);
	queue_head = (queue_head + points) % ARRAY_SIZE(queue);
	queue_count -= points;
	k_mutex_unlock(&queue_lock);
}

/* Returns true if everything queued was delivered. */
static bool upload_all(void)
{
	bool ok = true;

	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	sync_time();

	while (queue_count > 0) {
		int points;
		int length = build_body(&points);
		int rc = points > 0 ? post(length) : -EMSGSIZE;

		app_watchdog_feed(APP_MODULE_BACKEND);
		if (rc != 0) {
			LOG_ERR_RATELIMIT("upload failed: %d, HTTP %d", rc, http_status);
			app_status_set(APP_MODULE_BACKEND, APP_STATUS_ERROR);
			ok = false;
			break;
		}
		LOG_DBG("uploaded %d points", points);
		queue_drop(points);
		app_status_set(APP_MODULE_BACKEND, APP_STATUS_OK);
	}

	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	return ok;
}

static void backend_thread(void *a, void *b, void *c)
{
	int64_t next_upload = 0;

	while (true) {
		const struct zbus_channel *chan;
		union {
			struct sensor_data sensor;
			struct actuator_state actuator;
		} msg;

		if (zbus_sub_wait_msg(&backend_sub, &chan, &msg, K_SECONDS(APP_HEARTBEAT_S)) == 0) {
			if (chan == &sensor_data_chan) {
				queue_sensor(&msg.sensor);
			} else if (chan == &actuator_state_chan) {
				queue_actuator(&msg.actuator);
			}
		}
		app_watchdog_feed(APP_MODULE_BACKEND);

		if (host[0] == '\0' || token[0] == '\0') {
			app_status_set(APP_MODULE_BACKEND, APP_STATUS_DEBUG);
			continue;
		}
		if (app_status_get(APP_MODULE_NET) != APP_STATUS_OK || queue_count == 0 ||
		    k_uptime_get() < next_upload) {
			continue;
		}

		next_upload = k_uptime_get() + (upload_all() ? CONFIG_APP_BACKEND_UPLOAD_INTERVAL_S * 1000
							     : UPLOAD_RETRY_MS);
	}
}

K_THREAD_DEFINE(backend_tid, APP_BACKEND_STACK_SIZE, backend_thread, NULL, NULL, NULL,
		APP_THREAD_PRIORITY + 1, 0, SYS_FOREVER_MS);

int app_backend_init(void)
{
	return 0;
}

void app_backend_start(void)
{
	k_thread_start(backend_tid);
}

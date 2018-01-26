/*
 * Copyright (C) 2017 Jianhui Zhao <jianhuizhao329@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <umqtt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libubox/utils.h>
#include <libubox/ulog.h>

static void on_conack(struct umqtt_client *cl, uint8_t code)
{
    struct umqtt_topic topics[] = {
        {
            .len = strlen("test1"),
            .topic = "test1",
            .qos = 0x00
        },
        {
            .len = strlen("test2"),
            .topic = "test2",
            .qos = 0x01
        },
        {
            .len = strlen("test3"),
            .topic = "test3",
            .qos = 0x02
        }
    };

    ULOG_INFO("on_conack: %u\n", code);

    cl->publish(cl, "test2", "hello world", 1);
    cl->subscribe(cl, topics, ARRAY_SIZE(topics));
}

static void on_puback(struct umqtt_client *cl, uint16_t mid)
{
    ULOG_INFO("on_puback msg id: %u\n", mid);
}

static void on_pubrel(struct umqtt_client *cl, uint16_t mid)
{
    ULOG_INFO("on_pubrel msg id: %u\n", mid);
}

static void on_pubcomp(struct umqtt_client *cl, uint16_t mid)
{
    ULOG_INFO("on_pubcomp msg id: %u\n", mid);
}

static void on_unsuback(struct umqtt_client *cl, uint16_t mid)
{
    ULOG_INFO("on_unsuback msg id: %u\n", mid);
}

static void on_suback(struct umqtt_client *cl, uint16_t mid, uint8_t qos[], int num)
{
    int i;
    for (i = 0; i < num; i++)
        ULOG_INFO("on_suback msg id: %u, qos: 0x%02X\n", mid, qos[i]);
}

static void on_publish(struct umqtt_client *cl, const char *topic, struct umqtt_payload *payload)
{
    ULOG_INFO("on_publish: msd_id(%d) dup(%d) qos(%d) retain(%d) topic(%s) [%.*s]\n",
        payload->mid, payload->dup, payload->qos, payload->retain, topic, payload->len, payload->data);
}

static void on_error(struct umqtt_client *cl)
{
    ULOG_INFO("on_error: %u\n", cl->error);
}

static void on_close(struct umqtt_client *cl)
{
    ULOG_INFO("on_close\n");
    uloop_end();
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [option]\n"
        "      -h host      # Default is 'localhost'\n"
        "      -p port      # Default is 1883\n"
        "      -c file      # Load CA certificates from file\n"
        "      -n           # don't validate the server's certificate\n"
        "      -s           # Use ssl\n"
        , prog);
    exit(1);
}

int main(int argc, char **argv)
{
    int opt;
    struct umqtt_client *cl = NULL;
    static bool verify = true;
    const char *host = "localhost";
    int port = 1883;
    bool ssl = false;
    const char *crt_file = NULL;
    struct umqtt_options options = {
        .keep_alive = 12,
        .client_id = "LIBUMQTT"
    };

    while ((opt = getopt(argc, argv, "h:p:nc:s")) != -1) {
        switch (opt)
        {
        case 'h':
            host = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 's':
            ssl = true;
            break;
        case 'n':
            verify = false;
            break;
        case 'c':
            crt_file = optarg;
            break;
        default: /* '?' */
            usage(argv[0]);
        }
    }

    ULOG_INFO("libumqttc version %s\n", UMQTT_VERSION_STRING);

    uloop_init();

    cl = umqtt_new_ssl(host, port, ssl, crt_file, verify);
    if (!cl) {
        uloop_done();
        return -1;
    }
   
    cl->on_conack = on_conack;
    cl->on_puback = on_puback;
    cl->on_pubrel = on_pubrel;
    cl->on_suback = on_suback;
    cl->on_publish = on_publish;
    cl->on_pubcomp = on_pubcomp;
    cl->on_unsuback = on_unsuback;
    cl->on_error = on_error;
    cl->on_close = on_close;

    if (cl->connect(cl, &options, NULL) < 0) {
        ULOG_ERR("connect failed\n");
        goto err;
    }

    uloop_run();

err:
    cl->free(cl);

    uloop_done();
    
    return 0;
}
/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020, Erik Moqvist
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This file is part of the Messi project.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "chat_client.h"

static int line_length;
static char line_buf[128];
static bool connected = false;

static void on_connected(struct chat_client_t *self_p)
{
    struct chat_connect_req_t *message_p;

    message_p = chat_client_init_connect_req(self_p);
    message_p->user_p = self_p->user_p;
    chat_client_send(self_p);
}

static void on_disconnected(struct chat_client_t *self_p)
{
    (void)self_p;

    printf("Disconnected from the server.\n");

    line_length = 0;
    connected = false;
}

static void on_connect_rsp(struct chat_client_t *self_p,
                           struct chat_connect_rsp_t *message_p)
{
    (void)self_p;
    (void)message_p;

    printf("Connected to the server.\n");

    connected = true;
}

static void on_message_ind(struct chat_client_t *self_p,
                           struct chat_message_ind_t *message_p)
{
    (void)self_p;

    printf("<%s> %s\n", message_p->user_p, message_p->text_p);
}

static void send_message_ind(struct chat_client_t *self_p)
{
    struct chat_message_ind_t *message_p;

    message_p = chat_client_init_message_ind(self_p);
    message_p->user_p = self_p->user_p;
    message_p->text_p = &line_buf[0];
    chat_client_send(self_p);
}

static void user_input(struct chat_client_t *self_p)
{
    if (line_length == (sizeof(line_buf) - 1)) {
        line_length = 0;
    }

    read(STDIN_FILENO, &line_buf[line_length], 1);

    if (!connected) {
        return;
    }

    if (line_buf[line_length] == '\n') {
        line_buf[line_length] = '\0';
        send_message_ind(self_p);
        line_length = 0;
    } else {
        line_length++;
    }
}

static void parse_args(int argc,
                       const char *argv[],
                       const char **user_pp,
                       const char **uri_pp)
{
    if (argc < 2) {
        printf("usage: %s <user> [<uri>]\n", argv[0]);
        exit(1);
    }

    *user_pp = argv[1];

    if (argc == 3) {
        *uri_pp = argv[2];
    } else {
        *uri_pp = "tcp://127.0.0.1:6000";
    }
}

int main(int argc, const char *argv[])
{
    struct chat_client_t client;
    const char *user_p;
    const char *uri_p;
    int epoll_fd;
    struct epoll_event event;
    int res;
    uint8_t message[128];
    uint8_t workspace_in[128];
    uint8_t workspace_out[128];

    parse_args(argc, argv, &user_p, &uri_p);

    printf("Server URI: %s\n", uri_p);

    epoll_fd = epoll_create1(0);

    if (epoll_fd == -1) {
        return (1);
    }

    event.data.fd = STDIN_FILENO;
    event.events = EPOLLIN;

    res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &event);

    if (res != 0) {
        return (1);
    }

    line_length = 0;

    res = chat_client_init(&client,
                           user_p,
                           uri_p,
                           &message[0],
                           sizeof(message),
                           &workspace_in[0],
                           sizeof(workspace_in),
                           &workspace_out[0],
                           sizeof(workspace_out),
                           on_connected,
                           on_disconnected,
                           on_connect_rsp,
                           on_message_ind,
                           epoll_fd,
                           NULL);

    if (res != 0) {
        printf("Init failed.\n");

        return (1);
    }

    chat_client_start(&client);

    while (true) {
        res = epoll_wait(epoll_fd, &event, 1, -1);

        if (res != 1) {
            break;
        }

        if (event.data.fd == STDIN_FILENO) {
            user_input(&client);
        } else {
            chat_client_process(&client, event.data.fd, event.events);
        }
    }

    return (0);
}

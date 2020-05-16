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
#include <sys/epoll.h>
#include "my_protocol_client.h"

static void on_connected(struct my_protocol_client_t *self_p)
{
    printf("Connected, sending FooReq.\n");

    my_protocol_client_init_foo_req(self_p);
    my_protocol_client_send(self_p);
}

static void on_disconnected(struct my_protocol_client_t *self_p)
{
    (void)self_p;

    printf("Disconnected, exiting.\n");

    exit(0);
}

static void on_foo_rsp(struct my_protocol_client_t *self_p,
                       struct my_protocol_foo_rsp_t *message_p)
{
    (void)message_p;

    printf("Got FooRsp, sending BarInd twice.\n");

    my_protocol_client_init_bar_ind(self_p);
    my_protocol_client_send(self_p);
    my_protocol_client_send(self_p);
}

static void on_fie_req(struct my_protocol_client_t *self_p,
                       struct my_protocol_fie_req_t *message_p)
{
    (void)message_p;

    printf("Got FieReq, sending FieRsp.\n");

    my_protocol_client_init_fie_rsp(self_p);
    my_protocol_client_send(self_p);
}

int main()
{
    struct my_protocol_client_t client;
    int epoll_fd;
    struct epoll_event event;
    int res;
    uint8_t message[128];
    uint8_t workspace_in[128];
    uint8_t workspace_out[128];

    epoll_fd = epoll_create1(0);

    if (epoll_fd == -1) {
        return (1);
    }

    res = my_protocol_client_init(&client,
                                  "the-client",
                                  "tcp://127.0.0.1:7840",
                                  &message[0],
                                  sizeof(message),
                                  &workspace_in[0],
                                  sizeof(workspace_in),
                                  &workspace_out[0],
                                  sizeof(workspace_out),
                                  on_connected,
                                  on_disconnected,
                                  on_foo_rsp,
                                  on_fie_req,
                                  epoll_fd,
                                  NULL);

    if (res != 0) {
        printf("Init failed.\n");

        return (1);
    }

    my_protocol_client_start(&client);

    while (true) {
        res = epoll_wait(epoll_fd, &event, 1, -1);

        if (res != 1) {
            break;
        }

        my_protocol_client_process(&client, event.data.fd, event.events);
    }

    return (0);
}

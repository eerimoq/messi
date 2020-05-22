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
#include "my_protocol_server.h"

static void on_foo_req(struct my_protocol_server_t *self_p,
                       struct my_protocol_server_client_t *client_p,
                       struct my_protocol_foo_req_t *message_p)
{
    (void)client_p;
    (void)message_p;

    printf("Got FooReq. Sending FooRsp.\n");

    my_protocol_server_init_foo_rsp(self_p);
    my_protocol_server_reply(self_p);
}

static void on_bar_ind(struct my_protocol_server_t *self_p,
                       struct my_protocol_server_client_t *client_p,
                       struct my_protocol_bar_ind_t *message_p)
{
    (void)client_p;
    (void)message_p;

    static int count = 0;

    count++;

    if (count < 2) {
        printf("Got BarInd.\n");
    } else {
        printf("Got BarInd. Sending FieReq.\n");
        my_protocol_server_init_fie_req(self_p);
        my_protocol_server_reply(self_p);
    }
}

static void on_fie_rsp(struct my_protocol_server_t *self_p,
                       struct my_protocol_server_client_t *client_p,
                       struct my_protocol_fie_rsp_t *message_p)
{
    (void)client_p;
    (void)message_p;

    printf("Got FieRsp. Disconnecting the client and exiting.\n");

    my_protocol_server_disconnect(self_p, NULL);
    exit(0);
}

int main()
{
    struct my_protocol_server_t server;
    struct my_protocol_server_client_t clients[1];
    uint8_t clients_input_buffers[1][128];
    uint8_t message[128];
    uint8_t workspace_in[128];
    uint8_t workspace_out[128];
    int epoll_fd;
    struct epoll_event event;
    int res;

    epoll_fd = epoll_create1(0);

    if (epoll_fd == -1) {
        return (1);
    }

    res = my_protocol_server_init(&server,
                                  "tcp://127.0.0.1:7840",
                                  &clients[0],
                                  1,
                                  &clients_input_buffers[0][0],
                                  sizeof(clients_input_buffers[0]),
                                  &message[0],
                                  sizeof(message),
                                  &workspace_in[0],
                                  sizeof(workspace_in),
                                  &workspace_out[0],
                                  sizeof(workspace_out),
                                  NULL,
                                  NULL,
                                  on_foo_req,
                                  on_bar_ind,
                                  on_fie_rsp,
                                  epoll_fd,
                                  NULL);

    if (res != 0) {
        printf("Init failed.\n");

        return (1);
    }

    res = my_protocol_server_start(&server);

    if (res != 0) {
        printf("Start failed.\n");

        return (1);
    }

    printf("Server started.\n");

    while (true) {
        res = epoll_wait(epoll_fd, &event, 1, -1);

        if (res != 1) {
            break;
        }

        my_protocol_server_process(&server, event.data.fd, event.events);
    }

    return (1);
}

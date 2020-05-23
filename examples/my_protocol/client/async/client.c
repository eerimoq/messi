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
#include "client.h"

#define to_client(my_protocol_client_p) \
    async_container_of(my_protocol_client_p, struct client_t, client)

static void on_connected(struct my_protocol_client_t *self_p)
{
    printf("Connected. Sending FooReq.\n");

    my_protocol_client_init_foo_req(self_p);
    my_protocol_client_send(self_p);
}

static void on_disconnected(struct my_protocol_client_t *self_p,
                            enum messi_disconnect_reason_t disconnect_reason)
{
    (void)self_p;

    printf("Disconnected (reason: %s). Exiting.\n",
           messi_disconnect_reason_string(disconnect_reason));

    exit(0);

}

static void on_foo_rsp(struct my_protocol_client_t *self_p,
                       struct my_protocol_foo_rsp_t *message_p)
{
    (void)message_p;

    printf("Got FooRsp. Sending BarInd twice.\n");

    my_protocol_client_init_bar_ind(self_p);
    my_protocol_client_send(self_p);
    my_protocol_client_send(self_p);
}

static void on_fie_req(struct my_protocol_client_t *self_p,
                       struct my_protocol_fie_req_t *message_p)
{
    (void)message_p;

    printf("Got FieReq. Sending FieRsp.\n");

    my_protocol_client_init_fie_rsp(self_p);
    my_protocol_client_send(self_p);
}

void client_init(struct client_t *self_p, struct async_t *async_p)
{
    my_protocol_client_init(&self_p->client,
                            "tcp://127.0.0.1:7840",
                            &self_p->message[0],
                            sizeof(self_p->message),
                            &self_p->workspace_in[0],
                            sizeof(self_p->workspace_in),
                            &self_p->workspace_out[0],
                            sizeof(self_p->workspace_out),
                            on_connected,
                            on_disconnected,
                            on_foo_rsp,
                            on_fie_req,
                            async_p);
    my_protocol_client_start(&self_p->client);
}

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
#include "server.h"

static void on_client_disconnected(struct chat_server_t *self_p,
                                   struct chat_server_client_t *client_p)
{
    (void)self_p;
    (void)client_p;

    number_of_connected_clients--;

    printf("Number of connected clients: %d\n", number_of_connected_clients);
}

static void on_connect_req(struct chat_server_t *self_p,
                           struct chat_connect_req_t *message_p)
{
    printf("Client <%s> connected.\n", message_p->user_p);

    chat_server_init_connect_rsp(self_p);
    chat_server_send(self_p);
}

static void on_message_ind(struct chat_server_t *self_p,
                           struct chat_message_ind_t *message_in_p)
{
    struct chat_message_ind_t *message_p;

    message_p = chat_server_init_message_ind(&self_p->server);
    message_p->user_p = message_in_p->user_p;
    message_p->text_p = message_in_p->text_p;
    chat_server_broadcast(&self_p->server);
}

void server_init(struct server_t *self_p,
                 const char *uri_p,
                 struct async_t *async_p)
{
    chat_server_init(&self_p->server,
                     uri_p,
                     &self_p->clients[0],
                     10,
                     &self_p->clients_input_buffers[0][0],
                     sizeof(self_p->clients_input_buffers[0]),
                     &self_p->message[0],
                     sizeof(self_p->message),
                     &self_p->workspace_in[0],
                     sizeof(self_p->workspace_in),
                     &self_p->workspace_out[0],
                     sizeof(self_p->workspace_out),
                     NULL,
                     on_client_disconnected,
                     on_connect_req,
                     on_message_ind,
                     self_p,
                     async_p);
    chat_server_start(&self_p->server);

    printf("Server started.\n");
}

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

#define to_client(chat_client_p) \
    async_container_of(chat_client_p, struct client_t, client)

static void on_connected(struct chat_client_t *self_p)
{
    struct chat_connect_req_t *message_p;

    message_p = chat_client_init_connect_req(self_p);
    message_p->user_p = (char *)to_client(self_p)->user_p;
    chat_client_send(self_p);
}

static void on_disconnected(struct chat_client_t *self_p)
{
    printf("Disconnected from the server.\n");

    to_client(self_p)->connected = false;
}

static void on_connect_rsp(struct chat_client_t *self_p,
                           struct chat_connect_rsp_t *message_p)
{
    (void)message_p;

    printf("Connected to the server.\n");

    to_client(self_p)->connected = true;
}

static void on_message_ind(struct chat_client_t *self_p,
                           struct chat_message_ind_t *message_p)
{
    (void)self_p;

    printf("<%s> %s\n", message_p->user_p, message_p->text_p);
}

static void send_message(struct client_t *self_p)
{
    struct chat_message_ind_t *message_p;

    message_p = chat_client_init_message_ind(&self_p->client);
    message_p->user_p = (char *)self_p->user_p;
    message_p->text_p = &self_p->line.buf[0];
    chat_client_send(&self_p->client);
}

void client_init(struct client_t *self_p,
                 const char *user_p,
                 const char *server_uri_p,
                 struct async_t *async_p)
{
    self_p->user_p = user_p;
    self_p->line.length = 0;
    self_p->connected = false;
    chat_client_init(&self_p->client,
                     self_p->user_p,
                     server_uri_p,
                     &self_p->message[0],
                     sizeof(self_p->message),
                     &self_p->workspace_in[0],
                     sizeof(self_p->workspace_in),
                     &self_p->workspace_out[0],
                     sizeof(self_p->workspace_out),
                     on_connected,
                     on_disconnected,
                     on_connect_rsp,
                     on_message_ind,
                     async_p);
    chat_client_start(&self_p->client);
}

void client_user_input(struct async_threadsafe_data_t *data_p)
{
    struct client_t *self_p;

    self_p = (struct client_t *)data_p->obj_p;

    if (!self_p->connected) {
        return;
    }

    if (self_p->line.length == (sizeof(self_p->line.buf) - 1)) {
        self_p->line.length = 0;
    }

    self_p->line.buf[self_p->line.length] = data_p->data.buf[0];

    if (self_p->line.buf[self_p->line.length] == '\n') {
        self_p->line.buf[self_p->line.length] = '\0';
        send_message(self_p);
        self_p->line.length = 0;
    } else {
        self_p->line.length++;
    }
}

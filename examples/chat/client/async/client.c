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

static void on_connected(struct client_t *self_p)
{
    struct chat_connect_req_t *message_p;

    message_p = chat_client_init_connect_req(&self_p->client);
    message_p->user_p = self_p->user_p;
    chat_client_send(&self_p->client);
}

static void on_disconnected(struct client_t *self_p)
{
    printf("Disconnected from the server.\n");

    self_p->connected = false;
}

static void on_connect_rsp(struct client_t *self_p,
                           struct chat_connect_rsp_t *message_p)
{
    printf("Connected to the server.\n");

    self_p->connected = true;
}

static void on_message_ind(struct client_t *self_p,
                           struct chat_message_ind_t *message_p)
{
    printf("<%s> %s\n", message_p->user_p, message_p->text_p);
}

static void send_message(struct client_t *self_p)
{
    struct chat_message_ind_t *message_p;

    message_p = chat_client_init_message_ind(&self_p->client);
    message_p->user_p = self_p->user_p;
    message_p->text_p = self_p->line.buf[0];
    chat_client_send(&self_p->client);
}

void client_init(struct client_t *self_p,
                 const char *user_p,
                 const char *server_p,
                 struct async_t *async_p)
{
    self_p->user_p = user_p;
    self_p->line.length = 0;
    self_p->connected = false;
    chat_client_init(&self_p->client,
                     server_p,
                     (async_func_t)on_connected,
                     (async_func_t)on_disconnected,
                     on_connect_rsp,
                     on_message_ind,
                     self_p,
                     async_p);
    chat_client_start(&self_p->client);
}

void client_user_input(struct client_t *self_p, char *data_p)
{
    if (!self_p->connected) {
        free(data_p);

        return;
    }

    if (self_p->line.length == (sizeof(self_p->line.buf) - 1)) {
        self_p->line.length = 0;
    }

    self_p->line.buf[self_p->line.length] = *data_p;
    free(data_p);

    if (self_p->line.buf[self_p->line.length] == '\n') {
        self_p->line.buf[self_p->line.length] = '\0';
        send_message(self_p);
        self_p->line.length = 0;
    } else {
        self_p->line.length++;
    }
}

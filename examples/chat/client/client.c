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
 * This file is part of the Messager project.
 */

#include <stdio.h>
#include <string.h>
#include "client.h"

static void on_connected(struct client_t *self_p)
{
    printf("Connected.\n");
    async_timer_start(&self_p->send_message_timer);
}

static void on_disconnected(struct client_t *self_p)
{
    printf("Disconnected.\n");
    async_timer_stop(&self_p->send_message_timer);
}

static void on_message_ind(struct client_t *self_p,
                           struct chat_message_ind_t *message_p)
{
    printf("Received: %s\n", message_p->text_p);
}

static void on_send_message_timeout(struct client_t *self_p)
{
    char text[32];
    struct chat_message_ind_t *message_p;

    sprintf(&text[0], "count: %d", self_p->counter++);

    printf("Sending: %s\n", &text[0]);

    message_p = chat_client_init_message_ind(&self_p->client);
    message_p->text_p = &text[0];
    chat_client_send(&self_p->client);
}

void client_init(struct client_t *self_p,
                 const char *server_p,
                 struct async_t *async_p)
{
    self_p->counter = 0;
    chat_client_init(&self_p->client,
                     server_p,
                     (async_func_t)on_connected,
                     (async_func_t)on_disconnected,
                     on_message_ind,
                     self_p,
                     async_p);
    async_timer_init(&self_p->send_message_timer,
                     (async_timer_timeout_t)on_send_message_timeout,
                     self_p,
                     1000,
                     1000,
                     async_p);
    chat_client_start(&self_p->client);
}

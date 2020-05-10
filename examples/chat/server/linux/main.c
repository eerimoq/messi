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

#include "server.h"

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

    message_p = chat_server_init_message_ind(self_p);
    message_p->user_p = message_in_p->user_p;
    message_p->text_p = message_in_p->text_p;
    chat_server_broadcast(self_p);
}

int main()
{
    struct chat_server_t server;
    struct chat_server_client_t clients[10];
    int epoll_fd;

    chat_server_init(&server,
                     "tcp://127.0.0.1:6000",
                     on_connect_req,
                     on_message_ind,
                     epoll_fd);
    chat_server_start(&server);

    while (true) {
        res = epoll_wait(epoll_fd, &event, 1, -1);

        if (res != 1) {
            break;
        }

        if (char_server_has_file_descriptor(&server, event.fd)) {
            char_server_process(&server);
        }
    }

    return (0);
}

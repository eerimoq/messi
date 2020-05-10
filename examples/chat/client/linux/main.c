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

#include <sys/epoll.h>
#include "client.h"

static void parse_args(int argc,
                       const char *argv[],
                       const char **user_pp)
{
    if (argc != 2) {
        printf("usage: %s <user>\n", argv[0]);
        exit(1);
    }

    *user_pp = argv[1];
}

int main(int argc, const char *argv[])
{
    struct chat_client_t client;
    const char *user_p;
    int epoll_fd;
    struct epoll_event event;

    parse_args(argc, argv, &user_p);

    epoll_fd = epoll_create1(0);

    if (epoll_fd == -1) {
        return (1);
    }

    chat_client_init(&client,
                     "tcp://127.0.0.1:6000",
                     on_connect_rsp,
                     on_message_ind,
                     epoll_fd,
                     NULL);
    chat_client_start(&client);

    while (true) {
        res = epoll_wait(epoll_fd, &event, 1, -1);

        if (res != 1) {
            break;
        }

        if (chat_client_has_file_descriptor(&client, event.fd)) {
            chat_client_process(&client, event.fd, event.events);
        }
    }

    return (0);
}

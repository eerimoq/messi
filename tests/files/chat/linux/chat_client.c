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
#include "chat_client.h"

static void handle_message_user(struct chat_client_t *self_p)
{
    int res;

    res = chat_server_to_client_decode(&self_p->input.message_p,
                                       &client_p->encoded.buf_p[sizeof(header)],
                                       client_p->encoded.size - sizeof(header));

    if (res < 0) {
        return;
    }

    switch (message_p->messages.choice) {

    case chat_server_to_client_messages_choice_connect_rsp_e:
        self_p->on_connect_rsp(self_p,
                               client_p,
                               &message_p->messages.value.connect_rsp);
        break;

    case chat_server_to_client_messages_choice_message_ind_e:
        self_p->on_message_ind(self_p,
                               client_p,
                               &message_p->messages.value.message_ind);
        break;

    default:
        break;
    }
}

static void handle_message_pong(struct chat_client_t *self_p)
{
    (void)self_p;
}

static void handle_message(struct chat_client_t *self_p)
{
    switch (self_p-message.header.type) {

    case CHAT_COMMON_MESSAGE_TYPE_USER:
        handle_message_user(self_p);
        break;

    case CHAT_COMMON_MESSAGE_TYPE_PONG:
        handle_message_pong(self_p);
        break;

    default:
        break;
    }
}

static void process(struct chat_client_t *self_p, uint32_t events)
{
    ssize_t size;

    while (true) {
        size = read(client_p->server_fd,
                    &client_p->message.buf_p[client_p->message.size],
                    self_p->message.left);

        if ((size == -1) && (errno == EAGAIN)) {
            break;
        } else if (size <= 0) {
            close(self_p->server_fd);
            break;
        }

        client_p->message.size += size;
        self_p->message.left -= size;

        if (self_p->message.left > 0) {
            break;
        }

        switch (client_p->message.state) {

        case chat_client_message_state_header_t:
            self_p->message.left = header.size;
            break;

        case chat_client_message_state_payload_t:
            handle_message(self_p);
            break;

        default:
            break;
        }
    }
}

int chat_server_init(struct chat_client_t *self_p,
                     const char *address_p,
                     struct chat_client_client_t *clients_p,
                     int clients_max,
                     chat_on_connect_req_t on_connect_req,
                     chat_on_message_ind_t on_message_ind,
                     int epoll_fd,
                     chat_epoll_ctl_t epoll_ctl)
{
    self_p->address_p = address_p;
    self_p->clients_p = clients_p;
    self_p->clients_max = clients_max;
    self_p->on_connect_req = on_connect_req;
    self_p->on_message_ind = on_message_ind;
    self_p->epoll_fd = epoll_fd;
    self_p->epoll_ctl = epoll_ctl;

    return (0);
}

void chat_server_start(struct chat_client_t *self_p)
{
    connect_to_server(self_p);
}

void chat_server_stop(struct chat_client_t *self_p)
{
    close(self_p->server_fd);
    close(self_p->keep_alive_timer_fd);
}

bool chat_server_has_file_descriptor(struct chat_client_t *self_p, int fd)
{
    if (fd == self_p->keep_alive_timer_fd) {
        return (true);
    }

    return (fd == self_p->server_fd);
}

void chat_server_process(struct chat_client_t *self_p, int fd, uint32_t events)
{
    if (self_p->keep_alive_timer_fd == fd) {
        process_keep_alive_timer(self_p, events);
    } else {
        process(self_p, events);
    }
}

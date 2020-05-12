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

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include "chat_client.h"

static void reset_message(struct chat_client_t *self_p)
{
    self_p->message.state = chat_client_input_state_header_t;
    self_p->message.size = 0;
    self_p->message.left = sizeof(struct chat_common_header_t);
}

static void handle_message_user(struct chat_client_t *self_p)
{
    int res;
    struct chat_server_to_client_t *message_p;
    uint8_t *payload_buf_p;
    size_t payload_size;

    self_p->input.message_p = chat_server_to_client_new(
        &self_p->input.workspace.buf_p[0],
        self_p->input.workspace.size);
    message_p = self_p->input.message_p;

    if (message_p == NULL) {
        return;
    }

    payload_buf_p = &self_p->message.data.buf_p[sizeof(struct chat_common_header_t)];
    payload_size = self_p->message.data.size - sizeof(struct chat_common_header_t);

    res = chat_server_to_client_decode(message_p, payload_buf_p, payload_size);

    if (res != (int)payload_size) {
        return;
    }

    switch (message_p->messages.choice) {

    case chat_server_to_client_messages_choice_connect_rsp_e:
        self_p->on_connect_rsp(self_p,
                               &message_p->messages.value.connect_rsp);
        break;

    case chat_server_to_client_messages_choice_message_ind_e:
        self_p->on_message_ind(self_p,
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

static void handle_message(struct chat_client_t *self_p,
                           uint32_t type)
{
    switch (type) {

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
    (void)events;

    ssize_t size;
    struct chat_common_header_t *header_p;

    header_p = (struct chat_common_header_t *)self_p->message.data.buf_p;

    while (true) {
        size = read(self_p->server_fd,
                    &self_p->message.data.buf_p[self_p->message.size],
                    self_p->message.left);

        if ((size == -1) && (errno == EAGAIN)) {
            break;
        } else if (size <= 0) {
            close(self_p->server_fd);
            break;
        }

        self_p->message.size += size;
        self_p->message.left -= size;

        if (self_p->message.left > 0) {
            continue;
        }

        if (self_p->message.state == chat_client_input_state_header_t) {
            chat_common_header_ntoh(header_p);
            self_p->message.left = header_p->size;
            self_p->message.state = chat_client_input_state_payload_t;
        }

        if (self_p->message.left == 0) {
            handle_message(self_p, header_p->type);
            reset_message(self_p);
        }
    }
}

static void process_keep_alive_timer(struct chat_client_t *self_p, uint32_t events)
{
    (void)self_p;
    (void)events;
}

int chat_client_init(struct chat_client_t *self_p,
                     const char *server_p,
                     chat_client_on_connected_t on_connected,
                     chat_client_on_disconnected_t on_disconnected,
                     chat_on_connect_rsp_t on_connect_rsp,
                     chat_on_message_ind_t on_message_ind,
                     int epoll_fd,
                     chat_epoll_ctl_t epoll_ctl)
{
    self_p->server_p = server_p;
    self_p->on_connected = on_connected;
    self_p->on_disconnected = on_disconnected;
    self_p->on_connect_rsp = on_connect_rsp;
    self_p->on_message_ind = on_message_ind;
    self_p->epoll_fd = epoll_fd;
    self_p->epoll_ctl = epoll_ctl;

    return (0);
}

void chat_client_start(struct chat_client_t *self_p)
{
    (void)self_p;
    /* connect_to_server(self_p); */
}

void chat_client_stop(struct chat_client_t *self_p)
{
    close(self_p->server_fd);
    close(self_p->keep_alive_timer_fd);
}

void chat_client_process(struct chat_client_t *self_p, int fd, uint32_t events)
{
    if (self_p->keep_alive_timer_fd == fd) {
        process_keep_alive_timer(self_p, events);
    } else {
        process(self_p, events);
    }
}

void chat_client_send(struct chat_client_t *self_p)
{
    (void)self_p;
}

struct chat_connect_req_t *
chat_client_init_connect_req(struct chat_client_t *self_p)
{
    (void)self_p;

    return (NULL);
}

struct chat_message_ind_t *
chat_client_init_message_ind(struct chat_client_t *self_p)
{
    (void)self_p;

    return (NULL);
}

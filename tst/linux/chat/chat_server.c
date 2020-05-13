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
#include "chat_server.h"

static struct chat_server_client_t *alloc_client(struct chat_server_t *self_p)
{
    struct chat_server_client_t *client_p;

    client_p = self_p->clients.free_list_p;

    if (client_p != NULL) {
        /* Remove from free list. */
        self_p->clients.free_list_p = client_p->next_p;

        /* Add to used list. */
        client_p->next_p = self_p->clients.used_list_p;

        if (self_p->clients.used_list_p != NULL) {
            self_p->clients.used_list_p->prev_p = client_p;
        }

        self_p->clients.used_list_p = client_p;
    }

    return (client_p);
}

static void free_client(struct chat_server_t *self_p,
                        struct chat_server_client_t *client_p)
{
    /* Remove from used list. */
    if (client_p == self_p->clients.used_list_p) {
        self_p->clients.used_list_p = client_p->next_p;
    } else {
        client_p->prev_p->next_p = client_p->next_p;
    }

    if (client_p->next_p != NULL) {
        client_p->next_p->prev_p = client_p->prev_p;
    }

    /* Add to fre list. */
    client_p->next_p = self_p->clients.free_list_p;
    self_p->clients.free_list_p = client_p;
}

static void client_reset_input(struct chat_server_client_t *self_p)
{
    self_p->input.state = chat_server_client_input_state_header_t;
    self_p->input.size = 0;
    self_p->input.left = sizeof(struct chat_common_header_t);
}

static void close_socket(struct chat_server_t *self_p, int fd)
{
    self_p->epoll_ctl(self_p->epoll_fd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

static void process_listener(struct chat_server_t *self_p, uint32_t events)
{
    (void)events;

    int res;
    int client_fd;
    struct chat_server_client_t *client_p;

    client_fd = accept(self_p->listener_fd, NULL, 0);

    if (client_fd == -1) {
        return;
    }

    res = chat_common_make_non_blocking(client_fd);

    if (res == -1) {
        goto out;
    }

    client_p = alloc_client(self_p);

    if (client_p == NULL) {
        goto out;
    }

    client_p->fd = client_fd;
    client_reset_input(client_p);

    res = self_p->epoll_ctl(self_p->epoll_fd, EPOLL_CTL_ADD, client_fd, EPOLLIN);

    if (res == -1) {
        goto out;
    }

    return;

 out:

    close(client_fd);
}

static void handle_message_user(struct chat_server_t *self_p,
                                struct chat_server_client_t *client_p)
{
    int res;
    struct chat_client_to_server_t *message_p;
    uint8_t *payload_buf_p;
    size_t payload_size;

    self_p->input.message_p = chat_client_to_server_new(
        &self_p->input.workspace.buf_p[0],
        self_p->input.workspace.size);
    message_p = self_p->input.message_p;

    if (message_p == NULL) {
        return;
    }

    payload_buf_p = &client_p->input.buf_p[sizeof(struct chat_common_header_t)];
    payload_size = client_p->input.size - sizeof(struct chat_common_header_t);

    res = chat_client_to_server_decode(message_p, payload_buf_p, payload_size);

    if (res != (int)payload_size) {
        return;
    }

    self_p->current_client_p = client_p;

    switch (message_p->messages.choice) {

    case chat_client_to_server_messages_choice_connect_req_e:
        self_p->on_connect_req(self_p,
                               client_p,
                               &message_p->messages.value.connect_req);
        break;

    case chat_client_to_server_messages_choice_message_ind_e:
        self_p->on_message_ind(self_p,
                               client_p,
                               &message_p->messages.value.message_ind);
        break;

    default:
        break;
    }
}

static void handle_message_ping(struct chat_server_t *self_p,
                                struct chat_server_client_t *client_p)
{
    (void)self_p;
    (void)client_p;
}

static void handle_message(struct chat_server_t *self_p,
                           struct chat_server_client_t *client_p,
                           uint32_t type)
{
    switch (type) {

    case CHAT_COMMON_MESSAGE_TYPE_USER:
        handle_message_user(self_p, client_p);
        break;

    case CHAT_COMMON_MESSAGE_TYPE_PING:
        handle_message_ping(self_p, client_p);
        break;

    default:
        break;
    }
}

static void process_client(struct chat_server_t *self_p,
                           struct chat_server_client_t *client_p,
                           uint32_t events)
{
    (void)events;

    ssize_t size;
    struct chat_common_header_t *header_p;

    header_p = (struct chat_common_header_t *)client_p->input.buf_p;

    while (true) {
        size = read(client_p->fd,
                    &client_p->input.buf_p[client_p->input.size],
                    client_p->input.left);

        if (size <= 0) {
            if (!((size == -1) && (errno == EAGAIN))) {
                close_socket(self_p, client_p->fd);
                free_client(self_p, client_p);
            }

            break;
        }

        client_p->input.size += size;
        client_p->input.left -= size;

        if (client_p->input.left > 0) {
            continue;
        }

        if (client_p->input.state == chat_server_client_input_state_header_t) {
            chat_common_header_ntoh(header_p);
            client_p->input.left = header_p->size;
            client_p->input.state = chat_server_client_input_state_payload_t;
        }

        if (client_p->input.left == 0) {
            handle_message(self_p, client_p, header_p->type);
            client_reset_input(client_p);
        }
    }
}

static void on_connect_req_default(struct chat_server_t *self_p,
                                   struct chat_server_client_t *client_p,
                                   struct chat_connect_req_t *message_p)
{
    (void)self_p;
    (void)client_p;
    (void)message_p;
}

static void on_message_ind_default(struct chat_server_t *self_p,
                                   struct chat_server_client_t *client_p,
                                   struct chat_message_ind_t *message_p)
{
    (void)self_p;
    (void)client_p;
    (void)message_p;
}

static void create_user_header(struct chat_server_t *self_p, size_t payload_size)
{
    struct chat_common_header_t *header_p;

    header_p = (struct chat_common_header_t *)self_p->message.data.buf_p;
    header_p->type = CHAT_COMMON_MESSAGE_TYPE_USER;
    header_p->size = payload_size;
    chat_common_header_hton(header_p);
}

int chat_server_init(struct chat_server_t *self_p,
                     const char *address_p,
                     struct chat_server_client_t *clients_p,
                     int clients_max,
                     uint8_t *clients_input_bufs_p,
                     size_t client_input_size,
                     uint8_t *message_buf_p,
                     size_t message_size,
                     uint8_t *workspace_in_buf_p,
                     size_t workspace_in_size,
                     uint8_t *workspace_out_buf_p,
                     size_t workspace_out_size,
                     chat_on_connect_req_t on_connect_req,
                     chat_on_message_ind_t on_message_ind,
                     int epoll_fd,
                     chat_epoll_ctl_t epoll_ctl)
{
    (void)clients_max;

    int i;

    if (on_connect_req == NULL) {
        on_connect_req = on_connect_req_default;
    }

    if (on_message_ind == NULL) {
        on_message_ind = on_message_ind_default;
    }

    if (epoll_ctl == NULL) {
        epoll_ctl = chat_common_epoll_ctl_default;
    }

    self_p->address_p = address_p;
    self_p->on_connect_req = on_connect_req;
    self_p->on_message_ind = on_message_ind;
    self_p->epoll_fd = epoll_fd;
    self_p->epoll_ctl = epoll_ctl;

    /* Lists of clients. */
    self_p->clients.free_list_p = &clients_p[0];
    self_p->clients.input_buffer_size = client_input_size;

    for (i = 0; i < clients_max - 1; i++) {
        clients_p[i].next_p = &clients_p[i + 1];
        clients_p[i].input.buf_p = &clients_input_bufs_p[i * client_input_size];
    }

    clients_p[i].next_p = NULL;
    clients_p[i].input.buf_p = &clients_input_bufs_p[i * client_input_size];
    self_p->clients.used_list_p = NULL;

    self_p->message.data.buf_p = message_buf_p;
    self_p->message.data.size = message_size;
    self_p->input.workspace.buf_p = workspace_in_buf_p;
    self_p->input.workspace.size = workspace_in_size;
    self_p->output.workspace.buf_p = workspace_out_buf_p;
    self_p->output.workspace.size = workspace_out_size;

    return (0);
}

int chat_server_start(struct chat_server_t *self_p)
{
    int res;
    int listener_fd;
    struct sockaddr_in addr;
    int enable;

    listener_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (listener_fd == -1) {
        return (1);
    }

    enable = 1;

    res = setsockopt(listener_fd,
                     SOL_SOCKET,
                     SO_REUSEADDR,
                     &enable,
                     sizeof(enable));

    if (res != 0) {
        goto out;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(6000);
    inet_aton("127.0.0.1", (struct in_addr *)&addr.sin_addr.s_addr);

    res = bind(listener_fd, (struct sockaddr *)&addr, sizeof(addr));

    if (res == -1) {
        goto out;
    }

    res = listen(listener_fd, 5);

    if (res == -1) {
        goto out;
    }

    res = chat_common_make_non_blocking(listener_fd);

    if (res == -1) {
        goto out;
    }

    res = self_p->epoll_ctl(self_p->epoll_fd,
                            EPOLL_CTL_ADD,
                            listener_fd,
                            EPOLLIN);

    if (res == -1) {
        goto out;
    }

    self_p->listener_fd = listener_fd;

    return (0);

 out:

    close(listener_fd);

    return (-1);
}

void chat_server_stop(struct chat_server_t *self_p)
{
    struct chat_server_client_t *client_p;
    struct chat_server_client_t *next_client_p;

    close_socket(self_p, self_p->listener_fd);
    client_p = self_p->clients.used_list_p;

    while (client_p != NULL) {
        close_socket(self_p, client_p->fd);
        next_client_p = client_p->next_p;
        client_p->next_p = self_p->clients.free_list_p;
        self_p->clients.free_list_p = client_p;
        client_p = next_client_p;
    }
}

void chat_server_process(struct chat_server_t *self_p, int fd, uint32_t events)
{
    struct chat_server_client_t *client_p;

    if (self_p->listener_fd == fd) {
        process_listener(self_p, events);
    } else {
        client_p = self_p->clients.used_list_p;

        while (client_p != NULL) {
            if (client_p->fd == fd) {
                process_client(self_p, client_p, events);
                break;
            }

            client_p = client_p->next_p;
        }
    }
}

void chat_server_send(struct chat_server_t *self_p)
{
    int res;

    res = chat_server_to_client_encode(self_p->output.message_p,
                                       self_p->message.data.buf_p,
                                       self_p->message.data.size);

    if (res < 0) {
        return;
    }
}

void chat_server_reply(struct chat_server_t *self_p)
{
    int res;

    res = chat_server_to_client_encode(self_p->output.message_p,
                                       &self_p->message.data.buf_p[8],
                                       self_p->message.data.size - 8);

    if (res < 0) {
        return;
    }

    create_user_header(self_p, res);
    write(self_p->current_client_p->fd, self_p->message.data.buf_p, res + 8);
}

void chat_server_broadcast(struct chat_server_t *self_p)
{
    int res;
    struct chat_server_client_t *client_p;

    /* Create the message. */
    res = chat_server_to_client_encode(self_p->output.message_p,
                                       &self_p->message.data.buf_p[8],
                                       self_p->message.data.size);

    if (res < 0) {
        return;
    }

    create_user_header(self_p, res);

    /* Send it to all clients. */
    client_p = self_p->clients.used_list_p;

    while (client_p != NULL) {
        write(client_p->fd, self_p->message.data.buf_p, res + 8);
        client_p = client_p->next_p;
    }
}

struct chat_connect_rsp_t *
chat_server_init_connect_rsp(struct chat_server_t *self_p)
{
    self_p->output.message_p = chat_server_to_client_new(
        &self_p->output.workspace.buf_p[0],
        self_p->output.workspace.size);
    chat_server_to_client_messages_connect_rsp_init(self_p->output.message_p);

    return (&self_p->output.message_p->messages.value.connect_rsp);
}

struct chat_message_ind_t *
chat_server_init_message_ind(struct chat_server_t *self_p)
{
    self_p->output.message_p = chat_server_to_client_new(
        &self_p->output.workspace.buf_p[0],
        self_p->output.workspace.size);
    chat_server_to_client_messages_message_ind_init(self_p->output.message_p);

    return (&self_p->output.message_p->messages.value.message_ind);
}
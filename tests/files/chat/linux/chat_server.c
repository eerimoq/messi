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
#include <sys/timerfd.h>
#include "messi.h"
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

static int epoll_ctl_add(struct chat_server_t *self_p, int fd)
{
    return (self_p->epoll_ctl(self_p->epoll_fd, EPOLL_CTL_ADD, fd, EPOLLIN));
}

static int epoll_ctl_del(struct chat_server_t *self_p, int fd)
{
    return (self_p->epoll_ctl(self_p->epoll_fd, EPOLL_CTL_DEL, fd, 0));
}

static void close_fd(struct chat_server_t *self_p, int fd)
{
    epoll_ctl_del(self_p, fd);
    close(fd);
}

static void client_reset_input(struct chat_server_client_t *self_p)
{
    self_p->input.state = chat_server_client_input_state_header_t;
    self_p->input.size = 0;
    self_p->input.left = sizeof(struct chat_common_header_t);
}

static int client_start_keep_alive_timer(struct chat_server_client_t *self_p)
{
    struct itimerspec timeout;

    memset(&timeout, 0, sizeof(timeout));
    timeout.it_value.tv_sec = 3;

    return (timerfd_settime(self_p->keep_alive_timer_fd, 0, &timeout, NULL));
}

static int client_init(struct chat_server_client_t *self_p,
                       struct chat_server_t *server_p,
                       int client_fd)
{
    int res;

    self_p->client_fd = client_fd;
    client_reset_input(self_p);
    self_p->keep_alive_timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);

    if (self_p->keep_alive_timer_fd == -1) {
        return (-1);
    }

    res = client_start_keep_alive_timer(self_p);

    if (res == -1) {
        goto out1;
    }

    res = epoll_ctl_add(server_p, self_p->keep_alive_timer_fd);

    if (res == -1) {
        goto out1;
    }

    res = epoll_ctl_add(server_p, client_fd);

    if (res == -1) {
        goto out2;
    }

    return (0);

 out2:
    epoll_ctl_del(server_p, self_p->keep_alive_timer_fd);

 out1:
    close(self_p->keep_alive_timer_fd);

    return (-1);
}

static void client_destroy(struct chat_server_client_t *self_p,
                           struct chat_server_t *server_p)
{
    close_fd(server_p, self_p->client_fd);
    close_fd(server_p, self_p->keep_alive_timer_fd);
    server_p->on_client_disconnected(server_p, self_p);
    free_client(server_p, self_p);

    if (self_p == server_p->current_client_p) {
        server_p->current_client_p = NULL;
    }
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
        goto out1;
    }

    client_p = alloc_client(self_p);

    if (client_p == NULL) {
        goto out1;
    }

    res = client_init(client_p, self_p, client_fd);

    if (res != 0) {
        goto out2;
    }

    self_p->on_client_connected(self_p, client_p);

    return;

 out2:
    free_client(self_p, client_p);

 out1:
    close(client_fd);
}

static int handle_message_user(struct chat_server_t *self_p,
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
        return (-1);
    }

    payload_buf_p = &client_p->input.buf_p[sizeof(struct chat_common_header_t)];
    payload_size = client_p->input.size - sizeof(struct chat_common_header_t);

    res = chat_client_to_server_decode(message_p, payload_buf_p, payload_size);

    if (res != (int)payload_size) {
        return (-1);
    }

    self_p->current_client_p = client_p;

    switch (message_p->messages.choice) {

    case chat_client_to_server_messages_choice_connect_req_e:
        self_p->on_connect_req(
            self_p,
            client_p,
            &message_p->messages.value.connect_req);
        break;

    case chat_client_to_server_messages_choice_message_ind_e:
        self_p->on_message_ind(
            self_p,
            client_p,
            &message_p->messages.value.message_ind);
        break;

    default:
        break;
    }

    self_p->current_client_p = NULL;

    return (0);
}

static int handle_message_ping(struct chat_server_client_t *client_p)
{
    int res;
    ssize_t size;
    struct chat_common_header_t header;

    res = client_start_keep_alive_timer(client_p);

    if (res != 0) {
        return (res);
    }

    header.type = CHAT_COMMON_MESSAGE_TYPE_PONG;
    header.size = 0;
    chat_common_header_hton(&header);

    size = write(client_p->client_fd, &header, sizeof(header));

    if (size != sizeof(header)) {
        return (-1);
    }

    return (0);
}

static int handle_message(struct chat_server_t *self_p,
                          struct chat_server_client_t *client_p,
                          uint32_t type)
{
    int res;

    switch (type) {

    case CHAT_COMMON_MESSAGE_TYPE_USER:
        res = handle_message_user(self_p, client_p);
        break;

    case CHAT_COMMON_MESSAGE_TYPE_PING:
        res = handle_message_ping(client_p);
        break;

    default:
        res = -1;
        break;
    }

    return (res);
}

static void process_client_socket(struct chat_server_t *self_p,
                                  struct chat_server_client_t *client_p)
{
    int res;
    ssize_t size;
    struct chat_common_header_t *header_p;

    header_p = (struct chat_common_header_t *)client_p->input.buf_p;

    while (true) {
        size = read(client_p->client_fd,
                    &client_p->input.buf_p[client_p->input.size],
                    client_p->input.left);

        if (size <= 0) {
            if (!((size == -1) && (errno == EAGAIN))) {
                client_destroy(client_p, self_p);
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
            res = handle_message(self_p, client_p, header_p->type);

            if (res == 0) {
                client_reset_input(client_p);
            } else {
                client_destroy(client_p, self_p);
            }
        }
    }
}

static void process_client_keep_alive_timer(struct chat_server_t *self_p,
                                            struct chat_server_client_t *client_p)
{
    client_destroy(client_p, self_p);
}

static void on_connect_req_default(
    struct chat_server_t *self_p,
    struct chat_server_client_t *client_p,
    struct chat_connect_req_t *message_p)
{
    (void)self_p;
    (void)client_p;
    (void)message_p;
}
static void on_message_ind_default(
    struct chat_server_t *self_p,
    struct chat_server_client_t *client_p,
    struct chat_message_ind_t *message_p)
{
    (void)self_p;
    (void)client_p;
    (void)message_p;
}

static int encode_user_message(struct chat_server_t *self_p)
{
    int payload_size;
    struct chat_common_header_t *header_p;

    payload_size = chat_server_to_client_encode(
        self_p->output.message_p,
        &self_p->message.data.buf_p[sizeof(*header_p)],
        self_p->message.data.size - sizeof(*header_p));

    if (payload_size < 0) {
        return (payload_size);
    }

    header_p = (struct chat_common_header_t *)self_p->message.data.buf_p;
    header_p->type = CHAT_COMMON_MESSAGE_TYPE_USER;
    header_p->size = payload_size;
    chat_common_header_hton(header_p);

    return (payload_size + sizeof(*header_p));
}

static void on_client_connected_default(struct chat_server_t *self_p,
                                        struct chat_server_client_t *client_p)
{
        (void)self_p;
        (void)client_p;
}

static void on_client_disconnected_default(struct chat_server_t *self_p,
                                           struct chat_server_client_t *client_p)
{
        (void)self_p;
        (void)client_p;
}

int chat_server_init(
    struct chat_server_t *self_p,
    const char *server_uri_p,
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
    chat_server_on_client_connected_t on_client_connected,
    chat_server_on_client_disconnected_t on_client_disconnected,
    chat_server_on_connect_req_t on_connect_req,
    chat_server_on_message_ind_t on_message_ind,
    int epoll_fd,
    chat_epoll_ctl_t epoll_ctl)
{
    (void)clients_max;

    int i;
    int res;

    if (on_connect_req == NULL) {
        on_connect_req = on_connect_req_default;
    }

    if (on_message_ind == NULL) {
        on_message_ind = on_message_ind_default;
    }

    if (on_client_connected == NULL) {
        on_client_connected = on_client_connected_default;
    }

    if (on_client_disconnected == NULL) {
        on_client_disconnected = on_client_disconnected_default;
    }

    if (epoll_ctl == NULL) {
        epoll_ctl = chat_common_epoll_ctl_default;
    }

    res = messi_parse_tcp_uri(server_uri_p,
                              &self_p->server.address[0],
                              sizeof(self_p->server.address),
                              &self_p->server.port);

    if (res != 0) {
        return (res);
    }

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
    self_p->on_client_connected = on_client_connected;
    self_p->on_client_disconnected = on_client_disconnected;
    self_p->current_client_p = NULL;

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
    addr.sin_port = htons((short)self_p->server.port);
    inet_aton(&self_p->server.address[0], (struct in_addr *)&addr.sin_addr.s_addr);

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

    res = epoll_ctl_add(self_p, listener_fd);

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

    close_fd(self_p, self_p->listener_fd);
    client_p = self_p->clients.used_list_p;

    while (client_p != NULL) {
        close_fd(self_p, client_p->client_fd);
        close_fd(self_p, client_p->keep_alive_timer_fd);
        next_client_p = client_p->next_p;
        client_p->next_p = self_p->clients.free_list_p;
        self_p->clients.free_list_p = client_p;
        client_p = next_client_p;
    }
}

void chat_server_process(struct chat_server_t *self_p, int fd, uint32_t events)
{
    struct chat_server_client_t *client_p;

    if (fd == self_p->listener_fd) {
        process_listener(self_p, events);
    } else {
        client_p = self_p->clients.used_list_p;

        while (client_p != NULL) {
            if (fd == client_p->client_fd) {
                process_client_socket(self_p, client_p);
                break;
            } else if (fd == client_p->keep_alive_timer_fd) {
                process_client_keep_alive_timer(self_p, client_p);
                break;
            }

            client_p = client_p->next_p;
        }
    }
}

void chat_server_send(struct chat_server_t *self_p,
                        struct chat_server_client_t *client_p)
{
    int res;
    ssize_t size;

    res = encode_user_message(self_p);

    if (res < 0) {
        return;
    }

    size = write(client_p->client_fd,
                 self_p->message.data.buf_p,
                 res);

    if (size != res) {
        client_destroy(client_p, self_p);
    }
}

void chat_server_reply(struct chat_server_t *self_p)
{
    if (self_p->current_client_p != NULL) {
        chat_server_send(self_p, self_p->current_client_p);
    }
}

void chat_server_broadcast(struct chat_server_t *self_p)
{
    int res;
    ssize_t size;
    struct chat_server_client_t *client_p;
    struct chat_server_client_t *next_client_p;

    /* Create the message. */
    res = encode_user_message(self_p);

    if (res < 0) {
        return;
    }

    /* Send it to all clients. */
    client_p = self_p->clients.used_list_p;

    while (client_p != NULL) {
        size = write(client_p->client_fd, self_p->message.data.buf_p, res);
        next_client_p = client_p->next_p;

        if (size != res) {
            client_destroy(client_p, self_p);
        }

        client_p = next_client_p;
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


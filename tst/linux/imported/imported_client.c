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
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include "imported_client.h"

static int epoll_ctl_add(struct imported_client_t *self_p, int fd)
{
    return (self_p->epoll_ctl(self_p->epoll_fd, EPOLL_CTL_ADD, fd, EPOLLIN));
}

static void epoll_ctl_del(struct imported_client_t *self_p, int fd)
{
    self_p->epoll_ctl(self_p->epoll_fd, EPOLL_CTL_DEL, fd, 0);
}

static void close_fd(struct imported_client_t *self_p, int fd)
{
    epoll_ctl_del(self_p, fd);
    close(fd);
}

static void reset_message(struct imported_client_t *self_p)
{
    self_p->message.state = imported_client_input_state_header_t;
    self_p->message.size = 0;
    self_p->message.left = sizeof(struct imported_common_header_t);
}

static void handle_message_user(struct imported_client_t *self_p)
{
    int res;
    struct imported_server_to_client_t *message_p;
    uint8_t *payload_buf_p;
    size_t payload_size;

    self_p->input.message_p = imported_server_to_client_new(
        &self_p->input.workspace.buf_p[0],
        self_p->input.workspace.size);
    message_p = self_p->input.message_p;

    if (message_p == NULL) {
        return;
    }

    payload_buf_p = &self_p->message.data.buf_p[sizeof(struct imported_common_header_t)];
    payload_size = self_p->message.size - sizeof(struct imported_common_header_t);

    res = imported_server_to_client_decode(message_p, payload_buf_p, payload_size);

    if (res != (int)payload_size) {
        return;
    }

    switch (message_p->messages.choice) {

    case imported_server_to_client_messages_choice_bar_e:
        self_p->on_bar(
            self_p,
            &message_p->messages.value.bar);
        break;

    default:
        break;
    }
}

static void handle_message_pong(struct imported_client_t *self_p)
{
    self_p->pong_received = true;
}

static void handle_message(struct imported_client_t *self_p,
                           uint32_t type)
{
    switch (type) {

    case IMPORTED_COMMON_MESSAGE_TYPE_USER:
        handle_message_user(self_p);
        break;

    case IMPORTED_COMMON_MESSAGE_TYPE_PONG:
        handle_message_pong(self_p);
        break;

    default:
        break;
    }
}

static int start_timer(int fd, int seconds)
{
    struct itimerspec timeout;

    memset(&timeout, 0, sizeof(timeout));
    timeout.it_value.tv_sec = seconds;

    return (timerfd_settime(fd, 0, &timeout, NULL));
}

static void disconnect(struct imported_client_t *self_p)
{
    close_fd(self_p, self_p->server_fd);
    self_p->server_fd = -1;
    close_fd(self_p, self_p->keep_alive_timer_fd);
    self_p->keep_alive_timer_fd = -1;
}

static int start_keep_alive_timer(struct imported_client_t *self_p)
{
    return (start_timer(self_p->keep_alive_timer_fd, 2));
}

static int start_reconnect_timer(struct imported_client_t *self_p)
{
    int res;

    self_p->reconnect_timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);

    if (self_p->reconnect_timer_fd == -1) {
        return (-1);
    }

    res = epoll_ctl_add(self_p, self_p->reconnect_timer_fd);

    if (res == -1) {
        goto out;
    }

    return (start_timer(self_p->reconnect_timer_fd, 1));

 out:
    close(self_p->reconnect_timer_fd);
    self_p->reconnect_timer_fd = -1;

    return (-1);
}

static void process_socket(struct imported_client_t *self_p, uint32_t events)
{
    (void)events;

    ssize_t size;
    struct imported_common_header_t *header_p;

    header_p = (struct imported_common_header_t *)self_p->message.data.buf_p;

    while (true) {
        size = read(self_p->server_fd,
                    &self_p->message.data.buf_p[self_p->message.size],
                    self_p->message.left);

        if ((size == -1) && (errno == EAGAIN)) {
            break;
        } else if (size <= 0) {
            disconnect(self_p);
            self_p->on_disconnected(self_p);
            start_reconnect_timer(self_p);
            break;
        }

        self_p->message.size += size;
        self_p->message.left -= size;

        if (self_p->message.left > 0) {
            continue;
        }

        if (self_p->message.state == imported_client_input_state_header_t) {
            imported_common_header_ntoh(header_p);
            self_p->message.left = header_p->size;
            self_p->message.state = imported_client_input_state_payload_t;
        }

        if (self_p->message.left == 0) {
            handle_message(self_p, header_p->type);
            reset_message(self_p);
        }
    }
}

static void process_keep_alive_timer(struct imported_client_t *self_p, uint32_t events)
{
    (void)events;

    int res;
    struct imported_common_header_t header;
    ssize_t size;
    uint64_t value;

    size = read(self_p->keep_alive_timer_fd, &value, sizeof(value));

    if (size != sizeof(value)) {
        disconnect(self_p);
        self_p->on_disconnected(self_p);

        return;
    }

    if (!self_p->pong_received) {
        disconnect(self_p);
        self_p->on_disconnected(self_p);
        start_reconnect_timer(self_p);

        return;
    }

    res = start_keep_alive_timer(self_p);

    if (res == 0) {
        header.type = IMPORTED_COMMON_MESSAGE_TYPE_PING;
        header.size = 0;
        imported_common_header_hton(&header);
        write(self_p->server_fd, &header, sizeof(header));
        self_p->pong_received = false;
    } else {
        disconnect(self_p);
        self_p->on_disconnected(self_p);
    }
}

static int connect_to_server(struct imported_client_t *self_p)
{
    int res;
    int server_fd;
    struct sockaddr_in addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        return (-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(6000);
    inet_aton("127.0.0.1", (struct in_addr *)&addr.sin_addr.s_addr);

    res = connect(server_fd, (struct sockaddr *)&addr, sizeof(addr));

    if (res == -1) {
        goto out1;
    }

    res = imported_common_make_non_blocking(server_fd);

    if (res == -1) {
        goto out1;
    }

    res = epoll_ctl_add(self_p, server_fd);

    if (res == -1) {
        goto out1;
    }

    self_p->keep_alive_timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);

    if (self_p->keep_alive_timer_fd == -1) {
        goto out2;
    }

    res = epoll_ctl_add(self_p, self_p->keep_alive_timer_fd);

    if (res == -1) {
        goto out3;
    }

    res = start_keep_alive_timer(self_p);

    if (res != 0) {
        goto out4;
    }

    self_p->server_fd = server_fd;
    self_p->pong_received = true;
    self_p->on_connected(self_p);

    return (0);

 out4:
    epoll_ctl_del(self_p, self_p->keep_alive_timer_fd);

 out3:
    close(self_p->keep_alive_timer_fd);

 out2:
    epoll_ctl_del(self_p, server_fd);

 out1:
    close(server_fd);

    return (-1);
}

static void process_reconnect_timer(struct imported_client_t *self_p)
{
    close_fd(self_p, self_p->reconnect_timer_fd);
    self_p->reconnect_timer_fd = -1;

    if (connect_to_server(self_p) != 0) {
        start_reconnect_timer(self_p);
    }
}

static void on_bar_default(
    struct imported_client_t *self_p,
    struct types_bar_t *message_p)
{
    (void)self_p;
    (void)message_p;
}

int imported_client_init(
    struct imported_client_t *self_p,
    const char *user_p,
    const char *server_p,
    uint8_t *message_buf_p,
    size_t message_size,
    uint8_t *workspace_in_buf_p,
    size_t workspace_in_size,
    uint8_t *workspace_out_buf_p,
    size_t workspace_out_size,
    imported_client_on_connected_t on_connected,
    imported_client_on_disconnected_t on_disconnected,
    imported_client_on_bar_t on_bar,
    int epoll_fd,
    imported_epoll_ctl_t epoll_ctl)
{
    if (on_bar == NULL) {
        on_bar = on_bar_default;
    }

    if (epoll_ctl == NULL) {
        epoll_ctl = imported_common_epoll_ctl_default;
    }

    self_p->user_p = (char *)user_p;
    self_p->server_p = server_p;
    self_p->on_connected = on_connected;
    self_p->on_disconnected = on_disconnected;
    self_p->on_bar = on_bar;
    self_p->epoll_fd = epoll_fd;
    self_p->epoll_ctl = epoll_ctl;
    self_p->message.data.buf_p = message_buf_p;
    self_p->message.data.size = message_size;
    reset_message(self_p);
    self_p->input.workspace.buf_p = workspace_in_buf_p;
    self_p->input.workspace.size = workspace_in_size;
    self_p->output.workspace.buf_p = workspace_out_buf_p;
    self_p->output.workspace.size = workspace_out_size;
    self_p->server_fd = -1;
    self_p->keep_alive_timer_fd = -1;
    self_p->reconnect_timer_fd = -1;

    return (0);
}

void imported_client_start(struct imported_client_t *self_p)
{
    if (connect_to_server(self_p) != 0) {
        start_reconnect_timer(self_p);
    }
}

void imported_client_stop(struct imported_client_t *self_p)
{
    disconnect(self_p);

    if (self_p->reconnect_timer_fd != -1) {
        close_fd(self_p, self_p->reconnect_timer_fd);
        self_p->reconnect_timer_fd = -1;
    }
}

void imported_client_process(struct imported_client_t *self_p, int fd, uint32_t events)
{
    if (fd == self_p->server_fd) {
        process_socket(self_p, events);
    } else if (fd == self_p->keep_alive_timer_fd) {
        process_keep_alive_timer(self_p, events);
    } else if (fd == self_p->reconnect_timer_fd) {
        process_reconnect_timer(self_p);
    }
}

void imported_client_send(struct imported_client_t *self_p)
{
    int res;
    struct imported_common_header_t *header_p;

    res = imported_client_to_server_encode(
        self_p->output.message_p,
        &self_p->message.data.buf_p[sizeof(*header_p)],
        self_p->message.data.size - sizeof(*header_p));

    if (res < 0) {
        return;
    }

    header_p = (struct imported_common_header_t *)&self_p->message.data.buf_p[0];
    header_p->type = IMPORTED_COMMON_MESSAGE_TYPE_USER;
    header_p->size = res;
    imported_common_header_hton(header_p);
    write(self_p->server_fd,
          &self_p->message.data.buf_p[0],
          res + sizeof(*header_p));
}

struct types_foo_t *
imported_client_init_foo(struct imported_client_t *self_p)
{
    self_p->output.message_p = imported_client_to_server_new(
        &self_p->output.workspace.buf_p[0],
        self_p->output.workspace.size);
    imported_client_to_server_messages_foo_init(self_p->output.message_p);

    return (&self_p->output.message_p->messages.value.foo);
}

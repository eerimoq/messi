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
#include "{name}_client.h"

static int epoll_ctl_add(struct {name}_client_t *self_p, int fd)
{{
    return (self_p->epoll_ctl(self_p->epoll_fd, EPOLL_CTL_ADD, fd, EPOLLIN));
}}

static void epoll_ctl_del(struct {name}_client_t *self_p, int fd)
{{
    self_p->epoll_ctl(self_p->epoll_fd, EPOLL_CTL_DEL, fd, 0);
}}

static void close_fd(struct {name}_client_t *self_p, int fd)
{{
    epoll_ctl_del(self_p, fd);
    close(fd);
}}

static void reset_message(struct {name}_client_t *self_p)
{{
    self_p->message.state = {name}_client_input_state_header_t;
    self_p->message.size = 0;
    self_p->message.left = sizeof(struct {name}_common_header_t);
}}

static void handle_message_user(struct {name}_client_t *self_p)
{{
    int res;
    struct {name}_server_to_client_t *message_p;
    uint8_t *payload_buf_p;
    size_t payload_size;

    self_p->input.message_p = {name}_server_to_client_new(
        &self_p->input.workspace.buf_p[0],
        self_p->input.workspace.size);
    message_p = self_p->input.message_p;

    if (message_p == NULL) {{
        return;
    }}

    payload_buf_p = &self_p->message.data.buf_p[sizeof(struct {name}_common_header_t)];
    payload_size = self_p->message.size - sizeof(struct {name}_common_header_t);

    res = {name}_server_to_client_decode(message_p, payload_buf_p, payload_size);

    if (res != (int)payload_size) {{
        return;
    }}

    switch (message_p->messages.choice) {{

{handle_cases}
    default:
        break;
    }}
}}

static void handle_message_pong(struct {name}_client_t *self_p)
{{
    self_p->pong_received = true;
}}

static void handle_message(struct {name}_client_t *self_p,
                           uint32_t type)
{{
    switch (type) {{

    case {name_upper}_COMMON_MESSAGE_TYPE_USER:
        handle_message_user(self_p);
        break;

    case {name_upper}_COMMON_MESSAGE_TYPE_PONG:
        handle_message_pong(self_p);
        break;

    default:
        break;
    }}
}}

static int start_timer(int fd, int seconds)
{{
    struct itimerspec timeout;

    memset(&timeout, 0, sizeof(timeout));
    timeout.it_value.tv_sec = seconds;

    return (timerfd_settime(fd, 0, &timeout, NULL));
}}

static void disconnect(struct {name}_client_t *self_p)
{{
    close_fd(self_p, self_p->server_fd);
    self_p->server_fd = -1;
    close_fd(self_p, self_p->keep_alive_timer_fd);
    self_p->keep_alive_timer_fd = -1;
}}

static int start_keep_alive_timer(struct {name}_client_t *self_p)
{{
    return (start_timer(self_p->keep_alive_timer_fd, 2));
}}

static int start_reconnect_timer(struct {name}_client_t *self_p)
{{
    int res;

    self_p->reconnect_timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);

    if (self_p->reconnect_timer_fd == -1) {{
        return (-1);
    }}

    res = epoll_ctl_add(self_p, self_p->reconnect_timer_fd);

    if (res == -1) {{
        goto out;
    }}

    return (start_timer(self_p->reconnect_timer_fd, 1));

 out:
    close(self_p->reconnect_timer_fd);
    self_p->reconnect_timer_fd = -1;

    return (-1);
}}

static void process_socket(struct {name}_client_t *self_p, uint32_t events)
{{
    (void)events;

    ssize_t size;
    struct {name}_common_header_t *header_p;

    header_p = (struct {name}_common_header_t *)self_p->message.data.buf_p;

    while (true) {{
        size = read(self_p->server_fd,
                    &self_p->message.data.buf_p[self_p->message.size],
                    self_p->message.left);

        if ((size == -1) && (errno == EAGAIN)) {{
            break;
        }} else if (size <= 0) {{
            disconnect(self_p);
            self_p->on_disconnected(self_p);
            start_reconnect_timer(self_p);
            break;
        }}

        self_p->message.size += size;
        self_p->message.left -= size;

        if (self_p->message.left > 0) {{
            continue;
        }}

        if (self_p->message.state == {name}_client_input_state_header_t) {{
            {name}_common_header_ntoh(header_p);
            self_p->message.left = header_p->size;
            self_p->message.state = {name}_client_input_state_payload_t;
        }}

        if (self_p->message.left == 0) {{
            handle_message(self_p, header_p->type);
            reset_message(self_p);
        }}
    }}
}}

static void process_keep_alive_timer(struct {name}_client_t *self_p, uint32_t events)
{{
    (void)events;

    int res;
    struct {name}_common_header_t header;
    ssize_t size;
    uint64_t value;

    size = read(self_p->keep_alive_timer_fd, &value, sizeof(value));

    if (size != sizeof(value)) {{
        disconnect(self_p);
        self_p->on_disconnected(self_p);

        return;
    }}

    if (!self_p->pong_received) {{
        disconnect(self_p);
        self_p->on_disconnected(self_p);
        start_reconnect_timer(self_p);

        return;
    }}

    res = start_keep_alive_timer(self_p);

    if (res == 0) {{
        header.type = {name_upper}_COMMON_MESSAGE_TYPE_PING;
        header.size = 0;
        {name}_common_header_hton(&header);
        write(self_p->server_fd, &header, sizeof(header));
        self_p->pong_received = false;
    }} else {{
        disconnect(self_p);
        self_p->on_disconnected(self_p);
    }}
}}

static int connect_to_server(struct {name}_client_t *self_p)
{{
    int res;
    int server_fd;
    struct sockaddr_in addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {{
        return (-1);
    }}

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(6000);
    inet_aton("127.0.0.1", (struct in_addr *)&addr.sin_addr.s_addr);

    res = connect(server_fd, (struct sockaddr *)&addr, sizeof(addr));

    if (res == -1) {{
        goto out1;
    }}

    res = {name}_common_make_non_blocking(server_fd);

    if (res == -1) {{
        goto out1;
    }}

    res = epoll_ctl_add(self_p, server_fd);

    if (res == -1) {{
        goto out1;
    }}

    self_p->keep_alive_timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);

    if (self_p->keep_alive_timer_fd == -1) {{
        goto out2;
    }}

    res = epoll_ctl_add(self_p, self_p->keep_alive_timer_fd);

    if (res == -1) {{
        goto out3;
    }}

    res = start_keep_alive_timer(self_p);

    if (res != 0) {{
        goto out4;
    }}

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
}}

static void process_reconnect_timer(struct {name}_client_t *self_p)
{{
    close_fd(self_p, self_p->reconnect_timer_fd);
    self_p->reconnect_timer_fd = -1;

    if (connect_to_server(self_p) != 0) {{
        start_reconnect_timer(self_p);
    }}
}}

{on_defaults}
int {name}_client_init(
    struct {name}_client_t *self_p,
    const char *user_p,
    const char *server_p,
    uint8_t *message_buf_p,
    size_t message_size,
    uint8_t *workspace_in_buf_p,
    size_t workspace_in_size,
    uint8_t *workspace_out_buf_p,
    size_t workspace_out_size,
    {name}_client_on_connected_t on_connected,
    {name}_client_on_disconnected_t on_disconnected,
{on_message_params}
    int epoll_fd,
    {name}_epoll_ctl_t epoll_ctl)
{{
{on_params_default}
    if (epoll_ctl == NULL) {{
        epoll_ctl = {name}_common_epoll_ctl_default;
    }}

    self_p->user_p = (char *)user_p;
    self_p->server_p = server_p;
    self_p->on_connected = on_connected;
    self_p->on_disconnected = on_disconnected;
{on_params_assign}
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
}}

void {name}_client_start(struct {name}_client_t *self_p)
{{
    if (connect_to_server(self_p) != 0) {{
        start_reconnect_timer(self_p);
    }}
}}

void {name}_client_stop(struct {name}_client_t *self_p)
{{
    disconnect(self_p);

    if (self_p->reconnect_timer_fd != -1) {{
        close_fd(self_p, self_p->reconnect_timer_fd);
        self_p->reconnect_timer_fd = -1;
    }}
}}

void {name}_client_process(struct {name}_client_t *self_p, int fd, uint32_t events)
{{
    if (fd == self_p->server_fd) {{
        process_socket(self_p, events);
    }} else if (fd == self_p->keep_alive_timer_fd) {{
        process_keep_alive_timer(self_p, events);
    }} else if (fd == self_p->reconnect_timer_fd) {{
        process_reconnect_timer(self_p);
    }}
}}

void {name}_client_send(struct {name}_client_t *self_p)
{{
    int res;
    struct {name}_common_header_t *header_p;

    res = {name}_client_to_server_encode(
        self_p->output.message_p,
        &self_p->message.data.buf_p[sizeof(*header_p)],
        self_p->message.data.size - sizeof(*header_p));

    if (res < 0) {{
        return;
    }}

    header_p = (struct {name}_common_header_t *)&self_p->message.data.buf_p[0];
    header_p->type = {name_upper}_COMMON_MESSAGE_TYPE_USER;
    header_p->size = res;
    {name}_common_header_hton(header_p);
    write(self_p->server_fd,
          &self_p->message.data.buf_p[0],
          res + sizeof(*header_p));
}}

{init_messages}
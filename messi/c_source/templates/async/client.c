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

#include "messi.h"
#include "NAME_client.h"

static void reset_message(struct NAME_client_t *self_p)
{
    self_p->message.state = NAME_client_input_state_header_t;
    self_p->message.size = 0;
    self_p->message.left = sizeof(struct messi_header_t);
}

static void handle_message_user(struct NAME_client_t *self_p)
{
    int res;
    struct NAME_server_to_client_t *message_p;
    uint8_t *payload_buf_p;
    size_t payload_size;

    self_p->input.message_p = NAME_server_to_client_new(
        &self_p->input.workspace.buf_p[0],
        self_p->input.workspace.size);
    message_p = self_p->input.message_p;

    if (message_p == NULL) {
        return;
    }

    payload_buf_p = &self_p->message.data.buf_p[sizeof(struct messi_header_t)];
    payload_size = self_p->message.size - sizeof(struct messi_header_t);

    res = NAME_server_to_client_decode(message_p, payload_buf_p, payload_size);

    if (res != (int)payload_size) {
        return;
    }

    switch (message_p->messages.choice) {

HANDLE_CASES
    default:
        break;
    }
}

static void handle_message_pong(struct NAME_client_t *self_p)
{
    self_p->pong_received = true;
}

static void handle_message(struct NAME_client_t *self_p,
                           uint32_t type)
{
    switch (type) {

    case MESSI_MESSAGE_TYPE_SERVER_TO_CLIENT_USER:
        handle_message_user(self_p);
        break;

    case MESSI_MESSAGE_TYPE_PONG:
        handle_message_pong(self_p);
        break;

    default:
        break;
    }
}

static void on_keep_alive_timeout(struct NAME_client_t *self_p)
{
    struct messi_header_t header;

    if (self_p->pong_received) {
        async_timer_start(&self_p->keep_alive_timer);
        self_p->pong_received = false;
        messi_header_create(&header, MESSI_MESSAGE_TYPE_PING, 0);
        async_stcp_client_write(&self_p->stcp, &header, sizeof(header));
    } else {
        async_stcp_client_disconnect(&self_p->stcp);
        self_p->on_disconnected(self_p);
    }
}

static void on_stcp_connected(struct async_stcp_client_t *stcp_p,
                              int res)
{
    struct NAME_client_t *self_p;

    self_p = async_container_of(stcp_p, typeof(*self_p), stcp);

    if (res == 0) {
        async_timer_start(&self_p->keep_alive_timer);
        self_p->pong_received = true;
        self_p->on_connected(self_p);
    } else {
        async_timer_start(&self_p->reconnect_timer);
    }
}

static void on_stcp_disconnected(struct async_stcp_client_t *stcp_p)
{
    struct NAME_client_t *self_p;

    self_p = async_container_of(stcp_p, typeof(*self_p), stcp);

    async_timer_stop(&self_p->keep_alive_timer);
    self_p->on_disconnected(self_p);
    async_timer_start(&self_p->reconnect_timer);
}

static void on_stcp_input(struct async_stcp_client_t *stcp_p)
{
    ssize_t size;
    struct messi_header_t *header_p;
    struct NAME_client_t *self_p;

    self_p = async_container_of(stcp_p, typeof(*self_p), stcp);

    header_p = (struct messi_header_t *)self_p->message.data.buf_p;

    while (true) {
        size = async_stcp_client_read(
            stcp_p,
            &self_p->message.data.buf_p[self_p->message.size],
            self_p->message.left);

        if (size == 0) {
            break;
        }

        self_p->message.size += size;
        self_p->message.left -= size;

        if (self_p->message.left > 0) {
            continue;
        }

        if (self_p->message.state == NAME_client_input_state_header_t) {
            self_p->message.left = messi_header_get_size(header_p);
            self_p->message.state = NAME_client_input_state_payload_t;
        }

        if (self_p->message.left == 0) {
            handle_message(self_p, header_p->type);
            reset_message(self_p);
        }
    }
}

static void on_connected_default(struct NAME_client_t *self_p)
{
        (void)self_p;
}

static void on_disconnected_default(struct NAME_client_t *self_p)
{
        (void)self_p;
}

ON_DEFAULTS
int NAME_client_init(
    struct NAME_client_t *self_p,
    const char *user_p,
    const char *server_uri_p,
    uint8_t *message_buf_p,
    size_t message_size,
    uint8_t *workspace_in_buf_p,
    size_t workspace_in_size,
    uint8_t *workspace_out_buf_p,
    size_t workspace_out_size,
    NAME_client_on_connected_t on_connected,
    NAME_client_on_disconnected_t on_disconnected,
ON_MESSAGE_PARAMS
    struct async_t *async_p)
{
    int res;

ON_PARAMS_DEFAULT
    if (on_connected == NULL) {
        on_connected = on_connected_default;
    }

    if (on_disconnected == NULL) {
        on_disconnected = on_disconnected_default;
    }

    self_p->user_p = (char *)user_p;

    res = messi_parse_tcp_uri(server_uri_p,
                              &self_p->server.address[0],
                              sizeof(self_p->server.address),
                              &self_p->server.port);

    if (res != 0) {
        return (res);
    }

    self_p->on_connected = on_connected;
    self_p->on_disconnected = on_disconnected;
ON_PARAMS_ASSIGN
    self_p->message.data.buf_p = message_buf_p;
    self_p->message.data.size = message_size;
    reset_message(self_p);
    self_p->input.workspace.buf_p = workspace_in_buf_p;
    self_p->input.workspace.size = workspace_in_size;
    self_p->output.workspace.buf_p = workspace_out_buf_p;
    self_p->output.workspace.size = workspace_out_size;
    async_stcp_client_init(&self_p->stcp,
                           NULL,
                           on_stcp_connected,
                           on_stcp_disconnected,
                           on_stcp_input,
                           async_p);
    async_timer_init(&self_p->keep_alive_timer,
                     (async_timer_timeout_t)on_keep_alive_timeout,
                     self_p,
                     2000,
                     0,
                     async_p);
    async_timer_init(&self_p->reconnect_timer,
                     (async_timer_timeout_t)NAME_client_start,
                     self_p,
                     1000,
                     0,
                     async_p);

    return (0);
}

void NAME_client_start(struct NAME_client_t *self_p)
{
    async_stcp_client_connect(&self_p->stcp,
                              &self_p->server.address[0],
                              self_p->server.port);
}

void NAME_client_stop(struct NAME_client_t *self_p)
{
    async_stcp_client_disconnect(&self_p->stcp);
    async_timer_stop(&self_p->reconnect_timer);
    async_timer_stop(&self_p->keep_alive_timer);
}

void NAME_client_send(struct NAME_client_t *self_p)
{
    int res;
    struct messi_header_t *header_p;

    res = NAME_client_to_server_encode(
        self_p->output.message_p,
        &self_p->message.data.buf_p[sizeof(*header_p)],
        self_p->message.data.size - sizeof(*header_p));

    if (res < 0) {
        return;
    }

    header_p = (struct messi_header_t *)&self_p->message.data.buf_p[0];
    messi_header_create(header_p, MESSI_MESSAGE_TYPE_CLIENT_TO_SERVER_USER, res);
    async_stcp_client_write(&self_p->stcp,
                            &self_p->message.data.buf_p[0],
                            res + sizeof(*header_p));
}

INIT_MESSAGES

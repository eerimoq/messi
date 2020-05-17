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

#ifndef CHAT_CLIENT_H
#define CHAT_CLIENT_H

#include <stdint.h>
#include "messi.h"
#include "chat.h"
#include "async.h"

struct chat_client_t;

typedef void (*chat_client_on_connected_t)(struct chat_client_t *self_p);

typedef void (*chat_client_on_disconnected_t)(struct chat_client_t *self_p);

typedef void (*chat_client_on_connect_rsp_t)(
    struct chat_client_t *self_p,
    struct chat_connect_rsp_t *message_p);

typedef void (*chat_client_on_message_ind_t)(
    struct chat_client_t *self_p,
    struct chat_message_ind_t *message_p);

enum chat_client_input_state_t {
    chat_client_input_state_header_t = 0,
    chat_client_input_state_payload_t
};

struct chat_client_t {
    char *user_p;
    struct {
        char address[16];
        int port;
    } server;
    chat_client_on_connected_t on_connected;
    chat_client_on_disconnected_t on_disconnected;
    chat_client_on_connect_rsp_t on_connect_rsp;
    chat_client_on_message_ind_t on_message_ind;
    struct async_stcp_client_t stcp;
    struct async_timer_t keep_alive_timer;
    struct async_timer_t reconnect_timer;
    bool pong_received;
    struct {
        struct messi_buffer_t data;
        size_t size;
        size_t left;
        enum chat_client_input_state_t state;
    } message;
    struct {
        struct chat_server_to_client_t *message_p;
        struct messi_buffer_t workspace;
    } input;
    struct {
        struct chat_client_to_server_t *message_p;
        struct messi_buffer_t workspace;
    } output;
};

/**
 * Initialize given client.
 */
int chat_client_init(
    struct chat_client_t *self_p,
    const char *user_p,
    const char *server_uri_p,
    uint8_t *message_buf_p,
    size_t message_size,
    uint8_t *workspace_in_buf_p,
    size_t workspace_in_size,
    uint8_t *workspace_out_buf_p,
    size_t workspace_out_size,
    chat_client_on_connected_t on_connected,
    chat_client_on_disconnected_t on_disconnected,
    chat_client_on_connect_rsp_t on_connect_rsp,
    chat_client_on_message_ind_t on_message_ind,
    struct async_t *async_p);

/**
 * Start serving clients.
 */
void chat_client_start(struct chat_client_t *self_p);

/**
 * Stop serving clients.
 */
void chat_client_stop(struct chat_client_t *self_p);

/**
 * Send prepared message the server.
 */
void chat_client_send(struct chat_client_t *self_p);

struct chat_connect_req_t *
chat_client_init_connect_req(struct chat_client_t *self_p);

struct chat_message_ind_t *
chat_client_init_message_ind(struct chat_client_t *self_p);

#endif

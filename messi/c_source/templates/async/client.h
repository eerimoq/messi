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

#ifndef NAME_UPPER_CLIENT_H
#define NAME_UPPER_CLIENT_H

#include <stdint.h>
#include "messi.h"
#include "NAME.h"
#include "async.h"

struct NAME_client_t;

typedef void (*NAME_client_on_connected_t)(struct NAME_client_t *self_p);

typedef void (*NAME_client_on_disconnected_t)(struct NAME_client_t *self_p);

ON_MESSAGE_TYPEDEFS
enum NAME_client_input_state_t {
    NAME_client_input_state_header_t = 0,
    NAME_client_input_state_payload_t
};

struct NAME_client_t {
    char *user_p;
    struct {
        char address[16];
        int port;
    } server;
    NAME_client_on_connected_t on_connected;
    NAME_client_on_disconnected_t on_disconnected;
ON_MESSAGE_MEMBERS
    struct async_stcp_client_t stcp;
    struct async_timer_t keep_alive_timer;
    struct async_timer_t reconnect_timer;
    bool pong_received;
    struct {
        struct messi_buffer_t data;
        size_t size;
        size_t left;
        enum NAME_client_input_state_t state;
    } message;
    struct {
        struct NAME_server_to_client_t *message_p;
        struct messi_buffer_t workspace;
    } input;
    struct {
        struct NAME_client_to_server_t *message_p;
        struct messi_buffer_t workspace;
    } output;
};

/**
 * Initialize given client.
 */
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
    struct async_t *async_p);

/**
 * Start serving clients.
 */
void NAME_client_start(struct NAME_client_t *self_p);

/**
 * Stop serving clients.
 */
void NAME_client_stop(struct NAME_client_t *self_p);

/**
 * Send prepared message the server.
 */
void NAME_client_send(struct NAME_client_t *self_p);

INIT_MESSAGES
#endif

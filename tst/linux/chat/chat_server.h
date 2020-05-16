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

#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include <stdint.h>
#include "messi.h"
#include "chat.h"

struct chat_server_t;
struct chat_server_client_t;

typedef void (*chat_server_on_connect_req_t)(
    struct chat_server_t *self_p,
    struct chat_server_client_t *client_p,
    struct chat_connect_req_t *message_p);

typedef void (*chat_server_on_message_ind_t)(
    struct chat_server_t *self_p,
    struct chat_server_client_t *client_p,
    struct chat_message_ind_t *message_p);

enum chat_server_client_input_state_t {
    chat_server_client_input_state_header_t = 0,
    chat_server_client_input_state_payload_t
};

typedef void (*chat_server_on_client_connected_t)(
    struct chat_server_t *self_p,
    struct chat_server_client_t *client_p);

typedef void (*chat_server_on_client_disconnected_t)(
    struct chat_server_t *self_p,
    struct chat_server_client_t *client_p);

struct chat_server_t {
    struct {
        char address[16];
        int port;
    } server;
    chat_server_on_client_connected_t on_client_connected;
    chat_server_on_client_disconnected_t on_client_disconnected;
    chat_server_on_connect_req_t on_connect_req;
    chat_server_on_message_ind_t on_message_ind;
    int epoll_fd;
    messi_epoll_ctl_t epoll_ctl;
    int listener_fd;
    struct chat_server_client_t *current_client_p;
    struct {
        struct chat_server_client_t *connected_list_p;
        struct chat_server_client_t *free_list_p;
        struct chat_server_client_t *pending_disconnect_list_p;
        size_t input_buffer_size;
    } clients;
    struct {
        struct messi_buffer_t data;
        size_t left;
    } message;
    struct {
        struct chat_client_to_server_t *message_p;
        struct messi_buffer_t workspace;
    } input;
    struct {
        struct chat_server_to_client_t *message_p;
        struct messi_buffer_t workspace;
    } output;
};

struct chat_server_client_t {
    int client_fd;
    int keep_alive_timer_fd;
    struct {
        enum chat_server_client_input_state_t state;
        uint8_t *buf_p;
        size_t size;
        size_t left;
    } input;
    struct chat_server_client_t *next_p;
    struct chat_server_client_t *prev_p;
};

/**
 * Initialize given server.
 */
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
    messi_epoll_ctl_t epoll_ctl);

/**
 * Start serving clients.
 */
int chat_server_start(struct chat_server_t *self_p);

/**
 * Stop serving clients.
 */
void chat_server_stop(struct chat_server_t *self_p);

/**
 * Process any pending events on given file descriptor if it belongs
 * to given server.
 */
void chat_server_process(struct chat_server_t *self_p, int fd, uint32_t events);

/**
 * Send prepared message to given client.
 */
void chat_server_send(struct chat_server_t *self_p,
                        struct chat_server_client_t *client_p);

/**
 * Send prepared message to current client.
 */
void chat_server_reply(struct chat_server_t *self_p);

/**
 * Broadcast prepared message to all clients.
 */
void chat_server_broadcast(struct chat_server_t *self_p);

/**
 * Disconnect given client. If given client is NULL, the currect
 * client is disconnected.
 */
void chat_server_disconnect(struct chat_server_t *self_p,
                            struct chat_server_client_t *client_p);

struct chat_connect_rsp_t *
chat_server_init_connect_rsp(struct chat_server_t *self_p);

struct chat_message_ind_t *
chat_server_init_message_ind(struct chat_server_t *self_p);

#endif

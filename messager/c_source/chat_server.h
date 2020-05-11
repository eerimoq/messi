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

#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include <stdint.h>
#include "chat_common.h"

struct chat_server_t {
    const char *address_p;
    struct chat_server_client_t *clients_p;
    chat_on_connect_req_t on_connect_req;
    chat_on_message_ind_t on_message_ind;
    int epoll_fd;
    chat_epoll_ctl_t epoll_ctl;
    int listener_fd;
    struct chat_server_client_t *connected_clients_p;
    struct chat_common_buffer_t encoded;
    struct {
        struct chat_client_to_server_t *message_p;
        struct chat_common_buffer_t workspace;
    } input;
    struct {
        struct chat_server_to_client_t *message_p;
        struct chat_common_buffer_t workspace;
    } output;
};

struct chat_server_client_t {
    int fd;
    int keep_alive_timer_fd;
    struct chat_server_client_t *next_p;
};

/**
 * Initialize given server.
 */
int chat_server_init(struct chat_server_t *self_p,
                     const char *address_p,
                     struct chat_server_client_t *clients_p,
                     int clients_max,
                     chat_on_connect_req_t on_connect_req,
                     chat_on_message_ind_t on_message_ind,
                     int epoll_fd,
                     chat_epoll_ctl_t epoll_ctl);

/**
 * Start serving clients.
 */
int chat_server_start(struct chat_server_t *self_p);

/**
 * Stop serving clients.
 */
void chat_server_stop(struct chat_server_t *self_p);

/**
 * Returns true if given file descriptor belongs to given server.
 */
bool chat_server_has_file_descriptor(struct chat_server_t *self_p, int fd);

/**
 * Process pending events on given file descriptor.
 */
void chat_server_process(struct chat_server_t *self_p, int fd, uint32_t events);

#endif

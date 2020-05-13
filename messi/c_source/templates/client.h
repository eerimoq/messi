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

#ifndef {name_upper}_CLIENT_H
#define {name_upper}_CLIENT_H

#include <stdint.h>
#include "{name}_common.h"
#include "{name}.h"

struct {name}_client_t;

typedef void (*{name}_client_on_connected_t)(struct {name}_client_t *self_p);

typedef void (*{name}_client_on_disconnected_t)(struct {name}_client_t *self_p);

{on_message_typedefs}
enum {name}_client_input_state_t {{
    {name}_client_input_state_header_t = 0,
    {name}_client_input_state_payload_t
}};

struct {name}_client_t {{
    char *user_p;
    const char *server_p;
    {name}_client_on_connected_t on_connected;
    {name}_client_on_disconnected_t on_disconnected;
{on_message_members}
    int epoll_fd;
    {name}_epoll_ctl_t epoll_ctl;
    int server_fd;
    int keep_alive_timer_fd;
    int reconnect_timer_fd;
    bool pong_received;
    struct {{
        struct {name}_common_buffer_t data;
        size_t size;
        size_t left;
        enum {name}_client_input_state_t state;
    }} message;
    struct {{
        struct {name}_server_to_client_t *message_p;
        struct {name}_common_buffer_t workspace;
    }} input;
    struct {{
        struct {name}_client_to_server_t *message_p;
        struct {name}_common_buffer_t workspace;
    }} output;
}};

/**
 * Initialize given client.
 */
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
    {name}_epoll_ctl_t epoll_ctl);

/**
 * Start serving clients.
 */
void {name}_client_start(struct {name}_client_t *self_p);

/**
 * Stop serving clients.
 */
void {name}_client_stop(struct {name}_client_t *self_p);

/**
 * Process any pending events on given file descriptor if it belongs
 * to given server.
 */
void {name}_client_process(
    struct {name}_client_t *self_p,
    int fd,
    uint32_t events);

/**
 * Send prepared message the server.
 */
void {name}_client_send(struct {name}_client_t *self_p);

{init_messages}
#endif

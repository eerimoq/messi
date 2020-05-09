#ifndef HELLO_WORLD_SERVER_CLIENT_H
#define HELLO_WORLD_SERVER_CLIENT_H

#include "hello_world.h"

typedef void (*hello_world_server_client_on_greeting_ind_t)(
    struct hello_world_greeting_ind_t *message_p);

struct hello_world_server_buffer_t {
    void *buf_p;
    size_t size;
};

struct hello_world_server_t {
    struct {
        hello_world_server_client_on_greeting_ind_t on_greeting_ind;
    } handlers;
    struct hello_world_server_buffer_t encoded;
    struct {
        struct hello_world_client_to_server_t message;
        struct hello_world_server_buffer_t workspace;
    } input;
    struct {
        struct hello_world_client_to_server_t message;
        struct hello_world_server_buffer_t workspace;
    } output;
};

/**
 * Initialize given server.
 */
int hello_world_server_init(
    struct hello_world_server_t *self_p,
    void *input_workspace_buf_p,
    size_t input_workspace_size,
    struct hello_world_server_client_on_greeting_ind_t on_greeting_ind);

/**
 * Send the output message.
 */
void hello_world_client_send(struct hello_world_server_t *self_p);

/**
 * Serve clients forever.
 */
int hello_world_server_server_forever(struct hello_world_server_t *self_p);

/**
 * Initialize the output message as a greeting message.
 */
struct hello_world_greeting_ind_t *
hello_world_client_init_greeting_ind(struct hello_world_server_t *self_p);

#endif

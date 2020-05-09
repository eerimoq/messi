#ifndef HELLO_WORLD_SERVER_CLIENT_H
#define HELLO_WORLD_SERVER_CLIENT_H

#include "hello_world.h"

typedef void (*hello_world_server_client_on_greeting_ind_t)(
    struct hello_world_greeting_ind_t *message_p);

struct hello_world_server_client_handlers_t {
    hello_world_server_client_on_greeting_ind_t on_greeting_ind;
};

/**
 * Initialize given server.
 */
int hello_world_server_init(struct hello_world_server_t *self_p,
                            void *workspace_p,
                            size_t size,
                            struct hello_world_server_client_handlers_t *handlers_p);

/**
 * Serve clients forever.
 */
int hello_world_server_server_forever(struct hello_world_server_t *self_p);

#endif

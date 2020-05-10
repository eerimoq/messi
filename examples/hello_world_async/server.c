#include "hello_world.h"

static void handle_greeting_ind(struct hello_world_server_t *self_p,
                                struct hello_world_server_greeting_ind_t *message_p,
                                void *arg_p)
{
    (void)arg_p;

    printf("Received '%s'.\n", message_p->text_p);
}

int main()
{
    struct async_t async;
    struct hello_world_server_t server;
    struct hello_world_server_client_t clients[4];
    uint8_t workspace[128];

    async_init(&async);
    async_set_runtime(&async, async_runtime_create());
    hello_world_server_init(&server,
                            "tcp://127.0.0.1:6000",
                            &clients[0],
                            4,
                            &workspace[0],
                            sizeof(workspace),
                            NULL,
                            0,
                            handle_greeting_ind,
                            NULL,
                            &async);   
    async_run_forever(&async);

    return (0);
}

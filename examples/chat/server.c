#include "hello_world.h"

static void handle_message_ind(struct hello_world_server_t *self_p,
                               struct hello_world_message_ind_t *message_p,
                               void *arg_p)
{
    (void)arg_p;

    struct chat_message_ind_t *bmessage_p;

    bmessage_p = chat_server_init_message_ind(self_p);
    bmessage_p->text_p = message_p->text_p;
    chat_server_broadcast(self_p);
}

static void handle_user_ind(struct hello_world_server_t *self_p,
                            struct hello_world_user_ind_t *message_p,
                            void *arg_p)
{
    (void)arg_p;

    hello_world_server_reply(self_p);
}

int main()
{
    struct hello_world_server_t server;
    uint8_t workspace[128];

    hello_world_server_init(&server,
                            "tcp://127.0.0.1:6000",
                            &workspace[0],
                            sizeof(workspace),
                            handle_message_ind,
                            handle_user_ind,
                            NULL);
    hello_world_server_serve_forever(&server);

    return (1);
}

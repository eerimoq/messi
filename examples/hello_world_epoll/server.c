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
    struct hello_world_server_t server;
    struct hello_world_server_client_t clients[4];
    uint8_t workspace[128];

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
                            epoll_fd,
                            epoll_ctl);

    while (true) {
        res = epoll_wait(epoll_fd, &events, 1, -1);

        if (res != 1) {
            break;
        }

        if (hello_world_server_has_file_descriptor(&server, events.fd)) {
            hello_world_server_process(&server);
        }
    }

    return (1);
}

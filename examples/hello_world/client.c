#include "hello_world.h"

static void send_greeting(struct hello_world_client_t *self_p,
                          const char *text_p)
{
    struct hello_world_greeting_ind_t *message_p;

    printf("Sending '%s'.\n", text_p);

    message_p = hello_world_client_init_greeting_ind(self_p);
    message_p->text_p = text_p;
    hello_world_client_send(self_p);
}

int main()
{
    struct hello_world_client_t client;
    uint8_t workspace[128];

    hello_world_client_init(&client,
                            &workspace[0],
                            sizeof(workspace),
                            NULL,
                            NULL);
    hello_world_client_connect(&client, "tcp://127.0.0.1:6000");
    printf("Connected to the server.\n");

    send_greeting(&client, "Hello!");
    sleep(1);
    send_greeting(&client, "Hi!");
    sleep(1);
    send_greeting(&client, "Hi again!");

    printf("Disconnecting from the server..\n");
    hello_world_client_disconnect(&client);

    return (0);
}

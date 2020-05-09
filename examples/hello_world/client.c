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

static void handle_disconnected(struct hello_world_client_t *self_p)
{
    printf("Disconnected.\n");
}

int main()
{
    struct hello_world_client_t client;
    uint8_t workspace[128];
    int res;

    res = hello_world_client_init(&client,
                                  &workspace[0],
                                  sizeof(workspace),
                                  handle_disconnected,
                                  NULL);

    if (res != 0) {
        printf("Init failed.\n");

        return (1);
    }

    while (true) {
        res = hello_world_client_connect(&client, "tcp://127.0.0.1:6000");

        if (res != 0) {
            printf("Connect failed.\n");
            sleep(1);   
            continue;
        }

        printf("Connected to the server.\n");

        send_greeting(&client, "Hello!");
        sleep(1);
        send_greeting(&client, "Hi!");
        sleep(1);
        send_greeting(&client, "Hi again!");
        
        printf("Disconnecting from the server.\n");
        hello_world_client_disconnect(&client);

        printf("Waiting 3 seconds before connecting again.\n");
        sleep(3);
    }
    
    return (0);
}

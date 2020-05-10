#include "hello_world.h"

struct greeter_t {
    struct hello_world_client_t client;
    struct async_timer_t connect_timer;
    struct async_timer_t greeting_2_timer;
    struct async_timer_t greeting_3_timer;
    uint8_t workspace[128];
};

static void send_greeting(struct hello_world_client_t *self_p,
                          const char *text_p)
{
    struct hello_world_greeting_ind_t *message_p;

    printf("Sending '%s'.\n", text_p);

    message_p = hello_world_client_init_greeting_ind(self_p);
    message_p->text_p = text_p;
    hello_world_client_send(self_p);
}

static void connect_to_server(struct greeter_t *self_p)
{
    hello_world_client_connect(&client, "tcp://127.0.0.1:6000");
}

static void on_connect_timeout(struct greeter_t *self_p)
{
    connect_to_server(self_p);
}

static void on_connected(struct greeter_t *self_p, int res)
{
    if (res == 0) {
        printf("Connected to the server.\n");
        send_greeting(self_p, "Hello!");
        async_timer_start(&self_p->greeting_2_timer);
    } else {
        printf("Connect failed.\n");
        async_timer_start(&self_p->connect_timer);
    }
}

static void on_greeting_2_timeout(struct greeter_t *self_p)
{
    send_greeting(self_p, "Hi!");
    async_timer_start(&self_p->greeting_3_timer);
}

static void on_greeting_3_timeout(struct greeter_t *self_p)
{
    send_greeting(self_p, "Hi again!");

    printf("Disconnecting from the server.\n");
    hello_world_client_disconnect(&client);

    printf("Starting the reconnect timer.\n");
    async_timer_start(self_p->connect_timer);
}

static void handle_disconnected(struct hello_world_client_t *self_p)
{
    printf("Disconnected.\n");
}

static void greeter_init(struct greeter_t *self_p, struct async_t *async_p)
{
    async_timer_init(&self_p->connect_timer,
                     3000,
                     0,
                     on_connect_timeout,
                     async_p);
    async_timer_init(&self_p->greeting_2_timer,
                     1000,
                     0,
                     on_greeting_2_timeout,
                     async_p);
    async_timer_init(&self_p->greeting_3_timer,
                     1000,
                     0,
                     on_greeting_3_timeout,
                     async_p);
    hello_world_client_init(&self_p->client,
                            &self_p->workspace[0],
                            sizeof(self_p->workspace),
                            handle_disconnected,
                            NULL,
                            async_p);
    connect_to_server(self_p);
}

int main()
{
    struct async_t async;
    struct greeter_t greeter;

    async_init(&async);
    async_set_runtime(&async, async_runtime_create());
    greeter_init(&greeter, &async);
    async_run_forever(&async);

    return (0);
}

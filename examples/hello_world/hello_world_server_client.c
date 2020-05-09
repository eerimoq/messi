#include "hello_world_server_client.h"

struct header_t {
    uint32_t type;
    uint32_t size;
};

static void header_hton(struct header_t *header_p)
{
    header_p->type = htonl(header_p->type);
    header_p->size = htonl(header_p->size);
}

static void header_ntoh(struct header_t *header_p)
{
    header_p->type = ntohl(header_p->type);
    header_p->size = ntohl(header_p->size);
}

static int handle_message_user(struct hello_world_server_t *self_p)
{
    struct hello_world_client_to_server_t *message_p;

    message_p = hello_world_client_to_server_new();

    res = hello_world_decode(message_p, self_p->payload.buf_p, size);

    if (res < 0) {
        return (res);
    }

    switch (message_p->messages.choice) {

    case hello_world_client_to_server_messages_choice_greeting_ind_e:
        self_p->handlers.on_greeting_ind(&message_p->messages.value.greeting_ind);
        break;

    default:
        break;
    }

    return (0);
}

static int handle_message_ping(struct hello_world_server_t *self_p)
{
    struct header_t header;

    header.type = TYPE_PONG;
    header.size = 0;
    header_hton(&header);

    write(self_p, &header, sizeof(header));
}

int hello_world_server_init(struct hello_world_server_t *self_p,
                            void *workspace_p,
                            size_t size)
{
    self_p->on_message = on_message;
    self_p->message_p = time_message_new(&workspace[0], size);

    if (res != 0) {
        return (res);
    }
}

static int handle_message(struct hello_world_server_t *self_p,
                          uint32_t type)
{
    switch (type) {

    case MESSAGE_TYPE_USER:
        res = handle_message_user(self_p);
        break;

    case MESSAGE_TYPE_PING:
        res = handle_message_ping(self_p);
        break;

    default:
        break;
    }
}

static int read_message(struct hello_world_server_t *self_p)
{
    struct header_t header;
    ssize_t size;

    size = read(listener, &header, sizeof(header));

    if (size != sizeof(header)) {
        return (-EIO);
    }
    
    header_ntoh(&header);
    
    if (header.size > self_p->payload.size) {
        return (-EPROTO);
    }

    size = read(listener, self_p->payload.buf_p, header.size);

    if (size != header.size) {
        return (-EIO);
    }

    return (0);
}

int hello_world_server_server_forever(struct hello_world_server_t *self_p)
{
    socket();
    bind();
    accept();

    while (true) {
        poll();
        
        res = read_message(self_p);

        if (res == 0) {
            res = handle_message(self_p);
        }

        if (res != 0) {
            disconnect_client();
        }
    }
}

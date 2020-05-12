#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include "nala.h"
#include "chat_server.h"

#define EPOLL_FD                            9
#define LISTENER_FD                         10
#define ERIK_FD                             11
#define KALLE_FD                            12
#define FIA_FD                              13

/* echo "connect_req {user: \"Erik\"}"
   | protoc --encode=chat.ClientToServer ../tests/files/chat/chat.proto
   > encoded.bin */

static uint8_t connect_req_payload_erik[] = {
    0x0a, 0x06, 0x0a, 0x04, 0x45, 0x72, 0x69, 0x6b
};

static uint8_t connect_req_payload_kalle[] = {
    0x0a, 0x07, 0x0a, 0x05, 0x4b, 0x61, 0x6c, 0x6c, 0x65
};

static uint8_t connect_req_payload_fia[] = {
    0x0a, 0x05, 0x0a, 0x03, 0x46, 0x69, 0x61
};

static void on_connect_req(struct chat_server_t *self_p,
                           struct chat_server_client_t *client_p,
                           struct chat_connect_req_t *message_p)
{
    (void)client_p;

    printf("Client <%s> connected.\n", message_p->user_p);

    chat_server_init_connect_rsp(self_p);
    chat_server_send(self_p);
}

static void on_message_ind(struct chat_server_t *self_p,
                           struct chat_server_client_t *client_p,
                           struct chat_message_ind_t *message_in_p)
{
    (void)client_p;

    struct chat_message_ind_t *message_p;

    message_p = chat_server_init_message_ind(self_p);
    message_p->user_p = message_in_p->user_p;
    message_p->text_p = message_in_p->text_p;
    chat_server_broadcast(self_p);
}

static void connect_client(uint8_t *connect_req_buf_p,
                           size_t connect_req_size,
                           int client_fd)
{
    (void)connect_req_buf_p;
    (void)connect_req_size;
    (void)client_fd;

    /* TCP connect. */

    /* Chat connect messages. */
}

static void disconnect_client(int client_fd)
{
    (void)client_fd;

    /* TCP disconnect. */
}

static void connect_erik(void)
{
    connect_client(connect_req_payload_erik,
                   sizeof(connect_req_payload_erik),
                   ERIK_FD);
}

static void connect_kalle(void)
{
    connect_client(connect_req_payload_kalle,
                   sizeof(connect_req_payload_kalle),
                   KALLE_FD);
}

static void connect_fia(void)
{
    connect_client(connect_req_payload_fia,
                   sizeof(connect_req_payload_fia),
                   FIA_FD);
}

static void disconnect_erik(void)
{
    disconnect_client(ERIK_FD);
}

static void disconnect_kalle(void)
{
    disconnect_client(KALLE_FD);
}

static void disconnect_fia(void)
{
    disconnect_client(FIA_FD);
}

extern int __real_fcntl(int fd, int cmd, ...);

int fcntl_mock_va_arg_real(int fd, int cmd, va_list __nala_va_list)
{
    return (__real_fcntl(fd, cmd, va_arg(__nala_va_list, unsigned long)));
}

TEST(connect_and_disconnect_clients)
{
    struct chat_server_t server;
    struct chat_server_client_t clients[3];
    uint8_t clients_input_buffers[3][128];
    uint8_t workspace_in[128];

    ASSERT_EQ(chat_server_init(&server,
                               "tcp://127.0.0.1:6000",
                               &clients[3],
                               3,
                               &clients_input_buffers[0][0],
                               sizeof(clients_input_buffers[0]),
                               &workspace_in[0],
                               sizeof(workspace_in),
                               on_connect_req,
                               on_message_ind,
                               EPOLL_FD,
                               NULL), 0);

    /* Start creates a socket and starts listening for clients. */
    socket_mock_once(AF_INET, SOCK_STREAM, 0, LISTENER_FD);
    bind_mock_once(LISTENER_FD, sizeof(struct sockaddr_in), 0);
    listen_mock_once(LISTENER_FD, 5, 0);
    fcntl_mock_once(LISTENER_FD, F_GETFL, 0, "");
    fcntl_mock_once(LISTENER_FD, F_SETFL, O_NONBLOCK, "");
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_ADD, LISTENER_FD, 0);

    ASSERT_EQ(chat_server_start(&server), 0);

    /* Connect and disconnect clients. */
    connect_erik();
    connect_kalle();
    connect_fia();
    disconnect_kalle();
    connect_kalle();
    disconnect_erik();
    disconnect_fia();

    /* Stop with Kalle connected. */
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_DEL, LISTENER_FD, 0);
    close_mock_once(LISTENER_FD, 0);
    chat_server_stop(&server);
}

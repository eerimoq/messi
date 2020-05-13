#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include "nala.h"
#include "chat_server.h"

#define EPOLL_FD                            9
#define LISTENER_FD                         10
#define ERIK_FD                             11
#define KALLE_FD                            12
#define FIA_FD                              13
#define ERIK_TIMER_FD                       14
#define KALLE_TIMER_FD                      15
#define FIA_TIMER_FD                        16
#define KEEP_ALIVE_TIMER_FD                 17

#define HEADER_SIZE sizeof(struct chat_common_header_t)

static inline int client_to_timer_fd(int client_fd)
{
    return (client_fd + 3);
}

/* echo "connect_req {user: \"Erik\"}" \
   | protoc --encode=chat.ClientToServer ../tests/files/chat/chat.proto \
   > encoded.bin */

static uint8_t connect_req_erik[] = {
    /* Header. */
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08,
    /* Payload. */
    0x0a, 0x06, 0x0a, 0x04, 0x45, 0x72, 0x69, 0x6b
};

static uint8_t connect_req_kalle[] = {
    /* Header. */
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x09,
    /* Payload. */
    0x0a, 0x07, 0x0a, 0x05, 0x4b, 0x61, 0x6c, 0x6c, 0x65
};

static uint8_t connect_req_fia[] = {
    /* Header. */
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07,
    /* Payload. */
    0x0a, 0x05, 0x0a, 0x03, 0x46, 0x69, 0x61
};

static uint8_t connect_rsp[] = {
    /* Header. */
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02,
    /* Payload. */
    0x0a, 0x00
};

static struct chat_server_t server;
static struct chat_server_client_t clients[3];
static uint8_t clients_input_buffers[3][128];
static uint8_t message[128];
static uint8_t workspace_in[128];
static uint8_t workspace_out[128];

static void mock_prepare_make_non_blocking(int fd)
{
    fcntl_mock_once(fd, F_GETFL, 0, "");
    fcntl_mock_once(fd, F_SETFL, O_NONBLOCK, "");
}

static void on_connect_req(struct chat_server_t *self_p,
                           struct chat_server_client_t *client_p,
                           struct chat_connect_req_t *message_p)
{
    (void)client_p;

    printf("%s connected.\n", message_p->user_p);

    chat_server_init_connect_rsp(self_p);
    chat_server_reply(self_p);
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
    size_t payload_size;
    struct itimerspec timeout;

    payload_size = (connect_req_size - HEADER_SIZE);

    /* TCP connect. */
    accept_mock_once(LISTENER_FD, client_fd);
    mock_prepare_make_non_blocking(client_fd);
    timerfd_create_mock_once(CLOCK_MONOTONIC, 0, client_to_timer_fd(client_fd));
    memset(&timeout, 0, sizeof(timeout));
    timeout.it_value.tv_sec = 2;
    timerfd_settime_mock_once(client_to_timer_fd(client_fd), 0, 0);
    timerfd_settime_mock_set_new_value_in(&timeout, sizeof(timeout));
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_ADD, client_to_timer_fd(client_fd), 0);
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_ADD, client_fd, 0);

    chat_server_process(&server, LISTENER_FD, EPOLLIN);

    /* Chat connect messages. */
    read_mock_once(client_fd, HEADER_SIZE, HEADER_SIZE);
    read_mock_set_buf_out(&connect_req_buf_p[0], HEADER_SIZE);
    read_mock_once(client_fd, payload_size, payload_size);
    read_mock_set_buf_out(&connect_req_buf_p[HEADER_SIZE], payload_size);
    read_mock_once(client_fd, HEADER_SIZE, -1);
    read_mock_set_errno(EAGAIN);
    write_mock_once(client_fd, sizeof(connect_rsp), sizeof(connect_rsp));
    write_mock_set_buf_in(&connect_rsp[0], sizeof(connect_rsp));

    chat_server_process(&server, client_fd, EPOLLIN);
}

static void mock_prepare_client_destroy(int client_fd)
{
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_DEL, client_fd, 0);
    close_mock_once(client_fd, 0);
    epoll_ctl_mock_once(EPOLL_FD,
                        EPOLL_CTL_DEL,
                        client_to_timer_fd(client_fd),
                        0);
    close_mock_once(client_to_timer_fd(client_fd), 0);
}

static void disconnect_client(int client_fd)
{
    /* TCP disconnect. */
    read_mock_once(client_fd, HEADER_SIZE, -1);
    read_mock_set_errno(EPIPE);
    mock_prepare_client_destroy(client_fd);

    chat_server_process(&server, client_fd, EPOLLIN);
}

static void connect_erik(void)
{
    connect_client(connect_req_erik,
                   sizeof(connect_req_erik),
                   ERIK_FD);
}

static void connect_kalle(void)
{
    connect_client(connect_req_kalle,
                   sizeof(connect_req_kalle),
                   KALLE_FD);
}

static void connect_fia(void)
{
    connect_client(connect_req_fia,
                   sizeof(connect_req_fia),
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

static void start_server_with_three_clients_erik_kalle_and_fia()
{
    int enable;

    ASSERT_EQ(chat_server_init(&server,
                               "tcp://127.0.0.1:6000",
                               &clients[0],
                               3,
                               &clients_input_buffers[0][0],
                               sizeof(clients_input_buffers[0]),
                               &message[0],
                               sizeof(message),
                               &workspace_in[0],
                               sizeof(workspace_in),
                               &workspace_out[0],
                               sizeof(workspace_out),
                               on_connect_req,
                               on_message_ind,
                               EPOLL_FD,
                               NULL), 0);

    /* Start creates a socket and starts listening for clients. */
    socket_mock_once(AF_INET, SOCK_STREAM, 0, LISTENER_FD);
    enable = 1;
    setsockopt_mock_once(LISTENER_FD, SOL_SOCKET, SO_REUSEADDR, sizeof(enable), 0);
    setsockopt_mock_set_optval_in(&enable, sizeof(enable));
    bind_mock_once(LISTENER_FD, sizeof(struct sockaddr_in), 0);
    listen_mock_once(LISTENER_FD, 5, 0);
    mock_prepare_make_non_blocking(LISTENER_FD);
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_ADD, LISTENER_FD, 0);

    ASSERT_EQ(chat_server_start(&server), 0);
}

TEST(connect_and_disconnect_clients)
{
    start_server_with_three_clients_erik_kalle_and_fia();

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
    mock_prepare_client_destroy(KALLE_FD);

    chat_server_stop(&server);
}

TEST(broadcast)
{
    uint8_t message_ind[] = {
        /* Header. */
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10,
        /* Payload. */
        0x12, 0x0e, 0x0a, 0x04, 0x45, 0x72, 0x69, 0x6b,
        0x12, 0x06, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2e
    };
    size_t payload_size;

    start_server_with_three_clients_erik_kalle_and_fia();

    /* Connect clients. */
    connect_erik();
    connect_kalle();
    connect_fia();

    /* Broadcast a message as Fia. */
    payload_size = (sizeof(message_ind) - HEADER_SIZE);
    read_mock_once(FIA_FD, HEADER_SIZE, HEADER_SIZE);
    read_mock_set_buf_out(&message_ind[0], HEADER_SIZE);
    read_mock_once(FIA_FD, payload_size, payload_size);
    read_mock_set_buf_out(&message_ind[HEADER_SIZE], payload_size);
    read_mock_once(FIA_FD, HEADER_SIZE, -1);
    read_mock_set_errno(EAGAIN);
    write_mock_once(FIA_FD, sizeof(message_ind), sizeof(message_ind));
    write_mock_set_buf_in(&message_ind[0], sizeof(message_ind));
    write_mock_once(KALLE_FD, sizeof(message_ind), sizeof(message_ind));
    write_mock_set_buf_in(&message_ind[0], sizeof(message_ind));
    write_mock_once(ERIK_FD, sizeof(message_ind), sizeof(message_ind));
    write_mock_set_buf_in(&message_ind[0], sizeof(message_ind));

    chat_server_process(&server, FIA_FD, EPOLLIN);
}

TEST(keep_alive)
{
    uint8_t ping_req[] = {
        /* Header. */
        0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00
    };
    uint8_t ping_rsp[] = {
        /* Header. */
        0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00
    };

    start_server_with_three_clients_erik_kalle_and_fia();

    /* Only one client needed. */
    connect_erik();

    /* Send a ping message, the keep alive timer should be restarted
       and a pong mesage should be sent. */
    read_mock_once(ERIK_FD, HEADER_SIZE, HEADER_SIZE);
    read_mock_set_buf_out(&ping_req[0], HEADER_SIZE);
    read_mock_once(ERIK_FD, HEADER_SIZE, -1);
    read_mock_set_errno(EAGAIN);

    timerfd_settime_mock_once(ERIK_TIMER_FD, 0, 0);

    write_mock_once(ERIK_FD, sizeof(ping_rsp), sizeof(ping_rsp));
    write_mock_set_buf_in(&ping_rsp[0], sizeof(ping_rsp));

    chat_server_process(&server, ERIK_FD, EPOLLIN);

    /* Make the keep alive timer expire. The client should be
       disconnected. */
    read_mock_once(ERIK_TIMER_FD, sizeof(uint64_t), sizeof(uint64_t));
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_DEL, ERIK_FD, 0);
    close_mock_once(ERIK_FD, 0);
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_DEL, ERIK_TIMER_FD, 0);
    close_mock_once(ERIK_TIMER_FD, 0);

    chat_server_process(&server, ERIK_TIMER_FD, EPOLLIN);
}

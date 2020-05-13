#include <dbg.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include "nala.h"
#include "chat_client.h"

#define EPOLL_FD                                       9
#define SERVER_FD                                     10
#define KEEP_ALIVE_TIMER_FD                           17
#define RECONNECT_TIMER_FD                            18

#define HEADER_SIZE sizeof(struct chat_common_header_t)

static inline int client_to_timer_fd(int client_fd)
{
    return (client_fd + 3);
}

static uint8_t connect_req[] = {
    /* Header. */
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08,
    /* Payload. */
    0x0a, 0x06, 0x0a, 0x04, 0x45, 0x72, 0x69, 0x6b
};

static uint8_t connect_rsp[] = {
    /* Header. */
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02,
    /* Payload. */
    0x0a, 0x00
};

static struct chat_client_t client;
static uint8_t message[128];
static uint8_t workspace_in[128];
static uint8_t workspace_out[128];

static void mock_prepare_make_non_blocking(int fd)
{
    fcntl_mock_once(fd, F_GETFL, 0, "");
    fcntl_mock_once(fd, F_SETFL, O_NONBLOCK, "");
}

static void on_connected(struct chat_client_t *self_p)
{
    struct chat_connect_req_t *message_p;

    message_p = chat_client_init_connect_req(self_p);
    message_p->user_p = self_p->user_p;
    chat_client_send(self_p);
}

static void on_disconnected(struct chat_client_t *self_p)
{
    printf("Disconnected from the server.\n");

    self_p->connected = false;
}

void on_connect_rsp(struct chat_client_t *self_p,
                    struct chat_connect_rsp_t *message_p)
{
    (void)self_p;
    (void)message_p;

    FAIL("Must be mocked.");
}

static void on_message_ind(struct chat_client_t *self_p,
                           struct chat_message_ind_t *message_p)
{
    (void)self_p;

    printf("<%s> %s\n", message_p->user_p, message_p->text_p);
}

static void mock_prepare_connect_to_server()
{
    struct itimerspec timeout;

    socket_mock_once(AF_INET, SOCK_STREAM, 0, SERVER_FD);
    connect_mock_once(SERVER_FD, sizeof(struct sockaddr_in), 0);
    mock_prepare_make_non_blocking(SERVER_FD);
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_ADD, SERVER_FD, 0);
    timerfd_create_mock_once(CLOCK_MONOTONIC, 0, KEEP_ALIVE_TIMER_FD);
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_ADD, KEEP_ALIVE_TIMER_FD, 0);
    memset(&timeout, 0, sizeof(timeout));
    timeout.it_value.tv_sec = 2;
    timerfd_settime_mock_once(KEEP_ALIVE_TIMER_FD, 0, 0);
    timerfd_settime_mock_set_new_value_in(&timeout, sizeof(timeout));
    write_mock_once(SERVER_FD, sizeof(connect_req), sizeof(connect_req));
    write_mock_set_buf_in(&connect_req[0], sizeof(connect_req));
}

static void mock_prepare_start_reconnect_timer()
{
    struct itimerspec timeout;

    timerfd_create_mock_once(CLOCK_MONOTONIC, 0, RECONNECT_TIMER_FD);
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_ADD, RECONNECT_TIMER_FD, 0);
    memset(&timeout, 0, sizeof(timeout));
    timeout.it_value.tv_sec = 1;
    timerfd_settime_mock_once(RECONNECT_TIMER_FD, 0, 0);
    timerfd_settime_mock_set_new_value_in(&timeout, sizeof(timeout));
}

static void mock_prepare_close_fd(int fd)
{
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_DEL, fd, 0);
    close_mock_once(fd, 0);
}

static void start_client_and_connect_to_server()
{
    size_t payload_size;

    ASSERT_EQ(chat_client_init(&client,
                               "Erik",
                               "tcp://127.0.0.1:6000",
                               &message[0],
                               sizeof(message),
                               &workspace_in[0],
                               sizeof(workspace_in),
                               &workspace_out[0],
                               sizeof(workspace_out),
                               on_connected,
                               on_disconnected,
                               on_connect_rsp,
                               on_message_ind,
                               EPOLL_FD,
                               NULL), 0);

    /* TCP connect to the server and send ConnectReq. */
    mock_prepare_connect_to_server();

    chat_client_start(&client);

    /* Server responds with ConnectRsp. */
    payload_size = (sizeof(connect_rsp) - HEADER_SIZE);
    read_mock_once(SERVER_FD, HEADER_SIZE, HEADER_SIZE);
    read_mock_set_buf_out(&connect_rsp[0], HEADER_SIZE);
    read_mock_once(SERVER_FD, payload_size, payload_size);
    read_mock_set_buf_out(&connect_rsp[HEADER_SIZE], payload_size);
    read_mock_once(SERVER_FD, HEADER_SIZE, -1);
    read_mock_set_errno(EAGAIN);
    on_connect_rsp_mock_once();

    chat_client_process(&client, SERVER_FD, EPOLLIN);
}

TEST(connect_and_disconnect)
{
    start_client_and_connect_to_server();

    /* Disconnect from the server. */
    mock_prepare_close_fd(SERVER_FD);
    mock_prepare_close_fd(KEEP_ALIVE_TIMER_FD);

    chat_client_stop(&client);
}

TEST(connect_successful_on_second_attempt)
{
    ASSERT_EQ(chat_client_init(&client,
                               "Erik",
                               "tcp://127.0.0.1:6000",
                               &message[0],
                               sizeof(message),
                               &workspace_in[0],
                               sizeof(workspace_in),
                               &workspace_out[0],
                               sizeof(workspace_out),
                               on_connected,
                               on_disconnected,
                               on_connect_rsp,
                               on_message_ind,
                               EPOLL_FD,
                               NULL), 0);

    /* TCP connect to the server fails. Reconnect timer should be
       started. */
    socket_mock_once(AF_INET, SOCK_STREAM, 0, SERVER_FD);
    connect_mock_once(SERVER_FD, sizeof(struct sockaddr_in), -1);
    close_mock_once(SERVER_FD, 0);
    mock_prepare_start_reconnect_timer();

    chat_client_start(&client);

    /* Make the reconnect timer expire and then successfully connect
       to the server. */
    mock_prepare_close_fd(RECONNECT_TIMER_FD);
    mock_prepare_connect_to_server();

    chat_client_process(&client, RECONNECT_TIMER_FD, EPOLLIN);
}

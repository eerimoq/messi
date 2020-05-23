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

#define HEADER_SIZE sizeof(struct messi_header_t)

static inline int client_to_timer_fd(int client_fd)
{
    return (client_fd + 3);
}

static uint8_t connect_req[] = {
    /* Header. */
    0x01, 0x00, 0x00, 0x08,
    /* Payload. */
    0x0a, 0x06, 0x0a, 0x04, 0x45, 0x72, 0x69, 0x6b
};

static uint8_t connect_rsp[] = {
    /* Header. */
    0x02, 0x00, 0x00, 0x02,
    /* Payload. */
    0x0a, 0x00
};

static uint8_t message_ind[] = {
    /* Header. */
    0x02, 0x00, 0x00, 0x10,
    /* Payload. */
    0x12, 0x0e, 0x0a, 0x04, 0x45, 0x72, 0x69, 0x6b,
    0x12, 0x06, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2e
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

static void mock_prepare_write(int fd, const uint8_t *buf_p, size_t size)
{
    write_mock_once(fd, size, size);
    write_mock_set_buf_in(buf_p, size);
}

static void mock_prepare_read(int fd, uint8_t *buf_p, size_t size)
{
    read_mock_once(fd, size, size);
    read_mock_set_buf_out(buf_p, size);
}

void client_on_disconnected(struct chat_client_t *self_p,
                            enum messi_disconnect_reason_t disconnect_reason)
{
    (void)self_p;
    (void)disconnect_reason;

    FAIL("Must be mocked.");
}

void client_on_connect_rsp(struct chat_client_t *self_p,
                           struct chat_connect_rsp_t *message_p)
{
    (void)self_p;
    (void)message_p;

    FAIL("Must be mocked.");
}

void client_on_message_ind(struct chat_client_t *self_p,
                           struct chat_message_ind_t *message_p)
{
    (void)self_p;
    (void)message_p;

    FAIL("Must be mocked.");
}

static void assert_on_message_ind(struct chat_message_ind_t *actual_p,
                                  struct chat_message_ind_t *expected_p,
                                  size_t size)
{
    ASSERT_EQ(size, sizeof(*actual_p));
    ASSERT_EQ(actual_p->user_p, expected_p->user_p);
    ASSERT_EQ(actual_p->text_p, expected_p->text_p);
}

static void mock_prepare_connect(int fd, const char *address_p, int port, int res)
{
    struct sockaddr_in addr;

    connect_mock_once(fd, sizeof(addr), res);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton(address_p, (struct in_addr *)&addr.sin_addr.s_addr);
    connect_mock_set_addr_in((const struct sockaddr *)&addr, sizeof(addr));
}

static void mock_prepare_connect_to_server(const char *address_p, int port)
{
    struct itimerspec timeout;

    socket_mock_once(AF_INET, SOCK_STREAM, 0, SERVER_FD);
    mock_prepare_connect(SERVER_FD, address_p, port, 0);
    mock_prepare_make_non_blocking(SERVER_FD);
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_ADD, SERVER_FD, 0);
    timerfd_create_mock_once(CLOCK_MONOTONIC, 0, KEEP_ALIVE_TIMER_FD);
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_ADD, KEEP_ALIVE_TIMER_FD, 0);
    memset(&timeout, 0, sizeof(timeout));
    timeout.it_value.tv_sec = 2;
    timerfd_settime_mock_once(KEEP_ALIVE_TIMER_FD, 0, 0);
    timerfd_settime_mock_set_new_value_in(&timeout, sizeof(timeout));
    mock_prepare_write(SERVER_FD, &connect_req[0], sizeof(connect_req));
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

static void mock_prepare_start_keep_alive_timer()
{
    struct itimerspec timeout;

    memset(&timeout, 0, sizeof(timeout));
    timeout.it_value.tv_sec = 2;
    timerfd_settime_mock_once(KEEP_ALIVE_TIMER_FD, 0, 0);
    timerfd_settime_mock_set_new_value_in(&timeout, sizeof(timeout));
}

static void mock_prepare_close_fd(int fd)
{
    epoll_ctl_mock_once(EPOLL_FD, EPOLL_CTL_DEL, fd, 0);
    close_mock_once(fd, 0);
}

static void mock_prepare_read_try_again()
{
    read_mock_once(SERVER_FD, HEADER_SIZE, -1);
    read_mock_set_errno(EAGAIN);
}

static void mock_prepare_timer_read(int fd)
{
    read_mock_once(fd, sizeof(uint64_t), sizeof(uint64_t));
}

static void mock_prepare_disconnect()
{
    mock_prepare_close_fd(SERVER_FD);
    mock_prepare_close_fd(KEEP_ALIVE_TIMER_FD);
}

static void mock_prepare_disconnect_and_start_reconnect_timer(
    enum messi_disconnect_reason_t disconnect_reason)
{
    mock_prepare_disconnect();
    client_on_disconnected_mock_once(disconnect_reason);
    mock_prepare_start_reconnect_timer();
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
                               client_on_disconnected,
                               client_on_connect_rsp,
                               client_on_message_ind,
                               EPOLL_FD,
                               NULL), 0);

    /* TCP connect to the server and send ConnectReq. */
    mock_prepare_connect_to_server("127.0.0.1", 6000);

    chat_client_start(&client);

    /* Server responds with ConnectRsp. */
    payload_size = (sizeof(connect_rsp) - HEADER_SIZE);
    mock_prepare_read(SERVER_FD, &connect_rsp[0], HEADER_SIZE);
    mock_prepare_read(SERVER_FD, &connect_rsp[HEADER_SIZE], payload_size);
    mock_prepare_read_try_again();
    client_on_connect_rsp_mock_once();

    chat_client_process(&client, SERVER_FD, EPOLLIN);
}

TEST(connect_and_disconnect)
{
    start_client_and_connect_to_server();

    /* Disconnect from the server. */
    mock_prepare_disconnect();

    chat_client_stop(&client);
}

TEST(connect_successful_on_second_attempt)
{
    ASSERT_EQ(chat_client_init(&client,
                               "Erik",
                               "tcp://10.20.30.40:40234",
                               &message[0],
                               sizeof(message),
                               &workspace_in[0],
                               sizeof(workspace_in),
                               &workspace_out[0],
                               sizeof(workspace_out),
                               on_connected,
                               client_on_disconnected,
                               client_on_connect_rsp,
                               client_on_message_ind,
                               EPOLL_FD,
                               NULL), 0);

    /* TCP connect to the server fails. Reconnect timer should be
       started. */
    socket_mock_once(AF_INET, SOCK_STREAM, 0, SERVER_FD);
    mock_prepare_connect(SERVER_FD, "10.20.30.40", 40234, -1);
    close_mock_once(SERVER_FD, 0);
    mock_prepare_start_reconnect_timer();

    chat_client_start(&client);

    /* Make the reconnect timer expire and then successfully connect
       to the server. */
    mock_prepare_close_fd(RECONNECT_TIMER_FD);
    mock_prepare_connect_to_server("10.20.30.40", 40234);

    chat_client_process(&client, RECONNECT_TIMER_FD, EPOLLIN);
}

TEST(keep_alive)
{
    uint8_t ping[] = {
        /* Header. */
        0x03, 0x00, 0x00, 0x00
    };
    uint8_t pong[] = {
        /* Header. */
        0x04, 0x00, 0x00, 0x00
    };

    start_client_and_connect_to_server();

    /* Make the keep alive timer expire and receive a ping message. */
    mock_prepare_timer_read(KEEP_ALIVE_TIMER_FD);
    mock_prepare_start_keep_alive_timer();
    mock_prepare_write(SERVER_FD, &ping[0], sizeof(ping));

    chat_client_process(&client, KEEP_ALIVE_TIMER_FD, EPOLLIN);

    /* Send a pong message to the client. */
    mock_prepare_read(SERVER_FD, &pong[0], sizeof(pong));
    mock_prepare_read_try_again();

    chat_client_process(&client, SERVER_FD, EPOLLIN);

    /* Make the keep alive timer expire again, receive a ping message,
       but do not respond with a pong message. */
    mock_prepare_timer_read(KEEP_ALIVE_TIMER_FD);
    mock_prepare_start_keep_alive_timer();
    mock_prepare_write(SERVER_FD, &ping[0], sizeof(ping));

    chat_client_process(&client, KEEP_ALIVE_TIMER_FD, EPOLLIN);

    /* Make the keep alive timer expire to detect the missing pong and
       start the reconnect timer. Verify that the disconnected
       callback is called. */
    mock_prepare_timer_read(KEEP_ALIVE_TIMER_FD);
    mock_prepare_disconnect_and_start_reconnect_timer(
        messi_disconnect_reason_keep_alive_timeout_t);

    chat_client_process(&client, KEEP_ALIVE_TIMER_FD, EPOLLIN);

    /* Make the reconnect timer expire and perform a successful
       connect to the server. */
    mock_prepare_close_fd(RECONNECT_TIMER_FD);
    mock_prepare_connect_to_server("127.0.0.1", 6000);

    chat_client_process(&client, RECONNECT_TIMER_FD, EPOLLIN);
}

TEST(keep_alive_ping_write_error_disconnect)
{
    uint8_t ping[] = {
        /* Header. */
        0x02, 0x00, 0x00, 0x00
    };

    start_client_and_connect_to_server();

    /* Make the keep alive timer expire and receive a ping message. */
    mock_prepare_timer_read(KEEP_ALIVE_TIMER_FD);
    mock_prepare_start_keep_alive_timer();
    write_mock_once(SERVER_FD, sizeof(ping), -1);
    write_mock_set_errno(EIO);
    mock_prepare_disconnect_and_start_reconnect_timer(
        messi_disconnect_reason_general_error_t);

    chat_client_process(&client, KEEP_ALIVE_TIMER_FD, EPOLLIN);
}

TEST(server_disconnects)
{
    start_client_and_connect_to_server();

    /* Make the server disconnect from the client. */
    read_mock_once(SERVER_FD, HEADER_SIZE, 0);
    mock_prepare_disconnect_and_start_reconnect_timer(
        messi_disconnect_reason_connection_closed_t);

    chat_client_process(&client, SERVER_FD, EPOLLIN);
}

TEST(partial_message_read)
{
    struct chat_message_ind_t message;

    start_client_and_connect_to_server();

    /* Read a message partially (in multiple chunks). */
    read_mock_once(SERVER_FD, 4, 1);
    read_mock_set_buf_out(&message_ind[0], 1);
    read_mock_once(SERVER_FD, 3, 2);
    read_mock_set_buf_out(&message_ind[1], 2);
    read_mock_once(SERVER_FD, 1, 1);
    read_mock_set_buf_out(&message_ind[3], 1);
    read_mock_once(SERVER_FD, 16, 5);
    read_mock_set_buf_out(&message_ind[4], 5);
    read_mock_once(SERVER_FD, 11, -1);
    read_mock_set_errno(EAGAIN);

    chat_client_process(&client, SERVER_FD, EPOLLIN);

    /* Read the end of the message. */
    mock_prepare_read(SERVER_FD, &message_ind[9], 11);
    mock_prepare_read_try_again(SERVER_FD);
    client_on_message_ind_mock_once();
    message.user_p = "Erik";
    message.text_p = "Hello.";
    client_on_message_ind_mock_set_message_p_in(&message, sizeof(message));
    client_on_message_ind_mock_set_message_p_in_assert(assert_on_message_ind);

    chat_client_process(&client, SERVER_FD, EPOLLIN);
}

TEST(send_another_message_after_first_failed)
{
    struct chat_message_ind_t *message_p;

    start_client_and_connect_to_server();

    /* Send a message to the server with write failing. The client
       will be put in the pending disconnect state. */
    write_mock_once(SERVER_FD, sizeof(message_ind), -1);
    mock_prepare_close_fd(SERVER_FD);

    message_p = chat_client_init_message_ind(&client);
    message_p->user_p = "Erik";
    message_p->text_p = "Hello.";
    chat_client_send(&client);

    /* Send another message. */
    write_mock_once(-1, sizeof(message_ind), -1);

    chat_client_send(&client);

    /* Make the keep alive timer expire. The client should be
       disconnected and start the reconnect timer. */
    mock_prepare_timer_read(KEEP_ALIVE_TIMER_FD);
    mock_prepare_start_keep_alive_timer();
    write_mock_once(-1, HEADER_SIZE, -1);
    mock_prepare_close_fd(KEEP_ALIVE_TIMER_FD);
    client_on_disconnected_mock_once(messi_disconnect_reason_connection_closed_t);
    mock_prepare_start_reconnect_timer();

    chat_client_process(&client, KEEP_ALIVE_TIMER_FD, EPOLLIN);
}

TEST(encode_error)
{
    struct chat_message_ind_t *message_p;

    start_client_and_connect_to_server();

    mock_prepare_close_fd(SERVER_FD);

    message_p = chat_client_init_message_ind(&client);
    message_p->user_p = (
        "A very long string................................................"
        ".................................................................."
        ".................................................................."
        ".................................................................."
        ".................................................................."
        ".................................................................."
        ".................................................................."
        "........");
    message_p->text_p = "Hello.";
    chat_client_send(&client);

    /* Make the keep alive timer expire. The client should be
       disconnected and start the reconnect timer. */
    mock_prepare_timer_read(KEEP_ALIVE_TIMER_FD);
    mock_prepare_start_keep_alive_timer();
    write_mock_once(-1, HEADER_SIZE, -1);
    mock_prepare_close_fd(KEEP_ALIVE_TIMER_FD);
    client_on_disconnected_mock_once(messi_disconnect_reason_message_encode_error_t);
    mock_prepare_start_reconnect_timer();

    chat_client_process(&client, KEEP_ALIVE_TIMER_FD, EPOLLIN);
}

TEST(decode_error)
{
    static uint8_t malformed_message[] = {
        /* Header. */
        0x02, 0x00, 0x00, 0x01,
        /* Payload. */
        0x99
    };

    start_client_and_connect_to_server();

    mock_prepare_disconnect_and_start_reconnect_timer(
        messi_disconnect_reason_message_decode_error_t);

    mock_prepare_read(SERVER_FD, &malformed_message[0], 4);
    mock_prepare_read(SERVER_FD, &malformed_message[4], 1);
    read_mock_once(-1, HEADER_SIZE, -1);
    read_mock_set_errno(EIO);

    chat_client_process(&client, SERVER_FD, EPOLLIN);
}

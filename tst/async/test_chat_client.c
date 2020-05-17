#include "nala.h"
#include "chat_client.h"

#define HEADER_SIZE sizeof(struct messi_header_t)

static struct async_t async;
static struct chat_client_t client;
static uint8_t message[128];
static uint8_t workspace_in[128];
static uint8_t workspace_out[128];
static struct nala_async_stcp_client_init_params_t *stcp_init_params_p;
static struct nala_async_timer_init_params_t *keep_alive_params_p;
static struct nala_async_timer_init_params_t *reconnect_params_p;

static uint8_t connect_req[] = {
    /* Header. */
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08,
    /* Payload. */
    0x0a, 0x06, 0x0a, 0x04, 0x45, 0x72, 0x69, 0x6b
};

static uint8_t connect_rsp[] = {
    /* Header. */
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,
    /* Payload. */
    0x0a, 0x00
};

static uint8_t message_ind[] = {
    /* Header. */
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10,
    /* Payload. */
    0x12, 0x0e, 0x0a, 0x04, 0x45, 0x72, 0x69, 0x6b,
    0x12, 0x06, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2e
};

static void assert_on_message_ind(struct chat_message_ind_t *actual_p,
                                  struct chat_message_ind_t *expected_p,
                                  size_t size)
{
    ASSERT_EQ(size, sizeof(*actual_p));
    ASSERT_EQ(actual_p->user_p, expected_p->user_p);
    ASSERT_EQ(actual_p->text_p, expected_p->text_p);
}

static void on_connected(struct chat_client_t *self_p)
{
    struct chat_connect_req_t *message_p;

    message_p = chat_client_init_connect_req(self_p);
    message_p->user_p = self_p->user_p;
    chat_client_send(self_p);
}

static void mock_prepare_start_keep_alive_timer()
{
    async_timer_start_mock_once();
    async_timer_start_mock_set_self_p_in_pointer(keep_alive_params_p->self_p);
}

static void mock_prepare_stop_keep_alive_timer()
{
    async_timer_stop_mock_once();
    async_timer_stop_mock_set_self_p_in_pointer(keep_alive_params_p->self_p);
}

static void mock_prepare_start_reconnect_timer()
{
    async_timer_start_mock_once();
    async_timer_start_mock_set_self_p_in_pointer(reconnect_params_p->self_p);
}

static void mock_prepare_write(const uint8_t *buf_p, size_t size)
{
    async_stcp_client_write_mock_once(size);
    async_stcp_client_write_mock_set_buf_p_in(buf_p, size);
}

static void mock_prepare_read_try_again()
{
    async_stcp_client_read_mock_once(HEADER_SIZE, 0);
}

static void mock_prepare_read(size_t size, uint8_t *buf_p, size_t res)
{
    async_stcp_client_read_mock_once(size, res);
    async_stcp_client_read_mock_set_buf_p_out(buf_p, res);
}

void client_on_disconnected(struct chat_client_t *self_p)
{
    (void)self_p;

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

static void start_client_and_connect_to_server()
{
    int stcp_init_handle;
    int keep_alive_handle;
    int reconnect_handle;
    size_t payload_size;

    stcp_init_handle = async_stcp_client_init_mock_once();
    keep_alive_handle = async_timer_init_mock_once(2000, 0);
    reconnect_handle = async_timer_init_mock_once(1000, 0);

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
                               &async), 0);

    /* Get callbacks. */
    stcp_init_params_p = async_stcp_client_init_mock_get_params_in(stcp_init_handle);
    keep_alive_params_p = async_timer_init_mock_get_params_in(keep_alive_handle);
    reconnect_params_p = async_timer_init_mock_get_params_in(reconnect_handle);

    /* Start the client. */
    async_stcp_client_connect_mock_once("127.0.0.1", 6000);

    chat_client_start(&client);

    /* Successful connection. */
    mock_prepare_start_keep_alive_timer();
    mock_prepare_write(&connect_req[0], sizeof(connect_req));

    stcp_init_params_p->on_connected(stcp_init_params_p->self_p, 0);

    /* Server responds with ConnectRsp. */
    payload_size = (sizeof(connect_rsp) - HEADER_SIZE);
    mock_prepare_read(HEADER_SIZE, &connect_rsp[0], HEADER_SIZE);
    mock_prepare_read(payload_size, &connect_rsp[HEADER_SIZE], payload_size);
    mock_prepare_read_try_again();
    client_on_connect_rsp_mock_once();

    stcp_init_params_p->on_input(stcp_init_params_p->self_p);
}

TEST(connect_and_disconnect)
{
    start_client_and_connect_to_server();

    /* Disconnect from the server. */
    async_stcp_client_disconnect_mock_once();
    async_timer_stop_mock_once();
    async_timer_stop_mock_set_self_p_in_pointer(reconnect_params_p->self_p);
    mock_prepare_stop_keep_alive_timer();

    chat_client_stop(&client);
}

TEST(connect_successful_on_second_attempt)
{
    int stcp_init_handle;
    int keep_alive_handle;
    int reconnect_handle;

    stcp_init_handle = async_stcp_client_init_mock_once();
    keep_alive_handle = async_timer_init_mock_once(2000, 0);
    reconnect_handle = async_timer_init_mock_once(1000, 0);

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
                               &async), 0);

    /* Get callbacks. */
    stcp_init_params_p = async_stcp_client_init_mock_get_params_in(stcp_init_handle);
    keep_alive_params_p = async_timer_init_mock_get_params_in(keep_alive_handle);
    reconnect_params_p = async_timer_init_mock_get_params_in(reconnect_handle);

    /* Start the client. */
    async_stcp_client_connect_mock_once("127.0.0.1", 6000);

    chat_client_start(&client);

    /* Successful connection. */
    mock_prepare_start_reconnect_timer();

    stcp_init_params_p->on_connected(stcp_init_params_p->self_p, -1);

    /* Make the reconnect timer expire and then successfully connect
       to the server. */
    async_stcp_client_connect_mock_once("127.0.0.1", 6000);

    reconnect_params_p->on_timeout(reconnect_params_p->obj_p);

    /* Successful connection. */
    mock_prepare_start_keep_alive_timer();
    mock_prepare_write(&connect_req[0], sizeof(connect_req));

    stcp_init_params_p->on_connected(stcp_init_params_p->self_p, 0);
}

TEST(keep_alive)
{
    uint8_t ping[] = {
        /* Header. */
        0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00
    };
    uint8_t pong[] = {
        /* Header. */
        0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00
    };

    start_client_and_connect_to_server();

    /* Make the keep alive timer expire and receive a ping message. */
    mock_prepare_start_keep_alive_timer();
    mock_prepare_write(&ping[0], sizeof(ping));

    keep_alive_params_p->on_timeout(keep_alive_params_p->obj_p);

    /* Send a pong message to the client. */
    mock_prepare_read(sizeof(pong), &pong[0], sizeof(pong));
    mock_prepare_read_try_again();

    stcp_init_params_p->on_input(stcp_init_params_p->self_p);

    /* Make the keep alive timer expire again, receive a ping message,
       but do not respond with a pong message. */
    mock_prepare_start_keep_alive_timer();
    mock_prepare_write(&ping[0], sizeof(ping));

    keep_alive_params_p->on_timeout(keep_alive_params_p->obj_p);

    /* Make the keep alive timer expire to detect the missing pong and
       start the reconnect timer. Verify that the disconnected
       callback is called. */
    async_stcp_client_disconnect_mock_once();
    client_on_disconnected_mock_once();
    keep_alive_params_p->on_timeout(keep_alive_params_p->obj_p);

    /* Make the reconnect timer expire and perform a successful
       connect to the server. */
    async_stcp_client_connect_mock_once("127.0.0.1", 6000);

    reconnect_params_p->on_timeout(reconnect_params_p->obj_p);

    mock_prepare_start_keep_alive_timer();
    mock_prepare_write(&connect_req[0], sizeof(connect_req));

    stcp_init_params_p->on_connected(stcp_init_params_p->self_p, 0);
}

TEST(server_disconnects)
{
    start_client_and_connect_to_server();

    /* Make the server disconnect from the client. */
    mock_prepare_stop_keep_alive_timer();
    client_on_disconnected_mock_once();
    mock_prepare_start_reconnect_timer();

    stcp_init_params_p->on_disconnected(stcp_init_params_p->self_p);
}

TEST(partial_message_read)
{
    struct chat_message_ind_t message;

    start_client_and_connect_to_server();

    /* Read a message partially (in multiple chunks). */
    mock_prepare_read(8, &message_ind[0], 1);
    mock_prepare_read(7, &message_ind[1], HEADER_SIZE - 2);
    mock_prepare_read(1, &message_ind[7], 1);
    mock_prepare_read(16, &message_ind[8], 5);
    async_stcp_client_read_mock_once(11, 0);

    stcp_init_params_p->on_input(stcp_init_params_p->self_p);

    /* Read the end of the message. */
    mock_prepare_read(11, &message_ind[13], 11);
    mock_prepare_read_try_again();
    client_on_message_ind_mock_once();
    message.user_p = "Erik";
    message.text_p = "Hello.";
    client_on_message_ind_mock_set_message_p_in(&message, sizeof(message));
    client_on_message_ind_mock_set_message_p_in_assert(assert_on_message_ind);

    stcp_init_params_p->on_input(stcp_init_params_p->self_p);
}

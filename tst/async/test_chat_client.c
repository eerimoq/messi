#include "nala.h"
#include "chat_client.h"

static struct async_t async;
static struct chat_client_t client;
static uint8_t message[128];
static uint8_t workspace_in[128];
static uint8_t workspace_out[128];

void client_on_connected(struct chat_client_t *self_p)
{
    (void)self_p;

    FAIL("Must be mocked.");
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

TEST(connect_and_disconnect)
{
    async_timer_start_mock_none();
    async_timer_stop_mock_none();
    async_stcp_client_connect_mock_none();
    async_stcp_client_disconnect_mock_none();
    async_stcp_client_write_mock_none();
    async_stcp_client_read_mock_none();

    async_stcp_client_init_mock_once();
    async_timer_init_mock_once(2000, 0);
    async_timer_init_mock_once(1000, 0);

    ASSERT_EQ(chat_client_init(&client,
                               "Erik",
                               "tcp://127.0.0.1:6000",
                               &message[0],
                               sizeof(message),
                               &workspace_in[0],
                               sizeof(workspace_in),
                               &workspace_out[0],
                               sizeof(workspace_out),
                               client_on_connected,
                               client_on_disconnected,
                               client_on_connect_rsp,
                               client_on_message_ind,
                               &async), 0);
}

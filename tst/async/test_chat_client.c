#include "nala.h"
#include "chat_client.h"

TEST(connect_and_disconnect)
{
    async_timer_init_mock_none();
    async_timer_start_mock_none();
    async_timer_stop_mock_none();
    async_stcp_client_init_mock_none();
    async_stcp_client_connect_mock_none();
    async_stcp_client_disconnect_mock_none();
    async_stcp_client_write_mock_none();
    async_stcp_client_read_mock_none();
}

#include "nala.h"
#include "chat_server.h"

TEST(connect_and_disconnect_clients)
{
    async_stcp_server_init_mock_none();
    async_stcp_server_add_client_mock_none();
    async_stcp_server_start_mock_none();
    async_stcp_server_stop_mock_none();
    async_stcp_server_client_write_mock_none();
}

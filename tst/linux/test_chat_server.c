#include "nala.h"
#include "chat_server.h"

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

TEST(three_clients)
{
    struct chat_server_t server;
    int erik_fd;
    int kalle_fd;
    int fia_fd;

    ASSERT_EQ(chat_server_init(&server), 0);
    ASSERT_EQ(chat_server_start(&server), 0);

    /* Connect and disconnect clients. */
    erik_fd = connect_client(connect_req_payload_erik,
                             sizeof(connect_req_payload_erik));
    kalle_fd = connect_client(connect_req_payload_kalle,
                              sizeof(connect_req_payload_kalle));
    fia_fd = connect_client(connect_req_payload_fia,
                            sizeof(connect_req_payload_fia));
    disconnect_client(kalle_fd);
    kalle_fd = connect_client(kalle_fd);
    disconnect_client(erik_fd);
    disconnect_client(fia_fd);

    /* Stop with Kalle connected. */
    chat_server_stop();
}

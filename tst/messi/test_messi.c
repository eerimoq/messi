#include "nala.h"
#include "messi.h"

TEST(parse_uri_tcp)
{
    const char uri[] = "tcp://127.0.0.1:6000";
    char host[16];
    int port;

    ASSERT_EQ(messi_parse_tcp_uri(&uri[0], &host[0], sizeof(host), &port), 0);
    ASSERT_EQ(&host[0], "127.0.0.1");
    ASSERT_EQ(port, 6000);
}

TEST(parse_uri_tcp_any_address)
{
    const char uri[] = "tcp://:6000";
    char host[16];
    int port;

    ASSERT_EQ(messi_parse_tcp_uri(&uri[0], &host[0], sizeof(host), &port), 0);
    ASSERT_EQ(&host[0], "");
    ASSERT_EQ(port, 6000);
}

TEST(parse_uri_tcp_long_address)
{
    const char uri[] = "tcp://123.456.789.876:6001";
    char host[16];
    int port;

    ASSERT_EQ(messi_parse_tcp_uri(&uri[0], &host[0], sizeof(host), &port), 0);
    ASSERT_EQ(&host[0], "123.456.789.876");
    ASSERT_EQ(port, 6001);
}

TEST(parse_uri_tcp_small_host_buffer)
{
    const char uri[] = "tcp://127.0.0.1:6000";
    char host[9];
    int port;

    ASSERT_EQ(messi_parse_tcp_uri(&uri[0], &host[0], sizeof(host), &port), -1);
}

TEST(parse_uri_tcp_bad_scheme)
{
    const char uri[] = "foo://127.0.0.1:6000";
    char host[16];
    int port;

    ASSERT_EQ(messi_parse_tcp_uri(&uri[0], &host[0], sizeof(host), &port), -1);
}

TEST(parse_uri_tcp_no_colon_before_port)
{
    const char uri[] = "tcp://127.0.0.1 6000";
    char host[16];
    int port;

    ASSERT_EQ(messi_parse_tcp_uri(&uri[0], &host[0], sizeof(host), &port), -1);
}

TEST(parse_uri_tcp_only_scheme)
{
    const char uri[] = "tcp://";
    char host[16];
    int port;

    ASSERT_EQ(messi_parse_tcp_uri(&uri[0], &host[0], sizeof(host), &port), -1);
}

TEST(disconnect_reason_string)
{
    ASSERT_EQ(messi_disconnect_reason_string(
                  messi_disconnect_reason_message_encode_error_t),
              "Message encode error.");
    ASSERT_EQ(messi_disconnect_reason_string(
                  messi_disconnect_reason_message_decode_error_t),
              "Message decode error.");
    ASSERT_EQ(messi_disconnect_reason_string(
                  messi_disconnect_reason_connection_closed_t),
              "Connection closed.");
    ASSERT_EQ(messi_disconnect_reason_string(
                  messi_disconnect_reason_keep_alive_timeout_t),
              "Keep alive timeout.");
    ASSERT_EQ(messi_disconnect_reason_string(
                  messi_disconnect_reason_general_error_t),
              "General error.");
    ASSERT_EQ(messi_disconnect_reason_string(
                  messi_disconnect_reason_message_too_big_t),
              "Message too big.");
}

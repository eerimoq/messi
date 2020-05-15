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

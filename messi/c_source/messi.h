/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020, Erik Moqvist
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This file is part of the Messi project.
 */

#ifndef MESSI_H
#define MESSI_H

#include <stdint.h>
#include <arpa/inet.h>

/* Message types. */
#define MESSI_MESSAGE_TYPE_CLIENT_TO_SERVER_USER 1
#define MESSI_MESSAGE_TYPE_SERVER_TO_CLIENT_USER 2
#define MESSI_MESSAGE_TYPE_PING                  3
#define MESSI_MESSAGE_TYPE_PONG                  4

typedef int (*messi_epoll_ctl_t)(int epoll_fd, int op, int fd, uint32_t events);

struct messi_buffer_t {
    uint8_t *buf_p;
    size_t size;
};

struct messi_header_t {
    uint8_t type;
    uint8_t size[3];
} __attribute__ ((packed));

static inline void messi_header_set_size(struct messi_header_t *header_p,
                                         uint32_t size)
{
    header_p->size[0] = (size >> 16);
    header_p->size[1] = (size >> 8);
    header_p->size[2] = (size >> 0);
}

static inline uint32_t messi_header_get_size(struct messi_header_t *header_p)
{
    return (((uint32_t)header_p->size[0] << 16)
            | ((uint32_t)header_p->size[1] << 8)
            | ((uint32_t)header_p->size[2] << 0));
}

void messi_header_create(struct messi_header_t *header_p,
                         uint8_t message_type,
                         uint32_t size);

int messi_epoll_ctl_default(int epoll_fd, int op, int fd, uint32_t events);

int messi_make_non_blocking(int fd);

/**
 * Parse tcp://<host>:<port>. Returns zero(0) if successful.
 */
int messi_parse_tcp_uri(const char *uri_p,
                        char *host_p,
                        size_t host_size,
                        int *port_p);

#endif

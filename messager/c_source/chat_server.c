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
 * This file is part of the Messager project.
 */

#include <sys/epoll.h>
#include "chat.h"

static struct chat_server_client_t *client_alloc()
{
}

static struct chat_server_client_t *client_free()
{
}

static void process_listener(struct chat_server_t *self_p, uint32_t events)
{
    int client_fd;
    struct chat_server_client_t *client_p;

    client_fd = accept(self_p->listener_fd);

    if (client_fd == -1) {
        return;
    }

    client_p = client_alloc(self_p);

    if (client_p == NULL) {
        return;
    }

    client_p->fd = client_fd;
}

static void handle_message_user(struct chat_server_t *self_p,
                                struct chat_server_client_t *client_p)
{
    int res;
    struct chat_client_to_server_t *message_p;

    self_p->input.message_p = chat_client_to_server_new(
        &client_p->input.workspace.buf_p[0],
        client_p->input.workspace.size);
    message_p = client_p->input.message_p;

    if (message_p == NULL) {
        return;
    }

    res = chat_client_to_server_decode(message_p,
                                       &client_p->encoded.buf_p[sizeof(header)],
                                       client_p->encoded.size - sizeof(header));

    if (res < 0) {
        return;
    }

    switch (message_p->messages.choice) {

    case chat_client_to_server_messages_choice_connect_req_e:
        self_p->on_connect_req(self_p,
                               client_p,
                               &message_p->messages.value.connect_req);
        break;

    case chat_client_to_server_messages_choice_message_ind_e:
        self_p->on_message_ind(self_p,
                               client_p,
                               &message_p->messages.value.message_ind);
        break;

    default:
        break;
    }
}

static void handle_message_ping(struct chat_server_t *self_p,
                                struct chat_server_client_t *client_p)
{
    (void)self_p;
    (void)client_p;
}

static void handle_message(struct chat_server_t *self_p,
                           struct chat_server_client_t *client_p,
                           struct header_t *header_p)
{
    switch (header_p->type) {

    case CHAT_COMMON_MESSAGE_TYPE_USER:
        handle_message_user(self_p, client_p, header_p);
        break;

    case CHAT_COMMON_MESSAGE_TYPE_PING:
        handle_message_ping(self_p, client_p, header_p);
        break;

    default:
        break;
    }
}

static void process_client(struct chat_server_t *self_p,
                           struct chat_server_client_t *client_p,
                           uint32_t events)
{
    ssize_t size;

    while (true) {
        size = read(client_p->fd,
                    &client_p->input.buf_p[client_p->input.size],
                    self_p->input.left);

        if ((size == -1) && (errno == EAGAIN)) {
            break;
        } else if (size <= 0) {
            close(client_p->fd);
            break;
        }

        client_p->input.size += size;
        self_p->input.left -= size;

        if (self_p->input.left > 0) {
            break;
        }

        switch (client_p->input.state) {

        case chat_server_client_input_state_header_t:
            self_p->input.left = header.size;
            break;

        case chat_server_client_input_state_payload_t:
            handle_message(self_p, client_p, header_p);
            break;

        default:
            break;
        }
    }
}

int chat_server_init(struct chat_server_t *self_p,
                     const char *address_p,
                     struct chat_server_client_t *clients_p,
                     int clients_max,
                     chat_on_connect_req_t on_connect_req,
                     chat_on_message_ind_t on_message_ind,
                     int epoll_fd,
                     chat_epoll_ctl_t epoll_ctl)
{
    if (on_connect_req == NULL) {
        on_connect_req = on_connect_req_default;
    }

    if (on_message_ind == NULL) {
        on_message_ind = on_message_ind_default;
    }

    if (epoll_ctl == NULL) {
        epoll_ctl = epoll_ctl_default;
    }

    self_p->address_p = address_p;
    self_p->clients_p = clients_p;
    self_p->clients_max = clients_max;
    self_p->on_connect_req = on_connect_req;
    self_p->on_message_ind = on_message_ind;
    self_p->epoll_fd = epoll_fd;
    self_p->epoll_ctl = epoll_ctl;
    
    return (0);
}

int chat_server_start(struct chat_server_t *self_p)
{
    int res;
    int listener_fd;
    struct sockaddr_in addr;

    listener_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (listener_fd == -1) {
        return (1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(self_p->port));
    inet_aton(self_p->host_p, (struct in_addr *)&addr.sin_addr.s_addr);

    res = bind(listener_fd, &addr, sizeof(addr));

    if (res == -1) {
        goto out;
    }

    res = listen(listener_fd, 5);

    if (res == -1) {
        goto out;
    }

    res = fcntl(listener_fd,
                F_SETFL,
                fcntl(listener_fd, F_GETFL, 0) | O_NONBLOCK);

    if (res == -1) {
        goto out;
    }

    res = self_p->epoll_ctl(self_p->epoll_fd,
                            EPOLL_CTL_ADD,
                            listener_fd,
                            EPOLLIN);

    if (res == -1) {
        goto out;
    }

    return (0);

 out:

    close(self_p->listener_fd);

    return (-1);
}

void chat_server_stop(struct chat_server_t *self_p)
{
    self_p->epoll_ctl(self_p->epoll_fd, EPOLL_CTL_DEL, self_p->listener_fd, 0);
    close(self_p->listener_fd);

    while (client_p != NULL) {
        self_p->epoll_ctl(self_p->epoll_fd, EPOLL_CTL_DEL, client_p->fd, 0);
        close(client_p->fd);
        client_p = client_p->next_p;
    }
}

bool chat_server_has_file_descriptor(struct chat_server_t *self_p, int fd)
{
    if (self_p->listener_fd == fd) {
        return (true);
    }

    while (client_p != NULL) {
        if (client_p->fd == fd) {
            return (true);
        }

        client_p = client_p->next_p;
    }

    return (false);
}

void chat_server_process(struct chat_server_t *self_p, int fd, uint32_t events)
{
    if (self_p->listener_fd == fd) {
        process_listener(self_p, events);
    } else {
        while (client_p != NULL) {
            if (client_p->fd == fd) {
                process_client(self_p, client_p, events);
                break;
            }

            client_p = client_p->next_p;
        }
    }
}

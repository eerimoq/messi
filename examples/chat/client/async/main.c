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

#include "async.h"
#include "client.h"

static void parse_args(int argc,
                       const char *argv[],
                       const char **user_pp)
{
    if (argc != 2) {
        printf("usage: %s <user>\n", argv[0]);
        exit(1);
    }

    *user_pp = argv[1];
}

static void *forward_stdin_to_client_main(struct async_t *async_p)
{
    char *data_p;
    ssize_t size;

    async_utils_linux_make_stdin_unbuffered();

    while (true) {
        data_p = malloc(sizeof(*data_p));

        if (data_p == NULL) {
            break;
        }

        size = read(STDIN_FILENO, data_p, sizeof(*data_p));

        if (size != sizeof(*data_p)) {
            break;
        }

        async_call_threadsafe(async_p, client_user_input, data_p);
    }

    return (NULL);
}

int main(int argc, const char *argv[])
{
    struct async_t async;
    struct client_t client;
    const char *user_p;

    parse_args(argc, argv, &user_p);

    async_init(&async);
    async_set_runtime(&async, async_runtime_create());
    client_init(&client, user_p, "tcp://127.0.0.1:6000", &async);
    pthread_create((void *(*)(void *))forward_stdin_to_client_main, &async);
    async_run_forever(&async);

    return (1);
}

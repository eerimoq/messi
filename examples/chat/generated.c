
int time_init(struct time_channel_t *self_p,
              time_on_message_t on_message;
              void *workspace_p,
              size_t size)
{
    self_p->on_message = on_message;
    self_p->message_p = time_message_new(&workspace[0], size);

    if (res != 0) {
        return (res);
    }
}

void time_channel_on_input(struct time_channel_t *self_p)
{
    int res;

    res = pbtools_receive(self_p,
                          &self_p->encoded.buf_p[0],
                          self_p->encoded.size);

    if (res < 0) {
        return;
    }

    res = time_message_decode(self_p->message_p,
                              &self_p->encoded.buf_p[0],
                              self_p->encoded.size);

    if (res < 0) {
        return;
    }

    self_p->on_message(self_p, &self_p->message_p->messages, self_p->arg_p);
}

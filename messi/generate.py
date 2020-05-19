def make_format(mo):
    value = mo.group(0)

    if value in '{}':
        return 2 * value
    else:
        return f'{{{value.lower()}}}'


def get_messages(message):
    if len(message.oneofs) != 1 or message.messages:
        raise Exception(
            f'The {message.name} message does not contain exactly '
            f'one oneof.')

    oneof = message.oneofs[0]

    if oneof.name != 'messages':
        raise Exception(
            f'The oneof in {message.name} must be called messages, not '
            f'{oneof.name}.')

    return oneof.fields

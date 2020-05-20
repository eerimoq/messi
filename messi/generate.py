import os
from pbtools.parser import parse_file
from pbtools.parser import camel_to_snake_case


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


class Generator:

    RE_TEMPLATE_TO_FORMAT = None

    def __init__(self, filename, import_path, output_directory):
        parsed = parse_file(filename, import_path)
        basename = os.path.basename(filename)
        self.name = camel_to_snake_case(os.path.splitext(basename)[0])
        self.output_directory = output_directory
        self.client_to_server_messages = []
        self.server_to_client_messages = []

        for message in parsed.messages:
            if message.name == 'ClientToServer':
                self.client_to_server_messages = get_messages(message)
            elif message.name == 'ServerToClient':
                self.server_to_client_messages = get_messages(message)

    def read_template_file(self, filename):
        with open(os.path.join(self.templates_dir, filename)) as fin:
            return self.RE_TEMPLATE_TO_FORMAT.sub(make_format, fin.read())

    def create_file(self, filename, data):
        with open(os.path.join(self.output_directory, filename), 'w') as fout:
            fout.write(data)

    def generate_files_no_check(self):
        raise NotImplementedError()
            
    def generate_files(self):
        if not self.client_to_server_messages:
            return

        if not self.server_to_client_messages:
            return

        self.generate_files_no_check()

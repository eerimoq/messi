import os
import shutil
from pbtools.parser import parse_file
from pbtools.parser import camel_to_snake_case
from ..parser import get_messages


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
TEMPLATES_DIR = os.path.join(SCRIPT_DIR, 'templates')


class Generator:

    def __init__(self, name, parsed, output_directory):
        self.name = name
        self.output_directory = output_directory
        self.client_to_server_messages = []
        self.server_to_client_messages = []

        for message in parsed.messages:
            if message.name == 'ClientToServer':
                self.client_to_server_messages = get_messages(message)
            elif message.name == 'ServerToClient':
                self.server_to_client_messages = get_messages(message)

    def generate_files(self):
        with open(os.path.join(TEMPLATES_DIR, 'client.py'), 'r') as fin:
            code = fin.read()

        client_py = os.path.join(self.output_directory, f'{self.name}_client.py')

        with open(client_py, 'w') as fout:
            fout.write(code)

        shutil.copy(os.path.join(SCRIPT_DIR, 'messi.py'), self.output_directory)


def generate_files(import_path, output_directory, infiles):
    """Generate Python source code from proto-file(s).

    """

    for filename in infiles:
        parsed = parse_file(filename, import_path)
        basename = os.path.basename(filename)
        name = camel_to_snake_case(os.path.splitext(basename)[0])

        Generator(name, parsed, output_directory).generate_files()

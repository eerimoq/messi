import re
import os
from grpc_tools import protoc
from .. import generate


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

ON_MESSAGE = '''\
    async def on_{message.name}(self, message):
        """Called when a {message.name} message is received from the server.

        """
'''

INIT_MESSAGE = '''\
    def init_{message.name}(self):
        """Prepare a {message.name} message. Call `send()` to send it.

        """

        self._output = {name}_pb2.ClientToServer()
        self._output.{message.name}.SetInParent()

        return self._output.{message.name}
'''

HANDLE_MESSAGE = '''\
        elif choice == '{message.name}':
            await self.on_{message.name}(message.{message.name})\
'''


class Generator(generate.Generator):

    RE_TEMPLATE_TO_FORMAT = re.compile(r'{'
                                       r'|}'
                                       r'|NAME_TITLE'
                                       r'|NAME'
                                       r'|ON_MESSAGES'
                                       r'|INIT_MESSAGES'
                                       r'|HANDLE_MESSAGES')

    def __init__(self, filename, side, import_path, output_directory):
        super().__init__(filename, side, import_path, output_directory)
        self.templates_dir = os.path.join(SCRIPT_DIR, 'templates')

    def generate_client(self):
        on_messages = []
        init_messages = []
        handle_messages = []

        for message in self.client_to_server_messages:
            init_messages.append(INIT_MESSAGE.format(name=self.name,
                                                     message=message))

        for message in self.server_to_client_messages:
            on_messages.append(ON_MESSAGE.format(message=message))
            handle_messages.append(HANDLE_MESSAGE.format(message=message))

        name_title = self.name.title().replace('_', '')
        handle_messages = 8 * ' ' + '\n'.join(handle_messages)[10:]
        client_py = self.read_template_file('client.py')

        return client_py.format(name=self.name,
                                name_title=name_title,
                                on_messages='\n'.join(on_messages),
                                init_messages='\n'.join(init_messages),
                                handle_messages=handle_messages)

    def generate_client_files(self):
        self.create_file(f'{self.name}_client.py', self.generate_client())


def generate_protobuf_files(import_path, output_directory, infiles):
    command = ['protoc', f'--python_out={output_directory}']

    for path in import_path:
        command += ['-I', path]

    command += infiles

    result = protoc.main(command)

    if result != 0:
        raise Exception(f'protoc failed with exit code {result}.')


def generate_files(side, import_path, output_directory, infiles):
    """Generate Python source code from given proto-file(s).

    """

    generate_protobuf_files(import_path, output_directory, infiles)

    for filename in infiles:
        generator = Generator(filename, side, import_path, output_directory)
        generator.generate_files()

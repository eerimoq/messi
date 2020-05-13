import os
import shutil
import pbtools.c_source
from pbtools.parser import parse_file
from pbtools.parser import camel_to_snake_case


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
TEMPLATES_DIR = os.path.join(SCRIPT_DIR, 'templates')

CLIENT_H_ON_MESSAGE_TYPEDEF = '''\
typedef void (*{name}_client_on_{message}_t)(
    struct {name}_client_t *self_p,
    struct {name}_{message}_t *message_p);
'''

CLIENT_H_ON_MESSAGE_MEMBER = '''\
    {name}_client_on_{message}_t on_{message};\
'''

CLIENT_H_ON_MESSAGE_PARAM = '''\
    {name}_client_on_{message}_t on_{message},\
'''

CLIENT_H_INIT_MESSAGE = '''\
struct {name}_{message}_t *
{name}_client_init_{message}(struct {name}_client_t *self_p);
'''

SERVER_H_ON_MESSAGE_TYPEDEF = '''\
typedef void (*{name}_server_on_{message}_t)(
    struct {name}_server_t *self_p,
    struct {name}_server_client_t *client_p,
    struct {name}_{message}_t *message_p);
'''

SERVER_H_ON_MESSAGE_MEMBER = '''\
    {name}_server_on_{message}_t on_{message};\
'''

SERVER_H_ON_MESSAGE_PARAM = '''\
    {name}_server_on_{message}_t on_{message},\
'''

SERVER_H_INIT_MESSAGE = '''\
struct {name}_{message}_t *
{name}_server_init_{message}(struct {name}_server_t *self_p);
'''


def read_template_file(filename):
    with open(os.path.join(TEMPLATES_DIR, filename)) as fin:
        return fin.read()


class Generator:

    def __init__(self, name, parsed, output_directory):
        self.name = name
        self.output_directory = output_directory
        self.client_to_server_messages = ['connect_req', 'message_ind']
        self.server_to_client_messages = ['connect_rsp', 'message_ind']

    def generate_client_h(self):
        on_message_typedefs = []
        on_message_members = []
        on_message_params = []
        init_messages = []

        for message in self.client_to_server_messages:
            init_messages.append(
                CLIENT_H_INIT_MESSAGE.format(name=self.name,
                                             message=message))

        for message in self.server_to_client_messages:
            on_message_typedefs.append(
                CLIENT_H_ON_MESSAGE_TYPEDEF.format(name=self.name,
                                                   message=message))
            on_message_members.append(
                CLIENT_H_ON_MESSAGE_MEMBER.format(name=self.name,
                                                  message=message))
            on_message_params.append(
                CLIENT_H_ON_MESSAGE_PARAM.format(name=self.name,
                                                 message=message))

        client_h = read_template_file('client.h')

        return client_h.format(name=self.name,
                               name_upper=self.name.upper(),
                               on_message_typedefs='\n'.join(on_message_typedefs),
                               on_message_members='\n'.join(on_message_members),
                               on_message_params='\n'.join(on_message_params),
                               init_messages='\n'.join(init_messages))

    def generate_client_c(self, header_name):
        client_c = read_template_file('client.c')

        return client_c.format(name=self.name,
                               name_upper=self.name.upper())

    def generate_client(self, header_name):
        client_h = self.generate_client_h()
        client_c = self.generate_client_c(header_name)

        return client_h, client_c

    def generate_server_h(self):
        on_message_typedefs = []
        on_message_members = []
        on_message_params = []
        init_messages = []

        for message in self.server_to_client_messages:
            init_messages.append(
                SERVER_H_INIT_MESSAGE.format(name=self.name,
                                             message=message))

        for message in self.client_to_server_messages:
            on_message_typedefs.append(
                SERVER_H_ON_MESSAGE_TYPEDEF.format(name=self.name,
                                                   message=message))
            on_message_members.append(
                SERVER_H_ON_MESSAGE_MEMBER.format(name=self.name,
                                                  message=message))
            on_message_params.append(
                SERVER_H_ON_MESSAGE_PARAM.format(name=self.name,
                                                 message=message))

        server_h = read_template_file('server.h')

        return server_h.format(name=self.name,
                               name_upper=self.name.upper(),
                               on_message_typedefs='\n'.join(on_message_typedefs),
                               on_message_members='\n'.join(on_message_members),
                               on_message_params='\n'.join(on_message_params),
                               init_messages='\n'.join(init_messages))

    def generate_server_c(self, header_name):
        server_c = read_template_file('server.c')

        return server_c.format(name=self.name,
                               name_upper=self.name.upper())

    def generate_server(self, header_name):
        server_h = self.generate_server_h()
        server_c = self.generate_server_c(header_name)

        return server_h, server_c

    def generate_client_files(self):
        client_h = f'{self.name}_client.h'
        client_c = f'{self.name}_client.c'

        header, source = self.generate_client(client_h)

        client_h = os.path.join(self.output_directory, client_h)
        client_c = os.path.join(self.output_directory, client_c)

        with open(client_h, 'w') as fout:
            fout.write(header)

        with open(client_c, 'w') as fout:
            fout.write(source)

    def generate_server_files(self):
        server_h = f'{self.name}_server.h'
        server_c = f'{self.name}_server.c'

        header, source = self.generate_server(server_h)

        server_h = os.path.join(self.output_directory, server_h)
        server_c = os.path.join(self.output_directory, server_c)

        with open(server_h, 'w') as fout:
            fout.write(header)

        with open(server_c, 'w') as fout:
            fout.write(source)


def generate_files(import_path, output_directory, infiles):
    """Generate C source code from proto-file(s).

    """

    pbtools.c_source.generate_files(import_path, output_directory, infiles)

    for filename in infiles:
        parsed = parse_file(filename, import_path)
        basename = os.path.basename(filename)
        name = camel_to_snake_case(os.path.splitext(basename)[0])

        generator = Generator(name, parsed, output_directory)

        generator.generate_client_files()
        generator.generate_server_files()

    for filename in ['common.h', 'common.c']:
        shutil.copyfile(os.path.join(SCRIPT_DIR, filename),
                        os.path.join(output_directory, f'{name}_{filename}'))

import re
import os
import shutil
import pbtools.c_source
from pbtools.parser import parse_file
from pbtools.parser import camel_to_snake_case


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
TEMPLATES_DIR = os.path.join(SCRIPT_DIR, 'templates')

CLIENT_H_ON_MESSAGE_TYPEDEF = '''\
typedef void (*{name}_client_on_{message.name}_t)(
    struct {name}_client_t *self_p,
    struct {message.full_type_snake_case}_t *message_p);
'''

CLIENT_H_ON_MESSAGE_MEMBER = '''\
    {name}_client_on_{message.name}_t on_{message.name};\
'''

CLIENT_H_ON_MESSAGE_PARAM = '''\
    {name}_client_on_{message.name}_t on_{message.name},\
'''

CLIENT_H_INIT_MESSAGE = '''\
struct {message.full_type_snake_case}_t *
{name}_client_init_{message.name}(struct {name}_client_t *self_p);
'''

CLIENT_C_HANDLE_CASE = '''\
    case {name}_server_to_client_messages_choice_{message.name}_e:
        self_p->on_{message.name}(
            self_p,
            &message_p->messages.value.{message.name});
        break;
'''

CLIENT_C_ON_DEFAULT = '''\
static void on_{message.name}_default(
    struct {name}_client_t *self_p,
    struct {message.full_type_snake_case}_t *message_p)
{{
    (void)self_p;
    (void)message_p;
}}
'''

CLIENT_C_ON_MESSAGE_PARAM = '''\
    {name}_client_on_{message.name}_t on_{message.name},\
'''

CLIENT_C_ON_PARAM_DEFAULT = '''\
    if (on_{message.name} == NULL) {{
        on_{message.name} = on_{message.name}_default;
    }}
'''

CLIENT_C_ON_PARAM_ASSIGN = '''\
    self_p->on_{message.name} = on_{message.name};\
'''

CLIENT_C_INIT_MESSAGE = '''\
struct {message.full_type_snake_case}_t *
{name}_client_init_{message.name}(struct {name}_client_t *self_p)
{{
    self_p->output.message_p = {name}_client_to_server_new(
        &self_p->output.workspace.buf_p[0],
        self_p->output.workspace.size);
    {name}_client_to_server_messages_{message.name}_init(self_p->output.message_p);

    return (&self_p->output.message_p->messages.value.{message.name});
}}
'''

SERVER_H_ON_MESSAGE_TYPEDEF = '''\
typedef void (*{name}_server_on_{message.name}_t)(
    struct {name}_server_t *self_p,
    struct {name}_server_client_t *client_p,
    struct {message.full_type_snake_case}_t *message_p);
'''

SERVER_H_ON_MESSAGE_MEMBER = '''\
    {name}_server_on_{message.name}_t on_{message.name};\
'''

SERVER_H_ON_MESSAGE_PARAM = '''\
    {name}_server_on_{message.name}_t on_{message.name},\
'''

SERVER_H_INIT_MESSAGE = '''\
struct {message.full_type_snake_case}_t *
{name}_server_init_{message.name}(struct {name}_server_t *self_p);
'''

SERVER_C_HANDLE_CASE = '''\
    case {name}_client_to_server_messages_choice_{message.name}_e:
        self_p->on_{message.name}(
            self_p,
            client_p,
            &message_p->messages.value.{message.name});
        break;
'''

SERVER_C_ON_DEFAULT = '''\
static void on_{message.name}_default(
    struct {name}_server_t *self_p,
    struct {name}_server_client_t *client_p,
    struct {message.full_type_snake_case}_t *message_p)
{{
    (void)self_p;
    (void)client_p;
    (void)message_p;
}}
'''

SERVER_C_ON_MESSAGE_PARAM = '''\
    {name}_server_on_{message.name}_t on_{message.name},\
'''

SERVER_C_ON_PARAM_DEFAULT = '''\
    if (on_{message.name} == NULL) {{
        on_{message.name} = on_{message.name}_default;
    }}
'''

SERVER_C_ON_PARAM_ASSIGN = '''\
    self_p->on_{message.name} = on_{message.name};\
'''

SERVER_C_INIT_MESSAGE = '''\
struct {message.full_type_snake_case}_t *
{name}_server_init_{message.name}(struct {name}_server_t *self_p)
{{
    self_p->output.message_p = {name}_server_to_client_new(
        &self_p->output.workspace.buf_p[0],
        self_p->output.workspace.size);
    {name}_server_to_client_messages_{message.name}_init(self_p->output.message_p);

    return (&self_p->output.message_p->messages.value.{message.name});
}}
'''

RE_TEMPLATE_TO_FORMAT = re.compile(r'{'
                                   r'|}'
                                   r'|NAME_UPPER'
                                   r'|NAME'
                                   r'|ON_MESSAGE_TYPEDEFS'
                                   r'|ON_MESSAGE_MEMBERS'
                                   r'|ON_MESSAGE_PARAMS'
                                   r'|INIT_MESSAGES'
                                   r'|HANDLE_CASES'
                                   r'|ON_DEFAULTS'
                                   r'|ON_PARAMS_DEFAULT'
                                   r'|ON_PARAMS_ASSIGN')


def make_format(mo):
    value = mo.group(0)

    if value in '{}':
        return 2 * value
    else:
        return f'{{{value.lower()}}}'


def read_template_file(filename):
    with open(os.path.join(TEMPLATES_DIR, filename)) as fin:
        return RE_TEMPLATE_TO_FORMAT.sub(make_format, fin.read())


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
        handle_cases = []
        on_defaults = []
        on_message_params = []
        on_params_default = []
        on_params_assign = []
        init_messages = []

        for message in self.client_to_server_messages:
            init_messages.append(
                CLIENT_C_INIT_MESSAGE.format(name=self.name,
                                             message=message))

        for message in self.server_to_client_messages:
            handle_cases.append(
                CLIENT_C_HANDLE_CASE.format(name=self.name,
                                            message=message))
            on_defaults.append(
                CLIENT_C_ON_DEFAULT.format(name=self.name,
                                           message=message))
            on_message_params.append(
                CLIENT_C_ON_MESSAGE_PARAM.format(name=self.name,
                                                 message=message))
            on_params_default.append(
                CLIENT_C_ON_PARAM_DEFAULT.format(name=self.name,
                                                 message=message))
            on_params_assign.append(
                CLIENT_C_ON_PARAM_ASSIGN.format(name=self.name,
                                                message=message))

        client_c = read_template_file('client.c')

        return client_c.format(name=self.name,
                               name_upper=self.name.upper(),
                               handle_cases='\n'.join(handle_cases),
                               on_defaults=''.join(on_defaults),
                               on_message_params='\n'.join(on_message_params),
                               on_params_default='\n'.join(on_params_default),
                               on_params_assign='\n'.join(on_params_assign),
                               init_messages='\n'.join(init_messages))

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
        handle_cases = []
        on_defaults = []
        on_message_params = []
        on_params_default = []
        on_params_assign = []
        init_messages = []

        for message in self.client_to_server_messages:
            handle_cases.append(
                SERVER_C_HANDLE_CASE.format(name=self.name,
                                            message=message))
            on_defaults.append(
                SERVER_C_ON_DEFAULT.format(name=self.name,
                                           message=message))
            on_message_params.append(
                SERVER_C_ON_MESSAGE_PARAM.format(name=self.name,
                                                 message=message))
            on_params_default.append(
                SERVER_C_ON_PARAM_DEFAULT.format(name=self.name,
                                                 message=message))
            on_params_assign.append(
                SERVER_C_ON_PARAM_ASSIGN.format(name=self.name,
                                                message=message))

        for message in self.server_to_client_messages:
            init_messages.append(
                SERVER_C_INIT_MESSAGE.format(name=self.name,
                                             message=message))

        server_c = read_template_file('server.c')

        return server_c.format(name=self.name,
                               name_upper=self.name.upper(),
                               handle_cases='\n'.join(handle_cases),
                               on_defaults=''.join(on_defaults),
                               on_message_params='\n'.join(on_message_params),
                               on_params_default='\n'.join(on_params_default),
                               on_params_assign='\n'.join(on_params_assign),
                               init_messages='\n'.join(init_messages))

    def generate_server(self, header_name):
        server_h = self.generate_server_h()
        server_c = self.generate_server_c(header_name)

        return server_h, server_c

    def generate_common_h(self):
        common_h = read_template_file('common.h')

        return common_h.format(name=self.name,
                               name_upper=self.name.upper())

    def generate_common_c(self, header_name):
        common_c = read_template_file('common.c')

        return common_c.format(name=self.name,
                               name_upper=self.name.upper())

    def generate_common(self, header_name):
        common_h = self.generate_common_h()
        common_c = self.generate_common_c(header_name)

        return common_h, common_c

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

    def generate_common_files(self):
        common_h = f'{self.name}_common.h'
        common_c = f'{self.name}_common.c'

        header, source = self.generate_common(common_h)

        common_h = os.path.join(self.output_directory, common_h)
        common_c = os.path.join(self.output_directory, common_c)

        with open(common_h, 'w') as fout:
            fout.write(header)

        with open(common_c, 'w') as fout:
            fout.write(source)

    def generate_files(self):
        if not self.client_to_server_messages:
            return

        if not self.server_to_client_messages:
            return

        self.generate_client_files()
        self.generate_server_files()
        self.generate_common_files()


def generate_files(import_path, output_directory, infiles):
    """Generate C source code from proto-file(s).

    """

    pbtools.c_source.generate_files(import_path, output_directory, infiles)

    for filename in infiles:
        parsed = parse_file(filename, import_path)
        basename = os.path.basename(filename)
        name = camel_to_snake_case(os.path.splitext(basename)[0])

        Generator(name, parsed, output_directory).generate_files()

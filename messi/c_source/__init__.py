import re
import os
import pbtools.c_source
from .. import generate


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

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
/**
 * Prepare a {message.name} message. Call `send()` to send it.
 */
struct {message.full_type_snake_case}_t *{name}_client_init_{message.name}(
    struct {name}_client_t *self_p);
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
struct {message.full_type_snake_case}_t *{name}_client_init_{message.name}(
    struct {name}_client_t *self_p)
{{
    {name}_client_new_output_message(self_p);
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
/**
 * Prepare a {message.name} message. Call `send()`, `reply()` or `broadcast()`
 * to send it.
 */
struct {message.full_type_snake_case}_t *{name}_server_init_{message.name}(
    struct {name}_server_t *self_p);
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
struct {message.full_type_snake_case}_t *{name}_server_init_{message.name}(
    struct {name}_server_t *self_p)
{{
    {name}_server_new_output_message(self_p);
    {name}_server_to_client_messages_{message.name}_init(self_p->output.message_p);

    return (&self_p->output.message_p->messages.value.{message.name});
}}
'''


class Generator(generate.Generator):

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

    def __init__(self, filename, side, import_path, output_directory, platform):
        super().__init__(filename, side, import_path, output_directory)
        self.templates_dir = os.path.join(SCRIPT_DIR, 'templates', platform)

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

        client_h = self.read_template_file('client.h')

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

        client_c = self.read_template_file('client.c')

        return client_c.format(name=self.name,
                               name_upper=self.name.upper(),
                               handle_cases='\n'.join(handle_cases),
                               on_defaults='\n'.join(on_defaults),
                               on_message_params='\n'.join(on_message_params),
                               on_params_default='\n'.join(on_params_default),
                               on_params_assign='\n'.join(on_params_assign),
                               init_messages='\n'.join(init_messages))

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

        server_h = self.read_template_file('server.h')

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

        server_c = self.read_template_file('server.c')

        return server_c.format(name=self.name,
                               name_upper=self.name.upper(),
                               handle_cases='\n'.join(handle_cases),
                               on_defaults='\n'.join(on_defaults),
                               on_message_params='\n'.join(on_message_params),
                               on_params_default='\n'.join(on_params_default),
                               on_params_assign='\n'.join(on_params_assign),
                               init_messages='\n'.join(init_messages))

    def generate_client_files(self):
        client_h = f'{self.name}_client.h'
        self.create_file(client_h, self.generate_client_h())
        self.create_file(f'{self.name}_client.c', self.generate_client_c(client_h))

    def generate_server_files(self):
        server_h = f'{self.name}_server.h'
        self.create_file(server_h, self.generate_server_h())
        self.create_file(f'{self.name}_server.c', self.generate_server_c(server_h))


def generate_files(platform, side, import_path, output_directory, infiles):
    """Generate C source code from given proto-file(s).

    """

    pbtools.c_source.generate_files(import_path, output_directory, infiles)

    for filename in infiles:
        generator = Generator(filename,
                              side,
                              import_path,
                              output_directory,
                              platform)
        generator.generate_files()

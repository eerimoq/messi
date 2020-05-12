import os
import shutil
import pbtools.c_source


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))


def generate(import_path, output_directory, infiles):
    pbtools.c_source.generate_files(import_path, output_directory, infiles)

    filenames = [
        'chat_client.c',
        'chat_client.h',
        'chat_common.c',
        'chat_common.h',
        'chat_server.c',
        'chat_server.h'
        ]

    for filename in filenames:
        shutil.copy(os.path.join(SCRIPT_DIR, filename), output_directory)

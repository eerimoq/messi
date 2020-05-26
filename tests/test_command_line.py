import os
import unittest
from unittest.mock import patch
import shutil

import messi


def read_file(filename):
    with open(filename, 'r') as fin:
        return fin.read()


def remove_directory(name):
    if os.path.exists(name):
        shutil.rmtree(name)


class CommandLineTest(unittest.TestCase):

    maxDiff = None

    def assert_files_equal(self, actual, expected):
        # open(expected, 'w').write(open(actual, 'r').read())
        self.assertEqual(read_file(actual), read_file(expected))

    def assert_file_exists(self, path):
        self.assertTrue(os.path.exists(path), path)

    def assert_file_missing(self, path):
        self.assertFalse(os.path.exists(path), path)

    def assert_generated_c_files(self,
                                 protocol,
                                 platform,
                                 client_side=True,
                                 server_side=True):
        if client_side:
            self.assert_files_equal(
                f'generated/{protocol}_client.h',
                f'tests/files/{protocol}/{platform}/{protocol}_client.h')
            self.assert_files_equal(
                f'generated/{protocol}_client.c',
                f'tests/files/{protocol}/{platform}/{protocol}_client.c')
        else:
            self.assert_file_missing(f'generated/{protocol}_client.h')
            self.assert_file_missing(f'generated/{protocol}_client.c')

        if server_side:
            self.assert_files_equal(
                f'generated/{protocol}_server.h',
                f'tests/files/{protocol}/{platform}/{protocol}_server.h')
            self.assert_files_equal(
                f'generated/{protocol}_server.c',
                f'tests/files/{protocol}/{platform}/{protocol}_server.c')
        else:
            self.assert_file_missing(f'generated/{protocol}_server.h')
            self.assert_file_missing(f'generated/{protocol}_server.c')

        self.assert_file_exists(f'generated/{protocol}.h')
        self.assert_file_exists(f'generated/{protocol}.c')

    def assert_generated_py_files(self, protocol, client_side=True):
        if client_side:
            self.assert_files_equal(
                f'generated/{protocol}_client.py',
                f'tests/files/{protocol}/{protocol}_client.py')
        else:
            self.assert_file_missing(f'generated/{protocol}_client.py')

        self.assert_file_exists(
            f'generated/tests/files/{protocol}/{protocol}_pb2.py')

    def test_generate_c_source_linux(self):
        protocols = [
            'chat',
            'my_protocol'
        ]

        for protocol in protocols:
            argv = [
                'messi',
                'generate_c_source',
                '-o', 'generated',
                '-p', 'linux',
                f'tests/files/{protocol}/{protocol}.proto'
            ]

            remove_directory('generated')
            os.mkdir('generated')

            with patch('sys.argv', argv):
                messi.main()

            self.assert_generated_c_files(protocol, 'linux')

    def test_generate_c_source_async(self):
        protocols = [
            'chat',
            'my_protocol'
        ]

        for protocol in protocols:
            argv = [
                'messi',
                'generate_c_source',
                '-o', 'generated',
                '-p', 'async',
                f'tests/files/{protocol}/{protocol}.proto'
            ]

            remove_directory('generated')
            os.mkdir('generated')

            with patch('sys.argv', argv):
                messi.main()

            self.assert_generated_c_files(protocol, 'async')

    def test_generate_c_source_side(self):
        datas = [
            ('both', True, True),
            ('client', True, False),
            ('server', False, True)
        ]

        for side, client_side, server_side in datas:
            argv = [
                'messi',
                'generate_c_source',
                '-o', 'generated',
                '-s', side,
                f'tests/files/chat/chat.proto'
            ]

            remove_directory('generated')
            os.mkdir('generated')

            with patch('sys.argv', argv):
                messi.main()

            self.assert_generated_c_files('chat', 'linux', client_side, server_side)

    def test_generate_c_source_linux_imported(self):
        argv = [
            'messi',
            'generate_c_source',
            '-o', 'generated',
            '-p', 'linux',
            '-I', 'tests/files/imported',
            'tests/files/imported/types_not_package_name.proto',
            'tests/files/imported/imported.proto'
        ]

        remove_directory('generated')
        os.mkdir('generated')

        with patch('sys.argv', argv):
            messi.main()

        self.assert_generated_c_files('imported', 'linux')
        self.assert_file_missing('generated/types_not_package_name_server.h')
        self.assert_file_missing('generated/types_not_package_name_server.c')
        self.assert_file_missing('generated/types_not_package_name_client.h')
        self.assert_file_missing('generated/types_not_package_name_client.c')
        self.assert_file_exists('generated/types_not_package_name.h')
        self.assert_file_exists('generated/types_not_package_name.c')

    def test_generate_py_source(self):
        protocols = [
            'chat',
            'my_protocol'
        ]

        for protocol in protocols:
            argv = [
                'messi',
                'generate_py_source',
                '-o', 'generated',
                f'tests/files/{protocol}/{protocol}.proto'
            ]

            remove_directory('generated')
            os.mkdir('generated')

            with patch('sys.argv', argv):
                messi.main()

            self.assert_generated_py_files(protocol)

    def test_generate_py_source_side(self):
        datas = [
            ('both', True),
            ('client', True),
            ('server', False)
        ]

        for side, client_side in datas:
            argv = [
                'messi',
                'generate_py_source',
                '-o', 'generated',
                '-s', side,
                f'tests/files/chat/chat.proto'
            ]

            remove_directory('generated')
            os.mkdir('generated')

            with patch('sys.argv', argv):
                messi.main()

            self.assert_generated_py_files('chat', client_side)

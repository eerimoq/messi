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

    def assert_generated_c_files(self, protocol, platform):
        self.assert_files_equal(
            f'generated/{protocol}_server.h',
            f'tests/files/{protocol}/{platform}/{protocol}_server.h')
        self.assert_files_equal(
            f'generated/{protocol}_server.c',
            f'tests/files/{protocol}/{platform}/{protocol}_server.c')
        self.assert_files_equal(
            f'generated/{protocol}_client.h',
            f'tests/files/{protocol}/{platform}/{protocol}_client.h')
        self.assert_files_equal(
            f'generated/{protocol}_client.c',
            f'tests/files/{protocol}/{platform}/{protocol}_client.c')
        self.assert_file_exists(f'generated/{protocol}.h')
        self.assert_file_exists(f'generated/{protocol}.c')
        self.assert_file_exists('generated/pbtools.h')
        self.assert_file_exists('generated/pbtools.c')
        self.assert_file_exists('generated/messi.h')
        self.assert_file_exists('generated/messi.c')

    def assert_generated_py_files(self, protocol):
        self.assert_files_equal(
            f'generated/{protocol}_client.py',
            f'tests/files/{protocol}/{protocol}_client.py')
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

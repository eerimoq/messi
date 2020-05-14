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
        self.assertTrue(os.path.exists(path))

    def assert_file_missing(self, path):
        self.assertFalse(os.path.exists(path))

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
    
            self.assert_files_equal(
                f'generated/{protocol}_common.h',
                f'tests/files/{protocol}/linux/{protocol}_common.h')
            self.assert_files_equal(
                f'generated/{protocol}_common.c',
                f'tests/files/{protocol}/linux/{protocol}_common.c')
            self.assert_files_equal(
                f'generated/{protocol}_server.h',
                f'tests/files/{protocol}/linux/{protocol}_server.h')
            self.assert_files_equal(
                f'generated/{protocol}_server.c',
                f'tests/files/{protocol}/linux/{protocol}_server.c')
            self.assert_files_equal(
                f'generated/{protocol}_client.h',
                f'tests/files/{protocol}/linux/{protocol}_client.h')
            self.assert_files_equal(
                f'generated/{protocol}_client.c',
                f'tests/files/{protocol}/linux/{protocol}_client.c')
            self.assert_file_exists(f'generated/{protocol}.h')
            self.assert_file_exists(f'generated/{protocol}.c')
            self.assert_file_exists('generated/pbtools.h')
            self.assert_file_exists('generated/pbtools.c')

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

        self.assert_files_equal('generated/imported_common.h',
                                'tests/files/imported/linux/imported_common.h')
        self.assert_files_equal('generated/imported_common.c',
                                'tests/files/imported/linux/imported_common.c')
        self.assert_files_equal('generated/imported_server.h',
                                'tests/files/imported/linux/imported_server.h')
        self.assert_files_equal('generated/imported_server.c',
                                'tests/files/imported/linux/imported_server.c')
        self.assert_files_equal('generated/imported_client.h',
                                'tests/files/imported/linux/imported_client.h')
        self.assert_files_equal('generated/imported_client.c',
                                'tests/files/imported/linux/imported_client.c')
        self.assert_file_missing('generated/types_not_package_name_common.h')
        self.assert_file_missing('generated/types_not_package_name_common.c')
        self.assert_file_missing('generated/types_not_package_name_server.h')
        self.assert_file_missing('generated/types_not_package_name_server.c')
        self.assert_file_missing('generated/types_not_package_name_client.h')
        self.assert_file_missing('generated/types_not_package_name_client.c')
        self.assert_file_exists('generated/imported.h')
        self.assert_file_exists('generated/imported.c')
        self.assert_file_exists('generated/types_not_package_name.h')
        self.assert_file_exists('generated/types_not_package_name.c')
        self.assert_file_exists('generated/pbtools.h')
        self.assert_file_exists('generated/pbtools.c')

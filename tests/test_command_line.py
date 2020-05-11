import os
import unittest
from unittest.mock import patch
import shutil

import messager


def read_file(filename):
    with open(filename, 'r') as fin:
        return fin.read()


def remove_directory(name):
    if os.path.exists(name):
        shutil.rmtree(name)


class CommandLineTest(unittest.TestCase):

    maxDiff = None

    def assert_files_equal(self, actual, expected):
        # open(actual, 'w').write(open(expected, 'r').read())
        self.assertEqual(read_file(actual), read_file(expected))

    def test_generate_c_source_linux(self):
        argv = [
            'messager',
            'generate_c_source',
            '-o', 'generated',
            '-p', 'linux',
            'tests/files/chat/chat.proto'
        ]

        remove_directory('generated')
        os.mkdir('generated')

        with patch('sys.argv', argv):
            messager.main()

        self.assertTrue(os.path.exists('generated/chat.h'))
        self.assertTrue(os.path.exists('generated/chat.c'))
        self.assertTrue(os.path.exists('generated/pbtools.h'))
        self.assertTrue(os.path.exists('generated/pbtools.c'))

        with self.assertRaises(FileNotFoundError):
            self.assert_files_equal('generated/chat_common.h',
                                    'tests/files/chat/chat_common.h')
            self.assert_files_equal('generated/chat_server.h',
                                    'tests/files/chat/chat_server.h')
            self.assert_files_equal('generated/chat_server.c',
                                    'tests/files/chat/chat_server.c')
            self.assert_files_equal('generated/chat_client.h',
                                    'tests/files/chat/chat_client.h')
            self.assert_files_equal('generated/chat_client.c',
                                    'tests/files/chat/chat_client.c')

import sys
import argparse

from .version import __version__
from . import c_source
from . import py_source


def do_generate_c_source(args):
    c_source.generate_files(args.platform,
                            args.side,
                            args.import_path,
                            args.output_directory,
                            args.infiles)


def do_generate_py_source(args):
    py_source.generate_files(args.side,
                             args.import_path,
                             args.output_directory,
                             args.infiles)


def main():
    parser = argparse.ArgumentParser(description='Messi command line utility')
    parser.add_argument('-d', '--debug', action='store_true')
    parser.add_argument('--version',
                        action='version',
                        version=__version__,
                        help='Print version information and exit.')

    # Python 3 workaround for help output if subcommand is missing.
    subparsers = parser.add_subparsers(dest='one of the above')
    subparsers.required = True

    # C generate subparser.
    subparser = subparsers.add_parser('generate_c_source',
                                      help='Generate C source code.')
    subparser.add_argument(
        '-p', '--platform',
        choices=('linux', 'async'),
        default='linux',
        help='Platform to generate code for (default: %(default)s).')
    subparser.add_argument(
        '-s', '--side',
        choices=('both', 'client', 'server'),
        default='both',
        help='Side to generate code for (default: %(default)s).')
    subparser.add_argument('-I', '--import-path',
                           action='append',
                           default=[],
                           help='Path(s) where to search for imports.')
    subparser.add_argument('-o', '--output-directory',
                           default='.',
                           help='Output directory (default: %(default)s).')
    subparser.add_argument('infiles',
                           nargs='+',
                           help='Input protobuf file(s).')
    subparser.set_defaults(func=do_generate_c_source)

    # Python generate subparser.
    subparser = subparsers.add_parser('generate_py_source',
                                      help='Generate Python source code.')
    subparser.add_argument(
        '-s', '--side',
        choices=('both', 'client', 'server'),
        default='both',
        help='Side to generate code for (default: %(default)s).')
    subparser.add_argument('-I', '--import-path',
                           action='append',
                           default=[],
                           help='Path(s) where to search for imports.')
    subparser.add_argument('-o', '--output-directory',
                           default='.',
                           help='Output directory (default: %(default)s).')
    subparser.add_argument('infiles',
                           nargs='+',
                           help='Input protobuf file(s).')
    subparser.set_defaults(func=do_generate_py_source)

    args = parser.parse_args()

    if args.debug:
        args.func(args)
    else:
        try:
            args.func(args)
        except BaseException as e:
            sys.exit(f'error: {e}')

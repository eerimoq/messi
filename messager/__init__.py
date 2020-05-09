import argparse

from .version import __version__


def do_generate_c_source(args):
    pass


def main():
    parser = argparse.ArgumentParser(description='Messager command line utility')
    parser.add_argument('-d', '--debug', action='store_true')
    parser.add_argument('--version',
                        action='version',
                        version=__version__,
                        help='Print version information and exit.')

    # Python 3 workaround for help output if subcommand is missing.
    subparsers = parser.add_subparsers(dest='one of the above')
    subparsers.required = True

    subparser = subparsers.add_parser('generate_c_source',
                                      help='Generate C source code.')
    subparser.set_defaults(func=do_generate_c_source)

    args = parser.parse_args()

    if args.debug:
        args.func(args)
    else:
        try:
            args.func(args)
        except BaseException as e:
            sys.exit(str(e))

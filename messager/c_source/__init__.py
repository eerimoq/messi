import pbtools.c_source


def generate(import_path, output_directory, infiles):
    pbtools.c_source.generate_files(import_path, output_directory, infiles)

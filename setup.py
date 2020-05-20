#!/usr/bin/env python

from setuptools import setup
from setuptools import find_packages
import re


def find_version():
    return re.search(r"^__version__ = '(.*)'$",
                     open('messi/version.py', 'r').read(),
                     re.MULTILINE).group(1)


setup(name='messi',
      version=find_version(),
      description='Reliable message passing in distributed systems.',
      long_description=open('README.rst', 'r').read(),
      author='Erik Moqvist',
      author_email='erik.moqvist@gmail.com',
      license='MIT',
      classifiers=[
          'License :: OSI Approved :: MIT License',
          'Programming Language :: Python :: 3',
      ],
      keywords=[],
      url='https://github.com/eerimoq/messi',
      packages=find_packages(exclude=['tests']),
      install_requires=[
          'pbtools',
          'bitstruct',
          'grpcio-tools'
      ],
      test_suite="tests",
      include_package_data=True,
      entry_points = {
          'console_scripts': ['messi=messi.__init__:main']
      })

# Copyright (C) 2014 Srivats P.
# 
# This file is part of "Ostinato"
# 
# This is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>

import json
import os
import re
import shutil
import sys
from setuptools import setup

def read(fname):
    return open(fname).read()

def get_pkg_info():
    info = {}
    t = open('../server/version.cpp').read()
    info['version'] = re.search('version = "([^"]*)"', t).group(1)
    info['revision'] = re.search('revision = "([^"]*)"', t).group(1)
    return info

# ------- script starts from here ------- #

if len(sys.argv) >= 2 and sys.argv[1] == 'clean_sdist':
    shutil.rmtree('dist', ignore_errors = True)
    shutil.rmtree('ostinato.egg-info', ignore_errors = True)
    if os.path.exists('pkg_info.json'):
        os.remove('pkg_info.json')
    sys.exit(0)

if len(sys.argv) >= 2 and sys.argv[1] == 'sdist':
    if os.path.split(os.getcwd())[1] != 'binding':
        print 'This script needs to be run from the binding directory'
        print 'Current Working Directory is %s' % os.getcwd()
        sys.exit(1)

    pkg_info = get_pkg_info()
    with open('pkg_info.json', 'wt') as f:
        json.dump(pkg_info, f, indent=4)
else:
    with open('pkg_info.json') as f:
        pkg_info = json.load(f)

setup(name = 'ostinato',
      version = pkg_info['version'],
      author = 'Srivats P',
      author_email = 'pstavirs@gmail.com',
      license = "GPLv3+",
      url = 'http://ostinato.org',
      description = 'Ostinato is a network packet and traffic generator and analyzer. It aims to be "Wireshark in Reverse" and become complementary to Wireshark. It features custom packet crafting via a GUI or a script',
      long_description = read('README.txt'),
      install_requires = ['protobuf>=2.3.0'],
      packages = ['ostinato', 'ostinato.protocols'],
      package_dir = {'ostinato': '.'},
      package_data = {'ostinato': ['pkg_info.json']},
      platforms = ['Any'],
      classifiers = [
          'Development Status :: 5 - Production/Stable',
          'Intended Audience :: Telecommunications Industry',
          'License :: OSI Approved :: GNU General Public License v3 or later (GPLv3+)',
          'Topic :: Software Development :: Testing :: Traffic Generation',
          'Topic :: System :: Networking']
      )


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

import os
import shutil
import sys
from setuptools import setup

def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

if sys.argv[1] == 'clean_sdist':
    shutil.rmtree('dist', ignore_errors = True)
    shutil.rmtree('ostinato.egg-info', ignore_errors = True)
    sys.exit(0)

setup(name = 'ostinato',
      version = 'FIXME',
      author = 'Srivats P',
      author_email = 'pstavirs@gmail.com',
      license = "GPLv3+",
      url = 'http://ostinato.org',
      description = 'Ostinato is a network packet and traffic generator and analyzer. It aims to be "Wireshark in Reverse" and become complementary to Wireshark. It features custom packet crafting via a GUI or a script',
      long_description = read('README.txt'),
      install_requires = ['google.protobuf>=2.3'],
      packages=['ostinato', 'ostinato.protocols'],
      package_dir={'ostinato': ''}
      )

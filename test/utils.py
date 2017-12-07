#! /usr/bin/env python
import subprocess
import sys

#from distutils.spawn import find_executable
from distutils.version import LooseVersion

def get_tshark(minversion = None):
    tshark = 'tshark'
    #if sys.platform == 'win32':
    #    tshark = find_executable(tshark)
    if tshark and minversion:
        out = subprocess.check_output([tshark, '-v']).split(None, 2)
        # we expect output to be of the form 'Tshark <version> ..."
        ver = out[1]
        if LooseVersion(ver) < LooseVersion(minversion):
            tshark = None

    if tshark is not None:
        print('%s version %s found' % (tshark, ver))
    return tshark


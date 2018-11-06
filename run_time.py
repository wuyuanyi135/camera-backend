#!/usr/bin/env python

import os
import sys
import subprocess
if not os.environ.get('CMAKE_DURING_BUILD'):
    os.system("mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make")

if os.environ.get('DEV'):
    sys.exit(subprocess.call(['gdbserver', ":1234", 'build/src/vimba_backend']))
else:
    sys.exit(subprocess.call(['build/src/vimba_backend']))

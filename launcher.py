#!/usr/bin/python3

import sys
from ctypes import *

def main():
    lib = cdll.LoadLibrary("./kai")
    entry = getattr(lib, "main")
    argv = [  ]
    for s in sys.argv:
        argv.append(s.encode("utf-8"))
    argvptr = (c_char_p * len(sys.argv))(*argv)
    return entry(len(sys.argv), argvptr)

sys.exit(main())

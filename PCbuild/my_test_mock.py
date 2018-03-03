import __builtin__
import sys
import os.path

prefix = sys.executable
mod_prefix = os.path.normpath(os.path.join(prefix, "..", "..", "Lib"))

orig_open = __builtin__.open

def mod_open(filename, *args, **kwargs):
    if filename.startswith(prefix):
        print("====================++++++++++++++++++++")
        print(filename)
        filename = mod_prefix + filename[len(prefix):]
        print(filename)
    return orig_open(filename, *args, **kwargs)

__builtin__.open = mod_open

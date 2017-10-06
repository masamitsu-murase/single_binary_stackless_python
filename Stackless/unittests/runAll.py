"""
runAll.py

Runs all test-cases in files named test_*.py.
Should be run in the folder Stackless/unittests.

This driver reuses the CPython test.regrtest driver and
supports its options.
"""
from __future__ import absolute_import

import os
# just to make sure, it can be imported
import support  # @UnusedImport

from test import libregrtest
from test import __path__ as test_path

# path of the Stackless unittest directory
path = os.path.abspath(os.path.split(__file__)[0])
# add the stackless unittest dir to the package "test"
test_path.insert(0, path)

def main(tests=None, **kwargs):
    kwargs.setdefault("testdir", path)
    return libregrtest.main(tests=tests, **kwargs)

def main_in_temp_cwd(tests=None, **kwargs):
    kwargs.setdefault("testdir", path)
    # unfortunately, libregrtest.main_in_temp_cwd() does not support
    # **kwargs. Monkey patch it
    mglobals = libregrtest.main_in_temp_cwd.__globals__
    real_main = mglobals["main"]
    def main():
        return real_main(tests, **kwargs)
    mglobals["main"] = main
    try:
        libregrtest.main_in_temp_cwd()
    finally:
        mglobals["main"] = real_main

if __name__ == '__main__':
    main_in_temp_cwd()
    import gc
    gc.set_debug(gc.DEBUG_UNCOLLECTABLE)

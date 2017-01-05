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

from test import regrtest
from test import __path__ as test_path

# path of the Stackless unittest direectory
path = os.path.split(__file__)[0]
# add the stackless unittest dir to the package "test"
test_path.insert(0, path)

# monkey patch: remove all standard tests
del regrtest.STDTESTS[:]


def main():
    return regrtest.main(testdir=path)

if __name__ == '__main__':
    main()

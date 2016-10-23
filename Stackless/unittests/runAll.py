"""
runAll.py

Runs all testcases in files named test_*.py.
Should be run in the folder Stackless/unittests.
"""

import os
import unittest


def main():
    path = os.path.split(__file__)[0]
    print(path)
    testSuite = unittest.TestLoader().discover(path)
    unittest.TextTestRunner(verbosity=2).run(testSuite)

if __name__ == '__main__':
    main()
    import gc
    gc.set_debug(gc.DEBUG_UNCOLLECTABLE)

# coding: utf-8

import sys
import unittest
import os.path

sys.path.append(os.path.dirname(__file__))

def main():
    loader = unittest.TestLoader()
    test = loader.discover(os.path.dirname(__file__), "test*.py")
    # test = loader.discover(os.path.dirname(__file__), "*/test*.py")
    runner = unittest.TextTestRunner(verbosity=1)
    sys.exit(not runner.run(test).wasSuccessful())

main()

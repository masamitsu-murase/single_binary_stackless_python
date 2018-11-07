# coding: utf-8

import sys
import unittest
import subprocess


class TestSingleBinaryStacklessPython(unittest.TestCase):
    def test_single_binary_stackless_python(self):
        subprocess.check_call([sys.executable, "-m", "single_binary_stackless_python"], stdout=subprocess.DEVNULL)


if __name__ == "__main__":
    unittest.main()

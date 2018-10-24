# coding: utf-8

import os
import sys
import unittest
import flake8_lite
import subprocess
import textwrap


class TestFlake8Lite(unittest.TestCase):
    SAMPLE_FILENAME = "sample.py"
    SAMPLE_CODE = """\
    import sys
    import unordered_module


    def sample():
        pass
    def no_empty_line():
        unused_variable = None
        sys.exit()
    """

    def setUp(self):
        with open(self.SAMPLE_FILENAME, "w") as file:
            file.write(textwrap.dedent(self.SAMPLE_CODE))

    def tearDown(self):
        os.remove(self.SAMPLE_FILENAME)

    def run_flake8_lite(self, args=[]):
        return subprocess.run([sys.executable, "-m", "flake8_lite"] + args + [self.SAMPLE_FILENAME], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True).stdout

    def test_normal_check(self):
        output = self.run_flake8_lite()
        self.assertIn(" F401 ", output)
        self.assertIn(" E302 ", output)
        self.assertIn(" F841 ", output)

    def test_ignore_option(self):
        output = self.run_flake8_lite(["--ignore=E302"])
        self.assertIn(" F401 ", output)
        self.assertNotIn(" E302 ", output)
        self.assertIn(" F841 ", output)

        output = self.run_flake8_lite(["--ignore=F401,E302"])
        self.assertNotIn(" F401 ", output)
        self.assertNotIn(" E302 ", output)
        self.assertIn(" F841 ", output)

    def test_format_option(self):
        output = self.run_flake8_lite(["--format=%(code)s %(filename)s>%(row)d:%(col)d"])
        for code in ("F401", "E302", "F841"):
            self.assertTrue(any(i.startswith(code + " " + self.SAMPLE_FILENAME + ">") for i in output.splitlines()), "Check %s" % code)


if __name__ == "__main__":
    unittest.main()

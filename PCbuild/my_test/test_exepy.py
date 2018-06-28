# coding: utf-8

import os
import os.path
import sys
import unittest
import subprocess
import shutil
import textwrap
import glob


class TestExepy(unittest.TestCase):
    TEMPDIR = "tempdir"

    SINGLE_FILE_CONTENT = textwrap.dedent("""
    import sys
    print(len(sys.argv))
    """[1:])

    DOUBLE_FILE_CONTENT1 = textwrap.dedent("""
    import sys
    import hoge1
    hoge1.test()
    """[1:])

    SECOND_FILE_NAME = "hoge1.py"
    DOUBLE_FILE_CONTENT2 = textwrap.dedent("""
    import sys
    def test():
        print("file2")
    """[1:])

    def create_single_test_file(self, name):
        with open(name, "w") as file:
            file.write(self.SINGLE_FILE_CONTENT)

    def create_double_test_file(self, name):
        with open(name, "w") as file:
            file.write(self.DOUBLE_FILE_CONTENT1)
        with open(self.SECOND_FILE_NAME, "w") as file:
            file.write(self.DOUBLE_FILE_CONTENT2)

    def test_create_exe(self):
        os.mkdir(self.TEMPDIR)
        pwd = os.path.abspath(os.curdir)
        os.chdir(self.TEMPDIR)
        try:
            filename = "hoge.py"
            exename = "hoge.exe"
            self.create_single_test_file(filename)
            cmd = [sys.executable, "-m", "exepy", exename, filename]
            subprocess.check_call(cmd)
            os.remove(filename)
            output = subprocess.check_output(exename, universal_newlines=True)
            self.assertEqual(output, "1\n")
        finally:
            os.chdir(pwd)
            shutil.rmtree(self.TEMPDIR)

    def test_2_files(self):
        os.mkdir(self.TEMPDIR)
        pwd = os.path.abspath(os.curdir)
        os.chdir(self.TEMPDIR)
        try:
            filename = "hoge.py"
            exename = "hoge.exe"
            self.create_double_test_file(filename)
            cmd = [sys.executable, "-m", "exepy", exename, filename, self.SECOND_FILE_NAME]
            subprocess.check_call(cmd)
            for file in glob.iglob("*.py"):
                os.remove(file)
            output = subprocess.check_output(exename, universal_newlines=True)
            self.assertEqual(output, "file2\n")
        finally:
            os.chdir(pwd)
            shutil.rmtree(self.TEMPDIR)

    def test_argument(self):
        os.mkdir(self.TEMPDIR)
        pwd = os.path.abspath(os.curdir)
        os.chdir(self.TEMPDIR)
        try:
            filename = "hoge.py"
            exename = "hoge.exe"
            self.create_single_test_file(filename)
            cmd = [sys.executable, "-m", "exepy", exename, filename]
            subprocess.check_call(cmd)
            os.remove(filename)
            output = subprocess.check_output([exename, "arg1", "arg2"], universal_newlines=True)
            self.assertEqual(output, "3\n")
        finally:
            os.chdir(pwd)
            shutil.rmtree(self.TEMPDIR)


if __name__ == "__main__":
    unittest.main()

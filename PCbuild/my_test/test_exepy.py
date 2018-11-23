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

    SINGLE_FILE_CONTENT = textwrap.dedent("""\
    import sys
    print(len(sys.argv))
    """)

    DOUBLE_FILE_CONTENT1 = textwrap.dedent("""\
    import sys
    import hoge1
    hoge1.test()
    """)

    SECOND_FILE_NAME = "hoge1.py"
    DOUBLE_FILE_CONTENT2 = textwrap.dedent("""\
    import sys
    def test():
        print("file2")
    """)

    RESOURCE_FILE_CONTENT1 = textwrap.dedent("""\
    import dir
    import os

    data = __loader__.get_data(os.path.join(os.path.dirname(__file__), "image.bin"))
    if data == b"\x01\x02\x03\x04\x05\x06":
        print("ok")
    else:
        print("ng")
    """)
    RESOURCE_FILE_CONTENT2 = textwrap.dedent("""\
    import os

    data = __loader__.get_data(os.path.join(os.path.dirname(__file__), "image.bin"))
    if data == b"\x01\x02\x03\x04\x05\x06" * 2:
        print("ok")
    else:
        print("ng")
    """)

    def create_single_test_file(self, name):
        with open(name, "w") as file:
            file.write(self.SINGLE_FILE_CONTENT)

    def create_double_test_file(self, name):
        with open(name, "w") as file:
            file.write(self.DOUBLE_FILE_CONTENT1)
        with open(self.SECOND_FILE_NAME, "w") as file:
            file.write(self.DOUBLE_FILE_CONTENT2)

    def create_resource_test_file(self, name):
        with open(name, "w") as file:
            file.write(self.RESOURCE_FILE_CONTENT1)
        with open("image.bin", "wb") as file:
            file.write(b"\x01\x02\x03\x04\x05\x06")
        os.mkdir("dir")
        os.makedirs("dir/dir")
        with open("dir/__init__.py", "w") as file:
            file.write(self.RESOURCE_FILE_CONTENT2)
        with open("dir/image.bin", "wb") as file:
            file.write(b"\x01\x02\x03\x04\x05\x06" * 2)

    def test_create_exe(self):
        os.mkdir(self.TEMPDIR)
        pwd = os.path.abspath(os.curdir)
        os.chdir(self.TEMPDIR)
        try:
            filename = "hoge.py"
            exename = "hoge.exe"
            self.create_single_test_file(filename)
            cmd = [sys.executable, "-m", "exepy", "create", exename, filename]
            subprocess.check_call(cmd, stdout=subprocess.DEVNULL)
            os.remove(filename)
            output = subprocess.check_output(exename, universal_newlines=True)
            self.assertEqual(output, "1\n")

            cmd = [sys.executable, "-m", "exepy", "extract", exename]
            subprocess.check_call(cmd, stdout=subprocess.DEVNULL)
            with open(filename, "r") as file:
                content = file.read()
            self.assertEqual(content, self.SINGLE_FILE_CONTENT)

            cmd = [sys.executable, "-m", "exepy", "extract", exename]
            returncode = subprocess.call(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            self.assertNotEqual(returncode, 0)

            cmd = [sys.executable, "-m", "exepy", "extract", "-f", exename]
            returncode = subprocess.call(cmd, stdout=subprocess.DEVNULL)
            self.assertEqual(returncode, 0)
        finally:
            os.chdir(pwd)
            shutil.rmtree(self.TEMPDIR)

    def test_nocommand(self):
        os.mkdir(self.TEMPDIR)
        pwd = os.path.abspath(os.curdir)
        os.chdir(self.TEMPDIR)
        try:
            filename = "hoge.py"
            exename = "hoge.exe"
            self.create_single_test_file(filename)
            cmd = [sys.executable, "-m", "exepy", "--nocommand", "create", exename, filename]
            subprocess.check_call(cmd, stdout=subprocess.DEVNULL)
            os.remove(filename)

            cmd = [sys.executable, "-m", "exepy", "extract", exename]
            output = subprocess.check_output(cmd)
            with open(filename, "r") as file:
                content = file.read()
            self.assertEqual(content, self.SINGLE_FILE_CONTENT)
            self.assertIn("no argv", output)
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
            cmd = [sys.executable, "-m", "exepy", "c", exename, filename, self.SECOND_FILE_NAME]
            subprocess.check_call(cmd, stdout=subprocess.DEVNULL)
            for file in glob.iglob("*.py"):
                os.remove(file)
            output = subprocess.check_output(exename, universal_newlines=True)
            self.assertEqual(output, "file2\n")

            cmd = [sys.executable, "-m", "exepy", "e", exename]
            subprocess.check_call(cmd, stdout=subprocess.DEVNULL)
            with open(filename, "r") as file:
                content1 = file.read()
            with open(self.SECOND_FILE_NAME, "r") as file:
                content2 = file.read()
            self.assertEqual(content1, self.DOUBLE_FILE_CONTENT1)
            self.assertEqual(content2, self.DOUBLE_FILE_CONTENT2)
        finally:
            os.chdir(pwd)
            shutil.rmtree(self.TEMPDIR)

    def test_resource(self):
        os.mkdir(self.TEMPDIR)
        pwd = os.path.abspath(os.curdir)
        os.chdir(self.TEMPDIR)
        try:
            filename = "hoge.py"
            exename = "hoge.exe"
            self.create_resource_test_file(filename)
            cmd = [sys.executable, "-m", "exepy", "create", exename, filename, "image.bin", "dir"]
            subprocess.check_call(cmd, stdout=subprocess.DEVNULL)
            os.remove(filename)
            os.remove("image.bin")
            shutil.rmtree("dir")
            output = subprocess.check_output([exename], universal_newlines=True)
            self.assertEqual(output, "ok\nok\n")
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
            cmd = [sys.executable, "-m", "exepy", "create", exename, filename]
            subprocess.check_call(cmd, stdout=subprocess.DEVNULL)
            os.remove(filename)
            output = subprocess.check_output([exename, "arg1", "arg2"], universal_newlines=True)
            self.assertEqual(output, "3\n")
        finally:
            os.chdir(pwd)
            shutil.rmtree(self.TEMPDIR)


if __name__ == "__main__":
    unittest.main()

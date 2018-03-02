# coding: utf-8

import os
import sys
import unittest

class TestOriginalChanges(unittest.TestCase):
    def test_flags(self):
        self.assertEqual(sys.flags.dont_write_bytecode, 1)
        self.assertEqual(sys.flags.ignore_environment, 1)

    def test_version(self):
        version = sys.version
        if os.environ["BUILD_TARGET_CPU"] == "x86":
            self.assertIn("32 bit (Intel)", version)
        else:
            self.assertIn("64 bit (AMD64)", version)
    
    def test_windows_pe(self):
        if os.environ["WINDOWS_PE"] == "1":
            self.assertNotIn("_msi", sys.builtin_module_names)

if __name__ == "__main__":
    unittest.main()

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
        self.assertIn("Stackless", version)
        if os.environ["BUILD_TARGET_CPU"] == "x86":
            self.assertIn("32 bit (Intel)", version)
        else:
            self.assertIn("64 bit (AMD64)", version)
    
    def test_windows_pe(self):
        if os.environ["WINDOWS_PE"] == "1":
            self.assertNotIn("_msi", sys.builtin_module_names)

    def test_embeddedimport(self):
        import comtypes
        import yaml
        import pyreadline
        import jinja2

    def test_xml(self):
        import xml.etree.ElementTree as ET

        xml_text = """<data a="123" z="234" m="345">
          <test a="123" z="234" m="345">test</test>
        </data>"""
        xml_text_sorted = """<data a="123" m="345" z="234">
          <test a="123" m="345" z="234">test</test>
        </data>"""
        root = ET.fromstring(xml_text)
        output_sorted = ET.tostring(root, encoding="unicode")
        output = ET.tostring(root, encoding="unicode", sort_attrib=False)
        self.assertEqual(xml_text_sorted, output_sorted)
        self.assertEqual(xml_text, output)

if __name__ == "__main__":
    unittest.main()
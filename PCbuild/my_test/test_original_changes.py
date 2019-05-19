# coding: utf-8

import os
import re
import sys
import unittest


class TestOriginalChanges(unittest.TestCase):
    def xtest_lib2to3(self):
        import lib2to3.main
        self.assertTrue(lib2to3, "import lib2to3.main")

    def test_werkzeug(self):
        import werkzeug.debug
        app = werkzeug.debug.DebuggedApplication(None)
        self.assertIsNotNone(app.get_resource(None, "less.png"))

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
        import json
        self.assertEqual(json.__file__, sys.executable + r"\json\__init__.py")
        self.assertEqual(json.__loader__.get_filename("json"), sys.executable + r"\json\__init__.py")
        with open(os.path.join(os.path.dirname(__file__), "../../Lib/json/__init__.py"), "r") as file:
            source = re.sub(r"[ \t]*#.*", "", file.read())  # Remove comment easily.
        self.assertEqual(json.__loader__.get_source("json"), source)

        import json.decoder
        self.assertEqual(json.decoder.__file__, sys.executable + r"\json\decoder.py")
        self.assertEqual(json.decoder.__loader__.get_filename("json.decoder"), sys.executable + r"\json\decoder.py")
        with open(os.path.join(os.path.dirname(__file__), "../../Lib/json/decoder.py"), "r") as file:
            source = re.sub(r"[ \t]*#.*", "", file.read())  # Remove comment easily.
        self.assertEqual(json.decoder.__loader__.get_source("json.decoder"), source)

        with self.assertRaises(ImportError):
            json.__loader__.get_filename("json_not_exist")

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

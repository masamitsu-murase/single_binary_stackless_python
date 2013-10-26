from .. import abc
from .. import util
from . import util as builtin_util

frozen_machinery, source_machinery = util.import_importlib('importlib.machinery')

import sys
import unittest


class FinderTests(abc.FinderTests):

    """Test find_module() for built-in modules."""

    def test_module(self):
        # Common case.
        with util.uncache(builtin_util.NAME):
            found = self.machinery.BuiltinImporter.find_module(builtin_util.NAME)
            self.assertTrue(found)

    def test_package(self):
        # Built-in modules cannot be a package.
        pass

    def test_module_in_package(self):
        # Built-in modules cannobt be in a package.
        pass

    def test_package_in_package(self):
        # Built-in modules cannot be a package.
        pass

    def test_package_over_module(self):
        # Built-in modules cannot be a package.
        pass

    def test_failure(self):
        assert 'importlib' not in sys.builtin_module_names
        loader = self.machinery.BuiltinImporter.find_module('importlib')
        self.assertIsNone(loader)

    def test_ignore_path(self):
        # The value for 'path' should always trigger a failed import.
        with util.uncache(builtin_util.NAME):
            loader = self.machinery.BuiltinImporter.find_module(builtin_util.NAME,
                                                            ['pkg'])
            self.assertIsNone(loader)

Frozen_FinderTests, Source_FinderTests = util.test_both(FinderTests,
        machinery=[frozen_machinery, source_machinery])


if __name__ == '__main__':
    unittest.main()
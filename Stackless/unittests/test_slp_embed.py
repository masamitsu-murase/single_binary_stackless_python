# Run the tests in Stackless/unittests/_testembed_slp.c
# (tests for the Stackless Python embedding APIs)
"""Test the (Stackless)-Python C-API

Tests not relevant for pure Python code.
"""

from __future__ import print_function, absolute_import, division

import unittest
import os
import sys

from support import test_main  # @UnusedImport

from test.test_embed import EmbeddingTestsMixin
from test.support import verbose


@unittest.skip("Stackless issue 218")
class EmbeddingTests(EmbeddingTestsMixin, unittest.TestCase):
    def test_schedule(self):
        env = dict(os.environ)
        out, err = self.run_embedded_interpreter("slp_schedule", env=env)
        if verbose > 1:
            print()
            print(out)
            print(err)
        expected_output = '\n'.join([
            "Hello, World!"])
        # This is useful if we ever trip over odd platform behaviour
        self.maxDiff = None
        self.assertEqual(out.strip(), expected_output)


if __name__ == "__main__":
    if not sys.argv[1:]:
        sys.argv.append('-v')
    unittest.main()

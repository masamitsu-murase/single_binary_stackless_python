from __future__ import absolute_import
import unittest
import gc
import stackless
import types
import pickle
import copyreg

from support import test_main  # @UnusedImport
from support import StacklessTestCase


def f():
    yield 1


class TestGarbageCollection(StacklessTestCase):

    def testSimpleLeakage(self):
        leakage = []

        with stackless.atomic():
            gc.collect(2)
            before = frozenset(id(o) for o in gc.get_objects())

            for i in f():
                pass

            gc.collect(2)
            after = gc.get_objects()

        for x in after:
            if x is not before and id(x) not in before:
                leakage.append(x)

        if len(leakage):
            self.failUnless(len(leakage) == 0, "Leaked %s" % repr(leakage))


class TestGeneratorWrapper(StacklessTestCase):
    def test_run_wrap_generator(self):
        g = stackless._wrap.generator()
        self.assertIsInstance(g, types.GeneratorType)
        self.assertIsNot(type(g), types.GeneratorType)
        self.assertRaises(StopIteration, next, g)

    def test_wrap_generator_frame_code(self):
        g0 = stackless._wrap.generator()
        g1 = stackless._wrap.generator()
        self.assertIsInstance(g0.gi_frame, types.FrameType)
        self.assertIsInstance(g0.gi_code, types.CodeType)
        self.assertIs(g0.gi_code, g1.gi_code)
        self.assertIsNot(g0.gi_frame, g1.gi_frame)
        self.assertEqual(g0.__name__, "exhausted_generator")


class TestGeneratorPickling(StacklessTestCase):
    def test_name(self):
        def g():
            yield 1
        gen_orig = g()
        p = pickle.dumps(gen_orig)
        gen_new = pickle.loads(p)

        self.assertEqual(gen_new.__name__, gen_orig.__name__)

    def test_qualname(self):
        def g():
            yield 1
        gen_orig = g()
        p = pickle.dumps(gen_orig)
        gen_new = pickle.loads(p)

        self.assertEqual(gen_new.__qualname__, gen_orig.__qualname__)

    def test_noname(self):
        # Ensure unpickling compatibility with Python versions < 3.5
        def g():
            yield 11
        gen_orig = g()

        r = copyreg.dispatch_table[type(gen_orig)](gen_orig)

        self.assertIsInstance(r, tuple)
        self.assertEqual(len(r), 3)
        self.assertEqual(len(r[2]), 4)

        gen_new = r[0](*r[1])
        self.assertEqual(gen_new.__qualname__, "exhausted_generator")
        self.assertEqual(gen_new.__name__, "exhausted_generator")
        self.assertIs(type(gen_new), stackless._wrap.generator)

        # build the pre 3.5 argument tuple for __setstate__
        r = r[2][:-2]
        r = (r[0].frame,) + r[1:]
        gen_new.__setstate__(r)

        self.assertEqual(gen_new.__qualname__, "g")
        self.assertEqual(gen_new.__name__, "g")
        self.assertIs(type(gen_new), type(gen_orig))

        v = gen_new.__next__()
        self.assertEqual(v, 11)

    def test_exhausted(self):
        def g():
            yield 1
        gen_orig = g()
        self.assertEqual(gen_orig.__next__(), 1)
        self.assertRaises(StopIteration, gen_orig.__next__)
        p = pickle.dumps(gen_orig)
        self.assertRaises(StopIteration, gen_orig.__next__)
        gen_new = pickle.loads(p)
        self.assertRaises(StopIteration, gen_new.__next__)


if __name__ == '__main__':
    unittest.main()

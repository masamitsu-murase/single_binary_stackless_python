from __future__ import absolute_import
import unittest
import gc
import stackless
import types
import pickle
import copyreg
import sys

from support import test_main  # @UnusedImport
from support import StacklessTestCase, captured_stderr


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

    def run_GC_test(self, task, arg):
        tasklet = stackless.tasklet(task)(arg)
        tasklet.run()
        if False:  # To test the generator / coroutine / async gen
            tasklet.run()
            self.assertFalse(tasklet.alive)
            return
        self.assertTrue(tasklet.paused)
        tasklet.tempval = None
        with captured_stderr() as stringio:
            # must not throw or output
            if tasklet.restorable:
                tasklet.bind(None)
            # make sure, that t=None kills the last reference
            self.assertEqual(sys.getrefcount(tasklet), 2)
            tasklet = None
        self.assertEqual(stringio.getvalue(), "")

    def testGCRunningGenerator(self):
        def gen():
            try:
                # print("gen nesting level: ", stackless.current.nesting_level)
                stackless.schedule_remove()
                yield 1
            except:  # @IgnorePep8
                # print("exception in gen:", sys.exc_info())
                raise

        def task(generator):
            l = [i for i in generator]
            self.assertListEqual(l, [1])

        self.run_GC_test(task,gen())

    def testGCRunningCoroutine(self):
        async def coro():
            try:
                # print("coro nesting level: ", stackless.current.nesting_level)
                stackless.schedule_remove()
            except:  # @IgnorePep8
                # print("exception in coro:", sys.exc_info())
                raise

        def task(c):
            self.assertRaises(StopIteration, c.send, None)

        self.run_GC_test(task, coro())

    def testGCRunningAsyncGen(self):
        async def asyncgen():
            try:
                # print("asyncgen nesting level: ", stackless.current.nesting_level)
                stackless.schedule_remove()
            except:  # @IgnorePep8
                # print("exception in asyncgen:", sys.exc_info())
                raise
            yield 100

        def task(ag):
            c = ag.__anext__()
            with self.assertRaises(StopIteration) as cm:
                c.send(None)
            self.assertEqual(cm.exception.value, 100)
            c = ag.__anext__()
            self.assertRaises(StopAsyncIteration, c.send, None)

        self.run_GC_test(task, asyncgen())


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

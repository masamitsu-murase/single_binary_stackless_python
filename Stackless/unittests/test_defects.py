import unittest
import stackless
import gc
import sys
import types

from stackless import _test_nostacklesscall as apply
from support import StacklessTestCase, captured_stderr, require_one_thread


"""
Various regression tests for stackless defects.
Typically, one can start by adding a test here, then fix it.
Don't check in tests for un-fixed defects unless they are disabled (by adding a leading _)
"""


class TestTaskletDel(StacklessTestCase):
    # Defect:  If a tasklet's tempval contains any non-trivial __del__ function, it will cause
    # an assertion in debug mode due to violation of the stackless protocol.
    # The return value of the tasklet's function is stored in the tasklet's tempval and cleared
    # when the tasklet exits.
    # Also, the tasklet itself would have problems with a __del__ method.

    class ObjWithDel:

        def __del__(self):
            self.called_func()

        def called_func(self):
            pass  # destructor must call a function

    class TaskletWithDel(stackless.tasklet):

        def __del__(self):
            self.func()

        def func(self):
            pass

    class TaskletWithDelAndCollect(stackless.tasklet):

        def __del__(self):
            gc.collect()

    def BlockingReceive(self):
        # Function to block when run in a tasklet.
        def f():
            # must store c in locals
            c = stackless.channel()
            c.receive()
        return stackless.tasklet(f)()

    # Test that a tasklet tempval's __del__ operator works.
    def testTempval(self):
        def TaskletFunc(self):
            return self.ObjWithDel()

        stackless.tasklet(TaskletFunc)(self)
        stackless.run()

    # Test that a tasklet's __del__ operator works.
    def testTasklet(self):
        def TaskletFunc(self):
            pass

        self.TaskletWithDel(TaskletFunc)(self)
        stackless.run()

    # a gc.collect() in a tasklet's __del__ method causes
    def testCrash1(self):
        # we need a lost blocked tasklet here (print the ids for debugging)
        hex(id(self.BlockingReceive()))
        gc.collect()  # so that there isn't any garbage
        stackless.run()

        def TaskletFunc(self):
            pass
        hex(id(self.TaskletWithDelAndCollect(TaskletFunc)(self)))
        stackless.run()  # crash here


class Schedule(StacklessTestCase):

    def testScheduleRemove(self):
        # schedule remove doesn't work if it is the only tasklet running under watchdog
        def func(self):
            stackless.schedule_remove()
            self.fail("We shouldn't be here")
        stackless.run()  # flush all runnables
        stackless.tasklet(func)(self)
        stackless.run()

    @require_one_thread
    def testScheduleRemove2(self):
        # schedule remove doesn't work if it is the only tasklet with main blocked
        # main tasklet is blocked, this should raise an error
        def func(self, chan):
            self.assertRaises(RuntimeError, stackless.schedule_remove)
            chan.send(None)
        stackless.run()  # flush all runnables
        chan = stackless.channel()
        stackless.tasklet(func)(self, chan)
        chan.receive()

    def testScheduleRemove3(self):
        '''Schedule-remove the last reference to a tasklet 1'''
        def func():
            stackless.schedule_remove(None)
        stackless.tasklet(func)()
        stackless.run()

    def testScheduleRemove4(self):
        '''Schedule-remove the last reference to a tasklet 2'''
        def func():
            stackless.schedule_remove(None)
        stackless.tasklet(func)()
        stackless.schedule_remove(None)


class Channel(StacklessTestCase):

    def testTemporaryChannel(self):
        def f1():
            stackless.channel().receive()

        stackless.tasklet(f1)()
        old = stackless.enable_softswitch(True)
        try:
            stackless.run()
        finally:
            stackless.enable_softswitch(old)

    def testTemporaryChannel2(self):
        def f1():
            stackless.channel().receive()

        def f2():
            pass

        stackless.tasklet(f1)()
        stackless.tasklet(f2)()
        old = stackless.enable_softswitch(True)
        try:
            stackless.run()
        finally:
            stackless.enable_softswitch(old)


class TestInfiniteRecursion(StacklessTestCase):
    # test for http://www.stackless.com/ticket/20

    def testDirectRecursion(self):
        class A(object):
            # define __call__ in case http://www.stackless.com/ticket/18 is not fixed

            def __call__(self):
                pass
        A.__call__ = A()
        a = A()
        # might crash the Python(r) interpreter, if the recursion check does not kick in
        self.assertRaises(RuntimeError, a)

    def testIndirectDirectRecursion(self):
        class A(object):

            def __call__(self):
                pass

        class B(object):

            def __call__(self):
                pass
        A.__call__ = B()
        B.__call__ = A()
        a = A()
        self.assertRaises(RuntimeError, a)


class TestExceptionInScheduleCallback(StacklessTestCase):
    # Problem
    # Assertion failed: ts->st.current == NULL, file ..\Stackless\module\taskletobject.c, line 51
    # See https://bitbucket.org/stackless-dev/stackless/issue/38

    def scheduleCallback(self, prev, next):
        if next.is_main:
            raise RuntimeError("scheduleCallback")

    #@unittest.skip('crashes python')
    def testExceptionInScheduleCallback(self):
        stackless.set_schedule_callback(self.scheduleCallback)
        self.addCleanup(stackless.set_schedule_callback, None)
        stackless.tasklet(lambda: None)()
        with captured_stderr() as stderr:
            stackless.run()
        self.assertTrue("scheduleCallback" in stderr.getvalue())


class TestCrashUponFrameUnpickling(StacklessTestCase):

    def testCrasher(self):
        import pickle
        frame = sys._getframe()
        frameType = type(frame)
        while frame and frame.f_back:
            frame = frame.f_back
        p = pickle.dumps(frame, -1)
        frame = None
        frame = pickle.loads(p)
        self.assertIsInstance(frame, frameType)

        # this access crashes Stackless versions released before Feb 2nd 2014
        f_back = frame.f_back

        self.assertIsNone(f_back)

    def testMissingLocalsplusCrasher(self):
        # A test case for issue #61 https://bitbucket.org/stackless-dev/stackless/issue/61
        #
        # Some versions of stackless create pickles of frames with the localsplus tuple set to None.
        # This test creates a frame with localsplus=None and ensures that Python does not crash upon
        # accessing frame.f_locals
        def reduce_current():
            result = []

            def func(current):
                result.append(stackless._wrap.frame.__reduce__(current.frame))
            stackless.tasklet().bind(func, (stackless.current,)).run()
            return result[0]

        func, args, state = reduce_current()
        # state is a tuple of the form
        # ('f_code', 'valid', 'exec_name', 'f_globals', 'have_locals',
        #  'f_locals', 'f_trace', 'exc_as_tuple', 'f_lasti', 'f_lineno',
        #  'blockstack_as_tuple', 'localsplus_as_tuple')
        self.assertEqual(len(state), 12)

        state = list(state)

        # set valid=0, localsplus_as_tuple=None
        state[1] = 0
        state[-1] = None

        state = tuple(state)
        # create the frame
        frame = func(*args)
        frame.__setstate__(state)
        self.assertIsInstance(frame, types.FrameType)

        # this access crashes Stackless versions released before May 20th 2014
        f_locals = frame.f_locals

        self.assertIsInstance(f_locals, dict)


class TestShutdown(StacklessTestCase):

    def test_cstack_new(self):
        # test for issue #80 https://bitbucket.org/stackless-dev/stackless/issues/80/
        import subprocess
        rc = subprocess.call([sys.executable, "-S", "-E", "-c", """if 1:
            import stackless, sys
            from stackless import _test_nostacklesscall as apply

            def func():
                global channel
                assert stackless.current.nesting_level == 0
                assert apply(lambda : stackless.current.nesting_level) == 1, "apply does not recurse"
                apply(channel.receive)  # crash at nesting level 1

            channel = stackless.channel()
            task = stackless.tasklet().bind(func, ())  # simplest tasklet
            task.run()
            sys.exit(42)
            """])
        self.assertEqual(rc, 42)

    def test_interthread_kill(self):
        # test for issue #87 https://bitbucket.org/stackless-dev/stackless/issues/87/
        import subprocess
        rc = subprocess.call([sys.executable, "-S", "-E", "-c", """from __future__ import print_function, absolute_import\nif 1:
            import sys
            import _thread as thread
            import stackless
            import os
            import time
            from stackless import _test_nostacklesscall as apply

            # This lock is used as a simple event variable.
            ready = thread.allocate_lock()
            ready.acquire()

            if False:  # change to True to enable debug messages
                sys.stdout = sys.stderr  # C-assert messages go to STDERR
            else:
                def print(*args):
                    pass

            # Module globals are cleared before __del__ is run
            # So we save functions, objects, ... in a class dict.
            class C(object):
                time_sleep = time.sleep

                def other_thread_main(self):
                    print("other thread started")
                    assert stackless.main.nesting_level == 0
                    self.main = stackless.main
                    assert stackless.main is stackless.current
                    t1 = stackless.tasklet(apply)(self.main.switch)
                    t1.run()
                    assert t1.paused
                    assert t1.nesting_level == 1
                    print("OT-Main:", self.main)
                    print("OT-t1:", t1)

                    try:
                        ready.release()
                        while True:
                            self.main.run()
                    except TaskletExit:
                        self.time_sleep(999999)

            print("Main-Thread:", stackless.current)
            x = C()
            thread.start_new_thread(x.other_thread_main, ())
            ready.acquire()  # Be sure the other thread is waiting.
            x.main.kill()
            time.sleep(0.5)  # give other thread time to run
            # state is now: other thread has 2 tasklets:
            # - main tasklet is stackless and blocked in sleep(999999)
            # - t1 has a C-state and is paused
            print("at end")
            sys.stdout.flush()
            sys.exit(42)
            """])
        self.assertEqual(rc, 42)


class TestStacklessProtokoll(StacklessTestCase):
    """Various tests for violations of the STACKLESS_GETARG() STACKLESS_ASSERT() protocol

    See https://bitbucket.org/stackless-dev/stackless/issues/84
    """
    def test_invalid_args_channel_next(self):
        """test of typeobject.c wrap_next(...)"""
        func = stackless.channel().__next__
        # func(None) causes the crash
        self.assertRaises(TypeError, func, None)

    def test_invalid_args_tasklet_kill(self):
        func = stackless.tasklet().kill
        # func(False, None) causes the crash
        self.assertRaises(TypeError, func, False, None)


if __name__ == '__main__':
    if not sys.argv[1:]:
        sys.argv.append('-v')
    unittest.main()

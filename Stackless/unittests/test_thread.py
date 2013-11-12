# import common

import pickle
import unittest
import stackless
import sys
import traceback
import contextlib
import time

from support import StacklessTestCase, AsTaskletTestCase
try:
    import threading
    try:
        import thread
    except ImportError:
        import _thread as thread
    withThreads = True
except:
    withThreads = False

class SkipMixin(object):
    def skipUnlessSoftSwitching(self):
        if not stackless.enable_softswitch(None):
            self.skipTest("requires softswitching")

def GetRemoteTasklets(callables):
    """Get a non-scheduled tasklet on a remote thread"""
    import threading
    c = stackless.channel()
    def tfunc():
        # thread func.  Create a tasklet, remove it, and send it to the master.
        # then wait for the tasklet to finish.
        try:
            c2 = stackless.channel()
            tasklets = []
            for callable in callables:
                def helper(callable):
                    try:
                        callable()
                    except:
                        c2.send_throw(*sys.exc_info())
                    else:
                        c2.send(None)
                t = stackless.tasklet(helper)(callable)
                t.remove()
                tasklets.append(t)
            c.send(tasklets)
        except:
            c.send_throw(*sys.exc_info())
        stackless.__reduce__()
        for callable in callables:
            c2.receive()
        stackless.run() #drain the scheduler

    thread = threading.Thread(target=tfunc)
    thread.start()
    d = c.receive(), thread
    return d

class LingeringThread(threading.Thread):
    """ A thread that lingers on after executing its main function"""
    def __init__(self, *args, **kwargs):
        self.real_target = kwargs["target"]
        kwargs["target"] = self.thread_func
        super(LingeringThread, self).__init__(*args, **kwargs)
        self.shutdown = threading.Event()

    def thread_func(self, *args, **kwargs):
        result = self.real_target(*args, **kwargs)
        self.linger()
        return result

    def linger(self):
        # wait until join is called
        self.shutdown.wait()

    def join(self):
        self.shutdown.set()
        return super(LingeringThread, self).join()

class SchedulingThread(LingeringThread):
    """ A thread that runs a scheduling loop after executing its main function"""
    def linger(self):
        while not self.shutdown.is_set():
            stackless.run()
            time.sleep(0.001)

def GetRemoteTasklet(callable, args):
    """Get a non-scheduled tasklet on a remote thread"""
    tasklets, thread = GetRemoteTasklets([lambda:callable(*args)])
    return tasklets[0], thread

@unittest.skipUnless(withThreads, "requires thread support")
class TestRemoteSchedule(AsTaskletTestCase):
    def setUp(self):
        super(TestRemoteSchedule, self).setUp()
        self.events = []
    def testFoo(self):
        def foo(): pass
        t, thread = GetRemoteTasklet(foo, ())
        try:
            t.run()
        finally:
            thread.join(2)

    def testRun(self):
        def foo():
            self.events.append(0)
        t, thread = GetRemoteTasklet(foo, ())
        try:
            t.run()
        finally:
            thread.join(2)
        self.assertEqual(self.events, list(range(len(self.events))))

    def testInsert(self):
        def foo():
            self.events.append(0)
        t, thread = GetRemoteTasklet(foo, ())
        try:
            t.insert()
        finally:
            thread.join(2)
        self.assertEqual(self.events, list(range(len(self.events))))


    def testRunOrder(self):
        def a():
            self.events.append(0)
        def b():
            self.events.append(1)
        def c():
            self.events.append(2)

        (t1, t2, t3), thread = GetRemoteTasklets((a, b, c))
        try:
            with stackless.atomic():
                t2.insert()
                t3.insert()
                t1.run() #t1 should run first
        finally:
            thread.join(2)
        self.assertEqual(self.events, list(range(3)))

@unittest.skipUnless(withThreads, "requires thread support")
class TestRebindCrash(SkipMixin, unittest.TestCase):
    """A crash from Anselm Kruis, occurring when transferring tasklet to a thread"""
    def create_remote_tasklet(self, nontrivial=False, job=None):
        result = []
        e1 = threading.Event()
        e2 = threading.Event()
        def remove():
            stackless.schedule_remove(retval=None)
        def taskletfunc():
            result.append(stackless.getcurrent())
            if nontrivial:
                stackless.test_cstate(remove)
            else:
                remove()
            if job:
                job()
        def threadfunc():
            t = stackless.tasklet(taskletfunc)()
            t.run()
            e1.set()
            while not e2.is_set():
                stackless.run()
                time.sleep(0.001)
            e2.wait() #wait until we can die
        t = threading.Thread(target=threadfunc)
        t.start()
        e1.wait()

        # callable to end the thread
        def end():
            e2.set()
            t.join()
        return end, result[0]

    def to_current_thread(self, task):
        """
        Get a tasklet for the current thread.

        If the tasklet already belongs to the current thread, this
        method returns the tasklet unmodified.

        Otherwise, this method tries to
        unbind the tasklet and returns a newly created tasklet. If
        unbinding fails, the method raises :exc:`RuntimeError`.
        """
        if task.thread_id == thread.get_ident():
            return task
        reducedTask = task.__reduce__()
        # raise RuntimeError, if task is alive but not paused
        task.bind(None)

        if False:  # python will crash if set to False
            frameList = reducedTask[2][3]
            for i in range(len(frameList)):
                frame = frameList[i]
                if isinstance(frame, stackless.cframe):
                    reducedFrame = frame.__reduce__()
                    newFrame = reducedFrame[0](*reducedFrame[1])
                    newFrame.__setstate__(reducedFrame[2])
                    frameList[i] = newFrame
        # rebind the task
        task = reducedTask[0](*reducedTask[1])
        task.__setstate__(reducedTask[2])
        return task

    def test_crash(self):
        self.skipUnlessSoftSwitching()
        end, task = self.create_remote_tasklet()
        try:
            task = self.to_current_thread(task)
            task.run()
        finally:
            end()


    def test_no_rebind(self):
        result = []
        e = threading.Event()
        def job():
            result.append(thread.get_ident())
            e.set()
        end, task = self.create_remote_tasklet(job=job)
        try:
            task.run()
            e.wait()
            self.assertNotEqual(result[0], thread.get_ident())
        finally:
            end()

    def test_rebind(self):
        self.skipUnlessSoftSwitching()
        result = []
        def job():
            result.append(thread.get_ident())
        end, task = self.create_remote_tasklet(job=job)
        try:
            task.bind_thread()
            task.run()
            self.assertEqual(result[0], thread.get_ident())
        finally:
            end()

    def test_rebind_nontrivial(self):
        end, task = self.create_remote_tasklet(nontrivial=True)
        try:
            self.assertRaises(RuntimeError, task.bind_thread)
        finally:
            end()

class RemoteTaskletTests(unittest.TestCase):
    ThreadClass = LingeringThread
    def setUp(self):
        super(RemoteTaskletTests, self).setUp()
        self.taskletExecuted = False
        self.event = threading.Event()

    def create_tasklet(self, action, *args, **kw):
        self.tasklet = stackless.tasklet(action)(*args, **kw)
        self.event.set()

    def tasklet_action(self):
        self.taskletExecuted = True

    def create_thread_task(self):
        theThread = self.ThreadClass(target=self.create_tasklet,
        args=(self.tasklet_action,))
        theThread.start()
        self.event.wait()
        t = self.tasklet
        return theThread, t


class TestRemove(RemoteTaskletTests):
    def test_remove_balance(self):
        """ Test that remove from the runqueue of a remote thread does not affect the
        bookkeeping of the current thread.
        """
        before = stackless.getruncount()
        thread, task = self.create_thread_task()
        after = stackless.getruncount()
        self.assertEqual(before, after)
        task.remove()
        after = stackless.getruncount()
        # only the runnable count on the remote thread
        # should change
        self.assertEqual(before, after)

    def test_insert_balance(self):
        """ Test that insert into the runqueue of a remote thread does not affect the
        bookkeeping of the current thread.
        """
        thread, task = self.create_thread_task()
        task.remove()
        before = stackless.getruncount()
        task.insert()
        after = stackless.getruncount()
        # only the runnable count on the remote thread
        # should change
        self.assertEqual(before, after)

class DeadThreadTest(RemoteTaskletTests):
    def test_tasklet_from_dead_thread(self):
        theThread, t = self.create_thread_task()
        self.assertTrue(t.alive)
        theThread.join()
        # now the tasklet should have been killed.
        self.assertFalse(t.alive)

    def test_removed_tasklet_from_dead_thread(self):
        theThread, t = self.create_thread_task()
        self.assertTrue(t.scheduled)
        t.remove()
        self.assertFalse(t.scheduled)
        theThread.join()
        # now the tasklet should have been killed.
        self.assertFalse(t.alive)

    def test_rebound_tasklet_from_dead_thread(self):
        theThread, t = self.create_thread_task()
        t.remove()
        t.bind_thread()
        theThread.join()
        # now the tasklet should be alive
        self.assertTrue(t.alive)
        t.run()
        self.assertFalse(t.alive)

    def test_bind_runnable(self):
        theThread, t = self.create_thread_task()
        self.assertRaises(RuntimeError, t.bind_thread)
        theThread.join()

class BindThreadTest(RemoteTaskletTests):
    """More unittests for tasklet.bind_thread"""

    def testForeignThread_scheduled(self):
        theThread, t = self.create_thread_task()
        try:
            self.assertEqual(t.thread_id, theThread.ident)
            self.assertTrue(t.alive)
            self.assertFalse(t.paused)
            t.remove()
            self.assertTrue(t.paused)

            t.bind_thread()

            self.assertTrue(t.alive)
            self.assertTrue(t.paused)

            self.assertNotEqual(t.thread_id, theThread.ident)
            self.assertEqual(t.thread_id, thread.get_ident())
            t.insert()
            self.assertFalse(t.paused)

            stackless.run()
            self.assertTrue(self.taskletExecuted)
            self.assertFalse(t.alive)
        finally:
            theThread.join()

#///////////////////////////////////////////////////////////////////////////////

if __name__ == '__main__':
    import sys
    if not sys.argv[1:]:
        sys.argv.append('-v')

    unittest.main()
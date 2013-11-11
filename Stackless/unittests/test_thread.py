# import common

import pickle
import unittest
import stackless
import sys
import traceback
import contextlib

from support import StacklessTestCase, AsTaskletTestCase
try:
    import threading
    import thread
    withThreads = True
except:
    withThreads = False


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
class TestRebindCrash(unittest.TestCase):
    """A crash from Anselm Kruis, occurring when transferring tasklet to a thread"""
    def create_remote_tasklet(self):
        result = []
        e1 = threading.Event()
        e2 = threading.Event()
        def taskletfunc():
            result.append(stackless.getcurrent())
            stackless.schedule_remove(retval=None)
        def threadfunc():
            t = stackless.tasklet(taskletfunc)()
            t.run()
            e1.set()
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
        end, task = self.create_remote_tasklet()
        try:
            task = self.to_current_thread(task)
            task.run()
        finally:
            end()



#///////////////////////////////////////////////////////////////////////////////

if __name__ == '__main__':
    import sys
    if not sys.argv[1:]:
        sys.argv.append('-v')

    unittest.main()
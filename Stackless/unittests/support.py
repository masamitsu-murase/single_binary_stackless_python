import stackless
try:
    import threading
    withThreads = True
except:
    withThreads = False
import unittest
import re
import contextlib
import sys
from io import StringIO


@contextlib.contextmanager
def captured_stderr():
    old = sys.stderr
    new = StringIO()
    sys.stderr = new
    try:
        yield new
    finally:
        sys.stderr = old


class RegexMixIn(object):
    """for pre-2.7 support """

    def assertRaisesRegexp(self, klass, rex, func, *args):
        try:
            func(*args)
        except Exception as e:
            self.assertTrue(re.match(rex, str(e)))
            self.assertTrue(isinstance(e, klass))
        else:
            self.assertTrue(False, "exception not raised")


class StacklessTestCase(unittest.TestCase, RegexMixIn):

    def setUp(self):
        self.assertEqual(stackless.getruncount(), 1, "Leakage from other tests, with %d tasklets still in the scheduler" % (stackless.getruncount() - 1))
        if withThreads:
            self.assertEqual(threading.activeCount(), 1, "Leakage from other threads, with %d threads running (1 expected)" % (threading.activeCount()))

    def tearDown(self):
        # Tasklets created in pickling tests can be left in the scheduler when they finish.  We can feel free to
        # clean them up for the tests.  Any tests that expect to exit with no leaked tasklets should do explicit
        # assertions to check.
        mainTasklet = stackless.getmain()
        current = mainTasklet.next
        while current is not None and current is not mainTasklet:
            next = current.next
            current.kill()
            current = next
        self.assertEqual(stackless.getruncount(), 1, "Leakage from this test, with %d tasklets still in the scheduler" % (stackless.getruncount() - 1))
        if withThreads and threading.activeCount() > 1:
            activeThreads = threading.enumerate()
            activeThreads.remove(threading.currentThread())
            if activeThreads:
                activeThreads[0].join(0.5)
            if threading.activeCount() > 1:
                self.assertEqual(threading.activeCount(), 1, "Leakage from other threads, with %d threads running (1 expected)" % (threading.activeCount()))

    SAFE_TESTCASE_ATTRIBUTES = unittest.TestCase(methodName='run').__dict__.keys()

    def _addSkip(self, result, reason):
        # Remove non standard attributes. They could render the test case object unpickleable.
        # This is a hack, but it works fairly well.
        for k in list(self.__dict__.keys()):
            if k not in self.SAFE_TESTCASE_ATTRIBUTES and \
                    not isinstance(self.__dict__[k], (type(None), str, int, float)):
                del self.__dict__[k]
        super(StacklessTestCase, self)._addSkip(result, reason)


class AsTaskletTestCase(StacklessTestCase):
    """A test case class, that runs tests as tasklets"""

    def setUp(self):
        self._ran_AsTaskletTestCase_setUp = True
        if stackless.enable_softswitch(None):
            self.assertEqual(stackless.current.nesting_level, 0)

        super(StacklessTestCase, self).setUp()  # yes, its intended: call setUp on the grand parent class
        self.assertEqual(stackless.getruncount(), 1, "Leakage from other tests, with %d tasklets still in the scheduler" % (stackless.getruncount() - 1))
        if withThreads:
            self.assertEqual(threading.activeCount(), 1, "Leakage from other threads, with %d threads running (1 expected)" % (threading.activeCount()))

    def run(self, result=None):
        c = stackless.channel()
        c.preference = 1  # sender priority
        self._ran_AsTaskletTestCase_setUp = False

        def helper():
            try:
                c.send(super(AsTaskletTestCase, self).run(result))
            except:
                c.send_throw(*sys.exc_info())
        t = stackless.tasklet(helper)()
        result = c.receive()
        assert self._ran_AsTaskletTestCase_setUp
        return result

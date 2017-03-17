#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright (c) 2016 by Anselm Kruis
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

"""
Utility functions and classes
"""

from __future__ import absolute_import, print_function, division

import sys
import unittest
import re
import stackless
import weakref
import pickle
import inspect
import io
import contextlib
import gc
from test.support import run_unittest

# emit warnings about uncollectable objects
gc.set_debug(gc.DEBUG_UNCOLLECTABLE)
try:
    long  # @UndefinedVariable
except NameError:
    long = int  # @ReservedAssignment

try:
    import threading
    withThreads = True
except:
    withThreads = False


@contextlib.contextmanager
def captured_stderr():
    old = sys.stderr
    new = io.StringIO()
    sys.stderr = new
    try:
        yield new
    finally:
        sys.stderr = old


def require_one_thread(testcase):
    if withThreads:
        return unittest.skipIf(threading.active_count() > 1, "Test requires, that only a single thread is active")(testcase)
    return testcase


def get_current_watchdog_list():
    # The second argument of get_thread_info() is intentionally undocumented.
    # See C source.
    watchdog_list = stackless.get_thread_info(-1, 1 << 31)[3]
    if isinstance(watchdog_list, list):
        return watchdog_list
    # The watchdog list has not been created. Force its creation.
    # First clear the scheduler. We do not want to run any tasklets
    scheduled = []
    t = stackless.current.next
    while t not in (None, stackless.current):
        scheduled.append(t)
        t = t.next
    for t in scheduled:
        t.remove()
    try:
        stackless.run()  # creates the watchdog list
    finally:
        for t in scheduled:
            t.insert()
        if scheduled:
            assert stackless.current.next == scheduled[0]
    watchdog_list = stackless.get_thread_info(-1, 1 << 31)[3]
    assert isinstance(watchdog_list, list)
    return watchdog_list


class StacklessTestCaseMixin(object):
    def skipUnlessSoftswitching(self):
        if not stackless.enable_softswitch(None):
            self.skipTest("test requires soft-switching")

    def skipIfSoftswitching(self):
        if stackless.enable_softswitch(None):
            self.skipTest("test requires hard-switching")

    def assertCallableWith0Args(self, func, additionalArg=None):
        self.assertRaisesRegex(TypeError, r"takes no arguments|expected 0 arguments", func, additionalArg)

    _NO_KWARGS_RE = r"takes no keyword arguments"
    _NO_KWARGS_RE = re.compile(_NO_KWARGS_RE)

    def checkSignatureArbitraryArgsAndKw(self, func, nb_mandatory, *namesAndArgs):
        for r in self._checkSignature(func, nb_mandatory, "both", None, *namesAndArgs):
            yield r

    def checkSignatureArbitraryArgs(self, func, nb_mandatory, *namesAndArgs):
        for r in self._checkSignature(func, nb_mandatory, "args", None, *namesAndArgs):
            yield r

    def checkSignatureArbitraryKw(self, func, nb_mandatory, *namesAndArgs):
        for r in self._checkSignature(func, nb_mandatory, "kw", None, *namesAndArgs):
            yield r

    def checkSignatureNamedArgs(self, func, nb_mandatory, additionalArg, *namesAndArgs):
        for r in self._checkSignature(func, nb_mandatory, False, additionalArg, *namesAndArgs):
            yield r

    def _checkSignature(self, func, nb_mandatory, accept_arbitrary, additionalArg, *namesAndArgs):
        # check arguments and build the argument list
        self.assertIsInstance(nb_mandatory, int)
        self.assertGreaterEqual(nb_mandatory, 0)
        self.assertGreaterEqual(len(namesAndArgs) / 2, nb_mandatory)
        if not accept_arbitrary:
            self.assertGreaterEqual(len(namesAndArgs), 2)
            self.assertTrue(len(namesAndArgs) % 2 == 0)
            namesAndArgs_iter = iter(namesAndArgs)
        else:
            namesAndArgs_iter = iter(namesAndArgs[:nb_mandatory * 2])
        args = []
        names = []
        kwargs = {}
        for n in namesAndArgs_iter:
            v = (getattr(namesAndArgs_iter, "next", None) or namesAndArgs_iter.__next__)()
            self.assertIsInstance(n, str)
            args.append(v)
            names.append(n)
            kwargs[n] = v
        if accept_arbitrary:
            for i, v in enumerate(namesAndArgs[nb_mandatory * 2:]):
                n = "_arbitrary_nonsens_arg{0}_".format(i)
                args.append(v)
                names.append(n)
                kwargs[n] = v

        # try a call with args and additional arg
        exc_msg = r"(takes|expected) at most {0} argument".format(len(args))
        if len(args) == nb_mandatory:
            if nb_mandatory == 1:
                exc_msg = r"takes exactly one argument"
            else:
                pass
        if not accept_arbitrary:
            self.assertRaisesRegex(TypeError, exc_msg,
                                   func, *(args + [additionalArg]))
        if nb_mandatory > 0:
            exc_msg = r"(takes|expected) at least {0} argument|Required argument '{1}' \(pos 1\) not found".format(nb_mandatory, names[0])
            if len(args) == nb_mandatory:
                if nb_mandatory == 1:
                    exc_msg = r"takes exactly one argument"
                else:
                    pass
            self.assertRaisesRegex(TypeError, exc_msg, func)
        # only required positional arguments
        yield func(*(args[:nb_mandatory]))
        # all positional arguments
        if accept_arbitrary != "kw":
            yield func(*args)
        # try a call with kwargs
        try:
            # all arguments as kwargs
            if accept_arbitrary != "args":
                yield func(**kwargs)
            else:
                # only required args as kw-args
                yield func(**dict((k, kwargs[k]) for k in names[:nb_mandatory]))
                return
        except TypeError as e:
            if not self._NO_KWARGS_RE.search(str(e)):
                raise
            return
        else:
            # only required args as kw-args
            yield func(**dict((k, kwargs[k]) for k in names[:nb_mandatory]))


# call the class method prepare_test_methods(cls) after creating the
# class. This method can be used to modify the newly created class
class StacklessTestCaseMeta(type):
    def __init__(cls, *args, **kw):
        cls.prepare_test_methods()


class StacklessTestCase(unittest.TestCase, StacklessTestCaseMixin, metaclass=StacklessTestCaseMeta):

    @classmethod
    def prepare_test_method(cls, func, name):
        """Called after class creation

        This method creates the _H methods, which run without
        soft switching
        """
        if hasattr(func, "enable_softswitch") and not getattr(func, "_H_created", False):
            return ((func, name), )

        def wrapper_hardswitch(self, method=func):
            self.assertTrue(self.__setup_called, "Broken test case: it didn't call super(..., self).setUp()")
            self.assertFalse(stackless.enable_softswitch(None), "softswitch is enabled")
            return method(self)
        wrapper_hardswitch.enable_softswitch = False
        wrapper_hardswitch.__name__ = name + "_H"
        if func.__doc__:
            doc = func.__doc__
            if doc.startswith("(soft) "):
                doc = doc[7:]
            wrapper_hardswitch.__doc__ = "(hard) " + doc
        setattr(cls, wrapper_hardswitch.__name__, wrapper_hardswitch)

        if not hasattr(func, "_H_created"):
            func._H_created = True
            func.enable_softswitch = True
            if func.__doc__:
                func.__doc__ = "(soft) " + func.__doc__
        return ((func, name), (wrapper_hardswitch, wrapper_hardswitch.__name__))

    @classmethod
    def prepare_pickle_test_method(cls, func, name=None):
        """Called after class creation

        This method creates the Py0...n C0...n methods, which run with
        the Python or C implementation of the enumerated pickle protocol.

        This method also acts as a method decorator.
        """
        if name is None:
            # used as a decorator
            func.prepare = cls.prepare_pickle_test_method
            return func

        if hasattr(func, "_pickle_created"):
            return StacklessTestCase.prepare_test_method.__func__(cls, func, name)
        setattr(cls, name, None)
        r = []
        for i in range(0, pickle.HIGHEST_PROTOCOL + 1):
            for p_letter in ("C", "P"):
                def test(self, method=func, proto=i, pickle_module=p_letter, unpickle_module=p_letter):
                    self.assertTrue(self._StacklessTestCase__setup_called, "Broken test case: it didn't call super(..., self).setUp()")
                    self._pickle_protocol = proto
                    self._pickle_module = pickle_module
                    self._unpickle_module = unpickle_module
                    return method(self)
                if i == 0 and p_letter == "C":
                    test.__name__ = name
                else:
                    test.__name__ = "{:s}_{:s}{:d}".format(name, p_letter, i)
                test._pickle_created = True
                if func.__doc__:
                    doc = func.__doc__
                    match = re.match(r"\([PC][0-{:d}]\)".format(pickle.HIGHEST_PROTOCOL), doc)
                    if match:
                        doc = match.string[match.end():]
                    test.__doc__ = "({:s}{:d}) {:s}".format(p_letter, i, doc)
                setattr(cls, test.__name__, test)
                r.extend(StacklessTestCase.prepare_test_method.__func__(cls, test, test.__name__))
        return r

    @classmethod
    def prepare_test_methods(cls):
        """Called after class creation

        This method creates the _H methods, which run without
        soft switching
        """
        names = unittest.defaultTestLoader.getTestCaseNames(cls)
        for n in names:
            m = getattr(cls, n)
            if inspect.ismethod(m):
                m = m.__func__
            prepare = getattr(m, "prepare", cls.prepare_test_method)
            for x in prepare.__func__(cls, m, n):
                pass

    __setup_called = False
    __preexisting_threads = None
    __active_test_cases = weakref.WeakValueDictionary()

    def __init__(self, methodName='runTest'):
        # set the __enable_softswitch flag based on methodName
        try:
            m = getattr(self, methodName)
        except Exception:
            self.__enable_softswitch = None
        else:
            self.__enable_softswitch = getattr(m, "enable_softswitch", None)

        super(StacklessTestCase, self).__init__(methodName=methodName)

    # 2.7 lacks assertRaisesRegex
    try:
        unittest.TestCase.assertRaisesRegex  # @UndefinedVariable
    except AttributeError:
        assertRaisesRegex = unittest.TestCase.assertRaisesRegexp

    def setUpStacklessTestCase(self):
        """Initialisation

        This method must be called from :meth:`setUp`.
        """
        self._StacklessTestCase__setup_called = True
        self.addCleanup(stackless.enable_softswitch, stackless.enable_softswitch(self.__enable_softswitch))

        self.__active_test_cases[id(self)] = self
        if withThreads and self.__preexisting_threads is None:
            self.__preexisting_threads = frozenset(threading.enumerate())
            return len(self.__preexisting_threads)
        return 1

    def setUp(self):
        self.assertEqual(stackless.getruncount(), 1, "Leakage from other tests, with %d tasklets still in the scheduler" % (stackless.getruncount() - 1))
        expected_thread_count = self.setUpStacklessTestCase()
        if withThreads:
            active_count = threading.active_count()
            self.assertEqual(active_count, expected_thread_count, "Leakage from other threads, with %d threads running (%d expected)" % (active_count, expected_thread_count))

    def tearDown(self):
        # Tasklets created in pickling tests can be left in the scheduler when they finish.  We can feel free to
        # clean them up for the tests.  Any tests that expect to exit with no leaked tasklets should do explicit
        # assertions to check.
        self.assertTrue(self.__setup_called, "Broken test case: it didn't call super(..., self).setUp()")
        self.__setup_called = False
        mainTasklet = stackless.getmain()
        current = mainTasklet.next
        while current is not None and current is not mainTasklet:
            next_ = current.next
            current.kill()
            current = next_
        run_count = stackless.getruncount()
        self.assertEqual(run_count, 1, "Leakage from this test, with %d tasklets still in the scheduler" % (run_count - 1))
        if withThreads:
            preexisting_threads = self.__preexisting_threads
            self.__preexisting_threads = None  # avoid pickling problems, see _addSkip
            expected_thread_count = len(preexisting_threads)
            active_count = threading.active_count()
            if active_count > expected_thread_count:
                activeThreads = set(threading.enumerate())
                activeThreads -= preexisting_threads
                self.assertNotIn(threading.current_thread(), activeThreads, "tearDown runs on the wrong thread.")
                while activeThreads:
                    activeThreads.pop().join(0.5)
                active_count = threading.active_count()
            self.assertEqual(active_count, expected_thread_count, "Leakage from other threads, with %d threads running (%d expected)" % (active_count, expected_thread_count))
        gc.collect()  # emits warnings about uncollectable objects after each test

    def dumps(self, obj, protocol=None, *, fix_imports=True):
        if self._pickle_module == "P":
            return pickle._dumps(obj, protocol=protocol, fix_imports=fix_imports)
        elif self._pickle_module == "C":
            return pickle.dumps(obj, protocol=protocol, fix_imports=fix_imports)
        raise ValueError("Invalid pickle module")

    def loads(self, s, *, fix_imports=True, encoding="ASCII", errors="strict"):
        if self._pickle_module == "P":
            return pickle._loads(s, fix_imports=fix_imports, encoding=encoding, errors=errors)
        elif self._pickle_module == "C":
            return pickle.loads(s, fix_imports=fix_imports, encoding=encoding, errors=errors)
        raise ValueError("Invalid pickle module")

    # limited pickling support for test cases
    # Between setUp() and tearDown() the test-case has a
    # working __reduce__ method. Later the test case gets pickled by
    # value. However the test case is no longer functional.

    @property
    def __reduce__(self):
        if not self.__setup_called:
            # don't pickle by reduce
            return None

        def r():
            return (restore_testcase_from_id, (id(self),))
        return r

    @classmethod
    def restore_testcase_from_id(cls, id_):
        test_case = cls.__active_test_cases.get(id_)
        if test_case is None:
            raise pickle.UnpicklingError("the test case no longer exists")
        return test_case

    def __strip_attributes(self):
        # Remove non standard attributes. They could render the test case object unpickleable.
        # This is a hack, but it works fairly well.
        for k in list(self.__dict__.keys()):
            if k not in self.SAFE_TESTCASE_ATTRIBUTES and \
                    not isinstance(self.__dict__[k], (type(None), type(u""), type(b""), int, long, float)):
                del self.__dict__[k]

    if sys.hexversion >= 0x3040000:
        def _addSkip(self, result, test_case, reason):
            self.addCleanup(self.__strip_attributes)
            if hasattr(self, "_ran_AsTaskletTestCase_setUp"):
                # avoid a assertion violation in AsTaskletTestCase.run
                self._ran_AsTaskletTestCase_setUp = True
            super(StacklessTestCase, self)._addSkip(result, test_case, reason)
    else:
        def _addSkip(self, result, reason):
            self.addCleanup(self.__strip_attributes)
            if hasattr(self, "_ran_AsTaskletTestCase_setUp"):
                # avoid a assertion violation in AsTaskletTestCase.run
                self._ran_AsTaskletTestCase_setUp = True
            super(StacklessTestCase, self)._addSkip(result, reason)

_tc = StacklessTestCase(methodName='run')
try:
    _tc.setUp()
    StacklessTestCase.SAFE_TESTCASE_ATTRIBUTES = _tc.__dict__.keys()
finally:
    del _tc


class StacklessPickleTestCase(StacklessTestCase):
    """A test case class for pickle tests"""

    @classmethod
    def prepare_test_method(cls, func, name):
        return cls.prepare_pickle_test_method(func, name)


def restore_testcase_from_id(id_):
    return StacklessTestCase.restore_testcase_from_id(id_)


class AsTaskletTestCase(StacklessTestCase):
    """A test case class, that runs tests as tasklets"""

    def setUp(self):
        self._ran_AsTaskletTestCase_setUp = True
        if stackless.enable_softswitch(None):
            self.assertEqual(stackless.current.nesting_level, 0)

        # yes, its intended: call setUp on the grand parent class
        super(StacklessTestCase, self).setUp()
        expected_thread_count = self.setUpStacklessTestCase()
        self.assertEqual(stackless.getruncount(
        ), 1, "Leakage from other tests, with %d tasklets still in the scheduler" % (stackless.getruncount() - 1))
        if withThreads:
            active_count = threading.active_count()
            self.assertEqual(active_count, expected_thread_count, "Leakage from other threads, with %d threads running (%d expected)" % (active_count, expected_thread_count))

    def run(self, result=None):
        c = stackless.channel()
        c.preference = 1  # sender priority
        self._ran_AsTaskletTestCase_setUp = False

        def helper():
            try:
                c.send(super(AsTaskletTestCase, self).run(result))
            except:
                c.send_throw(*sys.exc_info())
        stackless.tasklet(helper)()
        result = c.receive()
        assert self._ran_AsTaskletTestCase_setUp
        return result


def test_main():
    """Main function for the CPython :mod:`test.regrtest` test driver.

    Import this function into your test_ module. It uses introspection
    to find the module name and run your tests.
    """
    stack = inspect.stack(0)
    for i in range(1, 5):
        try:
            the_module = stack[i][0].f_locals["the_module"]
            break
        except KeyError:
            pass
    else:
        raise RuntimeError("can't find local variable 'the_module'")

    test_suite = unittest.TestLoader().loadTestsFromModule(the_module)
    return run_unittest(test_suite)

import unittest
import gc

from support import StacklessTestCase


def f(): yield 1

class TestGarbageCollection(StacklessTestCase):
    def testSimpleLeakage(self):
        leakage = []

        gc.collect(2)
        before  = set(id(o) for o in gc.get_objects())

        for i in f():
            pass

        gc.collect(2)
        after = gc.get_objects()

        for x in after:
            if x is not before and id(x) not in before:
                leakage.append(x)

        try:
            __in_psyco__
            relevant = False
        except NameError:
            relevant = True
        if relevant and len(leakage):
            self.assertTrue(len(leakage) == 0, "Leaked %s" % repr(leakage))

if __name__ == '__main__':
    unittest.main()

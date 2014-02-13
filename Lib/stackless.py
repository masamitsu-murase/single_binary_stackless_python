"""
stackless
This is the stackless module
"""
import sys
import types

import _stackless
from _stackless import *
# various deugging things starting with underscore
from _stackless import _test_nostacklesscall, _pickle_moduledict, _gc_track, _gc_untrack
try:
    from _stackless import _peek  # defined if compiled with STACKLESS_SPY
except ImportError: pass
try:
    from _stackless import _get_refinfo, _get_all_objects  # defined if compiled with Py_TRACE_REFS
except ImportError: pass

# To support old pickles and the unittests, see test_pickle.testXrange
from _stackless import _wrap
sys.modules["stackless._wrap"] = _wrap

def __reduce__():
    return "stackless"
def __reduce_ex_(*args):
    return "stackless"

def transmogrify():
    """
    this function creates a subclass of the ModuleType with properties.
    Stackless has historically had module properties, something very unusual in Python.
    We need to do that by replacing the current module object as it is being created
    """

    class StacklessModuleType(types.ModuleType):

        def current(self):
            return self.getcurrent()
        current = property(current, doc=getcurrent.__doc__)

        def runcount(self):
            return self.getruncount()
        runcount = property(runcount, doc=getruncount.__doc__)

        def main(self):
            return self.getmain()
        main = property(main, doc=getmain.__doc__)

        def debug(self):
            return self.getdebug()
        debug = property(debug, doc=getdebug.__doc__)

        def uncollectables(self):
            return self.getuncollectables()
        uncollectables = property(uncollectables, doc=getuncollectables);

        def threads(self):
            return self.getthreads()
        threads = property(threads, doc=getthreads.__doc__)

        # Pass module level flags into the builtin
        @property
        def pickle_with_tracing_state(self):
            """Will tasklet save their tracing state when pickled?"""
            return self._stackless.pickle_with_tracing_state
        @pickle_with_tracing_state.setter
        def pickle_with_tracing_state(self, val):
            self._stackless.pickle_with_tracing_state = val

    m = StacklessModuleType("stackless", __doc__)
    m.__dict__.update(globals())
    del m.transmogrify, m.types, m.sys

    # odd curiosity, the stackless module contains a reference to itself
    # deprectated, of course
    m.stackless = m

    sys.modules["stackless"] = m

transmogrify()

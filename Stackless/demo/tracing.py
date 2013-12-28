from stackless import *
import types
import traceback


def _tasklet__repr__(self):
    try:
        return "<tasklet %s>" % ("main" if self.is_main else self.name,)
    except AttributeError:
        return super(tasklet, self).__repr__()
tasklet.__repr__ = _tasklet__repr__


class NamedTasklet(tasklet):
    __slots__ = ["name"]

    def __new__(self, func, name=None):
        t = tasklet.__new__(self, func)
        if name is None:
            name = "at %08x" % (id(t))
        t.name = name
        return t


class Mutex(object):
    def __init__(self, capacity=1):
        self.queue = channel()
        self.capacity = capacity

    def isLocked(self):
        '''return non-zero if locked'''
        return self.capacity == 0

    def lock(self):
        '''acquire the lock'''
        currentTasklet = stackless.getcurrent()
        atomic = currentTasklet.set_atomic(True)
        try:
            if self.capacity:
                self.capacity -= 1
            else:
                self.queue.receive()
        finally:
            currentTasklet.set_atomic(atomic)

    def unlock(self):
        '''release the lock'''
        currentTasklet = stackless.getcurrent()
        atomic = currentTasklet.set_atomic(True)
        try:
            if self.queue.balance < 0:
                self.queue.send(None)
            else:
                self.capacity += 1
        finally:
            currentTasklet.set_atomic(atomic)

m = Mutex()


def task():
    name = getcurrent().name
    print name, "acquiring"
    m.lock()
    print name, "switching"
    schedule()
    print name, "releasing"
    m.unlock()


def trace_function(frame, event, arg):
    if frame.f_code.co_name in ('schedule_cb', 'channel_cb'):
        return None
    print "         trace_function: %s %s in %s, line %s" % \
        (stackless.current, event, frame.f_code.co_name, frame.f_lineno)
    if event in ('call', 'line', 'exception'):
        return trace_function
    return None


def channel_cb(channel, tasklet, sending, willblock):
    tf = tasklet.trace_function
    try:
        tasklet.trace_function = None
        print "Channel CB, tasklet %r, %s%s" % \
        (tasklet, ("recv", "send")[sending], ("", " will block")[willblock])
    finally:
        tasklet.trace_function = tf


def schedule_cb(prev, next):
    current = stackless.getcurrent()
    current_tf = current.trace_function
    try:
        current.trace_function = None
        current_info = "Schedule CB, current %r, " % (current,)
        if current_tf is None:
            # also look at the previous frame, in case this callback is exempt
            # from tracing
            f_back = current.frame.f_back
            if f_back is not None:
                current_tf = f_back.f_trace

        if not prev:
            print "%sstarting %r" % (current_info, next)
        elif not next:
            print "%sending %r" % (current_info, prev)
        else:
            print "%sjumping from %s to %s" % (current_info, prev, next)
        prev_tf = current_tf if prev is current else prev.trace_function
        next_tf = current_tf if next is current else next.trace_function
        print "    Current trace functions: prev: %r, next: %r" % \
            (prev_tf, next_tf)
        if next is not None:
            if not next.is_main:
                tf = trace_function
            else:
                tf = None
            print "    Setting trace function for next: %r" % (tf,)
            task = next.frame
            if next is current:
                task = task.f_back
            while task is not None:
                if isinstance(task, types.FrameType):
                    task.f_trace = tf
                task = task.f_back
            next.trace_function = tf
    except:
        traceback.print_exc()
    finally:
        current.trace_function = current_tf

if __name__ == "__main__":
    set_channel_callback(channel_cb)
    set_schedule_callback(schedule_cb)

    NamedTasklet(task, "tick")()
    NamedTasklet(task, "trick")()
    NamedTasklet(task, "track")()

    run()

    set_channel_callback(None)
    set_schedule_callback(None)

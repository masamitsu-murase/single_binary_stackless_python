from __future__ import absolute_import, print_function

import sys
import stackless
import traceback


class NamedTasklet(stackless.tasklet):
    __slots__ = ("name",)

    def __init__(self, func, name=None):
        stackless.tasklet.__init__(self, func)
        if name is None:
            name = "at %08x" % (id(self))
        self.name = name

    def __repr__(self):
        return "<tasklet %s>" % (self.name)


class Mutex(object):

    def __init__(self, capacity=1):
        self.queue = stackless.channel()
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
    name = stackless.getcurrent().name
    print(name, "acquiring")
    m.lock()
    print(name, "switching")
    stackless.schedule()
    print(name, "releasing")
    m.unlock()


def trace_function(frame, event, arg):
    if frame.f_code.co_name in ('schedule_cb', 'channel_cb'):
        return None
    print("         trace_function: %s %s in %s, line %s" %
          (stackless.current, event, frame.f_code.co_name, frame.f_lineno))
    if event in ('call', 'line', 'exception'):
        return trace_function
    return None


def channel_cb(channel, tasklet, sending, willblock):
    tf = tasklet.trace_function
    try:
        tasklet.trace_function = None
        print("Channel CB, tasklet %r, %s%s" %
              (tasklet, ("recv", "send")[sending], ("", " will block")[willblock]))
    finally:
        tasklet.trace_function = tf


def schedule_cb(prev, next):
    # During a tasklet switch (during the execution of this function) the
    # the result of stackless.getcurrent() is implementation defined.
    # Therefore this function avoids any assumptions about the current tasklet.
    current_tf = sys.gettrace()
    try:
        sys.settrace(None)  # don't trace this callback
        current_frame = sys._getframe()
        if current_tf is None:
            # also look at the previous frame, in case this callback is exempt
            # from tracing
            f_back = current_frame.f_back
            if f_back is not None:
                current_tf = f_back.f_trace

        current_info = "Schedule CB "
        if not prev:
            print("%sstarting %r" % (current_info, next))
        elif not next:
            print("%sending %r" % (current_info, prev))
        else:
            print("%sjumping from %s to %s" % (current_info, prev, next))

        # Inform about the installed trace functions
        prev_tf = current_tf if prev.frame is current_frame else prev.trace_function
        next_tf = current_tf if next.frame is current_frame else next.trace_function
        print("    Current trace functions: prev: %r, next: %r" % (prev_tf, next_tf))

        # Eventually set a trace function
        if next is not None:
            if not next.is_main:
                tf = trace_function
            else:
                tf = None
            print("    Setting trace function for next: %r" % (tf,))
            # Set the "global" trace function for the tasklet
            next.trace_function = tf
            # Set the "local" trace function for each frame
            # This is required, if the tasklet is already running
            frame = next.frame
            if frame is current_frame:
                frame = frame.f_back
            while frame is not None:
                frame.f_trace = tf
                frame = frame.f_back
    except:
        traceback.print_exc()
    finally:
        sys.settrace(current_tf)

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == 'hard':
        stackless.enable_softswitch(False)

    stackless.set_channel_callback(channel_cb)
    stackless.set_schedule_callback(schedule_cb)

    NamedTasklet(task, "tick")()
    NamedTasklet(task, "trick")()
    NamedTasklet(task, "track")()

    stackless.run()

    stackless.set_channel_callback(None)
    stackless.set_schedule_callback(None)

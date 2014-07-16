#
# -*- coding: utf-8 -*-
#
# Created by ? (Christian?)
# Updated for changes to the Stackless atomicity by N. J. Harman.
#

from __future__ import absolute_import, print_function, division, unicode_literals
import stackless


class mutex:
    def __init__(self, capacity=1):
        self.queue = stackless.channel()
        self.capacity = capacity

    def isLocked(self):
        '''return non-zero if locked'''
        return self.capacity == 0

    def lock(self):
        '''acquire the lock'''
        with stackless.atomic():
            if self.capacity:
                self.capacity -= 1
            else:
                self.queue.receive()

    def unlock(self):
        '''release the lock'''
        with stackless.atomic():
            if self.queue.balance < 0:
                self.queue.send(None)
            else:
                self.capacity += 1


class NamedTasklet(stackless.tasklet):
    __slots__ = ["name"]

    def __init__(self, func, name=None):
        super(NamedTasklet, self).__init__(func)
        if not name:
            name = "at %08x" % (id(self))
        self.name = name

    def __repr__(self):
        return "<tasklet %s>" % getattr(self, 'name', id(self))


def channel_cb(channel, tasklet, sending, willblock):
    print(tasklet, ("recv", "send")[sending], ("nonblocking", "blocking")[willblock])


def schedule_cb(prev, next):
    if not prev:
        print("starting", next)
    elif not next:
        print("ending", prev)
    else:
        print("jumping from %s to %s" % (prev, next))
        next.trace_function = prev.trace_function


def f(m):
    name = stackless.getcurrent().name
    print(name, "acquiring")
    m.lock()
    print(name, "switching")
    stackless.schedule()
    print(name, "releasing")
    m.unlock()


def main(debug=False):
    m = mutex()

    NamedTasklet(f, name="tick")(m)
    NamedTasklet(f, name="trick")(m)
    NamedTasklet(f, name="track")(m)

    if debug:
        stackless.set_channel_callback(channel_cb)
        stackless.set_schedule_callback(schedule_cb)

    stackless.run()

    stackless.set_channel_callback(None)
    stackless.set_schedule_callback(None)


if __name__ == "__main__":
    import sys
    main(debug="--debug" in sys.argv)

.. highlightlang:: c

.. comment: affected files: data\refcounts.dat
.. comment: affected files: c-api\stackless.rst
.. comment: to do: link c-api\stackless.rst in somewhere
.. comment: to do: generate new docs

Tasklets
--------

.. cfunction:: PyTaskletObject *PyTasklet_New(PyTypeObject *type, PyObject *func)

  Return a new tasklet object.  *type* must be derived from :ctype:`PyTasklet_Type`
  or *NULL*.  *func* must be a callable object (normal use-case) or *NULL*, if the
  tasklet is being used via capture().

.. todo: in the case where NULL is returned and slp_ensure_linkage fails no
   exception is set, which is in contrast elsewhere in the function.

.. cfunction:: int PyTasklet_Setup(PyTaskletObject *task, PyObject *args, PyObject *kwds)
  
  Binds a tasklet function to parameters, making it ready to run and inserts in
  into the runnables queue.  Returns ``0`` if successful or ``-1`` in the case of failure.

.. cfunction:: int PyTasklet_Run(PyTaskletObject *task)

  Forces *task* to run immediately.  Returns ``0`` if successful, and ``-1`` in the
  case of failure.

.. cfunction:: int PyTasklet_Run_nr(PyTaskletObject *task)

  Forces *task* to run immediately, soft switching if possible.  Returns ``1`` if
  the call soft switched, ``0`` if the call hard switched and -1 in the case of
  failure.

.. cfunction:: int PyTasklet_Remove(PyTaskletObject *task)

  Removes *task* from the runnables queue.  Be careful! If this tasklet has a C
  stack attached, you need to either resume running it or kill it.  Just dropping
  it might give an inconsistent system state.  Returns ``0`` if successful, and
  ``-1`` in the case of failure.

.. cfunction:: int PyTasklet_Insert(PyTaskletObject *task)

  Insert *task* into the runnables queue, if it isn't already there.   If it is
  blocked or dead, the function returns ``-1`` and a :exc:`RuntimeError` is raised.

.. cfunction:: PyObject *PyTasklet_Become(PyTaskletObject *self, PyObject *retval)

  Use of this API function is undocumented and unrecommended.

  .. deprecated:: 2.5
     Proved problematic in production use and are pending removal.

.. cfunction:: PyObject* PyTasklet_Capture(PyTaskletObject *self, PyObject *retval)

  Use of this API function is undocumented and unrecommended.

  .. deprecated:: 2.5
     Proved problematic in production use and are pending removal.

.. cfunction:: int PyTasklet_RaiseException(PyTaskletObject *self, PyObject *klass, PyObject *args)

  Raises an instance of the *klass* exception on the *self* tasklet.  *klass* must
  be a subclass of :exc:`Exception`.  Returns ``1`` if the call soft switched, ``0``
  if the call hard switched and ``-1`` in the case of failure.

  .. note:: Raising :exc:`TaskletExit` on a tasklet can be done to silently kill
     it, see :cfunc:`PyTasklet_Kill`.  

.. cfunction:: int PyTasklet_Kill(PyTaskletObject *self)
     
  Raises :exc:`TaskletExit` on tasklet *self*.  This should result in *task* being
  silently killed. Returns ``1`` if the call soft switched, ``0`` if the call hard
  switched and ``-1`` in the case of failure.

.. cfunction:: int PyTasklet_GetAtomic(PyTaskletObject *task)

  Returns ``1`` if *task* is atomic, otherwise ``0``.

.. cfunction:: int PyTasklet_SetAtomic(PyTaskletObject *task, int flag)
  
  Returns ``1`` if *task* is currently atomic, otherwise ``0``.  Sets the
  atomic attribute to the logical value of *flag*.

.. cfunction:: int PyTasklet_GetIgnoreNesting(PyTaskletObject *task)

  Returns ``1`` if *task* ignores its nesting level when choosing whether to
  auto-schedule it, otherwise ``0``.

.. cfunction:: int PyTasklet_SetIgnoreNesting(PyTaskletObject *task, int flag)

  Returns the existing value of the *ignore_nesting* attribute for the tasklet
  *task*, setting it to the logical value of *flag*.  If true, the tasklet may
  be auto-scheduled even if its *nesting_level* is > ``0``.

.. cfunction:: int PyTasklet_GetBlockTrap(PyTaskletObject *task)

  Returns ``1`` if *task* is designated as not being allowed to be blocked on a
  channel, otherwise ``0``.

.. cfunction:: void PyTasklet_SetBlockTrap(PyTaskletObject *task, int value)

  Returns ``1`` if *task* was already designated as not being allowed to be blocked
  on a channel, otherwise ``0``.  This attribute is set to the logical value of
  *value*.

.. cfunction:: PyObject *PyTasklet_GetFrame(PyTaskletObject *task)

  Returns the current frame that *task* is executing in, or *NULL*

.. cfunction:: int PyTasklet_IsMain(PyTaskletObject *task)

  Returns ``1`` if *task* is the main tasklet, otherwise ``0``.

.. cfunction:: int PyTasklet_IsCurrent(PyTaskletObject *task)

  Returns ``1`` if *task* is the current tasklet, otherwise ``0``.

.. cfunction:: int PyTasklet_GetRecursionDepth(PyTaskletObject *task)

  Return the current recursion depth of *task*.

.. cfunction:: int PyTasklet_GetNestingLevel(PyTaskletObject *task)

  Return the current nesting level of *task*.

.. cfunction:: int PyTasklet_Alive(PyTaskletObject *task)

  Returns ``1`` if *task* is alive (has an associated frame), otherwise
  ``0`` if it is dead.
  
.. cfunction:: int PyTasklet_Paused(PyTaskletObject *task)

  Returns ``1`` if *task* is paused, otherwise ``0``.  A tasklet is paused if it is
  alive, but not scheduled or blocked on a channel.

.. cfunction:: int PyTasklet_Scheduled(PyTaskletObject *task)

  Returns ``1`` if *task* is scheduled, otherwise ``0``.  In the context of this
  function a tasklet is considered to be scheduled if it is alive, and in the
  scheduler runnables list or blocked on a channel.

.. cfunction:: int PyTasklet_Restorable(PyTaskletObject *task)

  Returns ``1`` if *task* can be fully unpickled, otherwise ``0``.  A tasklet can
  be pickled whether it is fully restorable or not for the purposes of debugging
  and introspection.  A tasklet that has been hard-switched cannot be fully
  pickled, for instance.

Channels
--------

 .. cfunction:: PyChannelObject* PyChannel_New(PyTypeObject *type)

  Return a new channel object, or *NULL* in the case of failure.  *type* must be
  derived from :ctype:`PyChannel_Type` or be *NULL*, otherwise a :exc:`TypeError`
  is raised.

.. cfunction:: int PyChannel_Send(PyChannelObject *self, PyObject *arg)

  Send *arg* on the channel *self*.  Returns ``0`` if the operation was
  successful, or ``-1`` in the case of failure.

.. cfunction:: int PyChannel_Send_nr(PyChannelObject *self, PyObject *arg)

  Send *arg* on the channel *self*, soft switching if possible.  Returns ``1`` if
  the call soft switched, ``0`` if the call hard switched and -1 in the case of
  failure.

.. cfunction:: PyObject *PyChannel_Receive(PyChannelObject *self)

  Receive on the channel *self*.  Returns a |PY| object if the operation was
  successful, or *NULL* in the case of failure.

.. cfunction:: PyObject *PyChannel_Receive_nr(PyChannelObject *self)

  Receive on the channel *self*, soft switching if possible.  Returns a |PY|
  object if the operation was successful, :ctype:`Py_UnwindToken` if a soft switch
  occurred, or *NULL* in the case of failure.

.. cfunction:: int PyChannel_SendException(PyChannelObject *self, PyObject *klass, PyObject *value)

  Returns ``0`` if successful or ``-1`` in the case of failure.  An instance of the
  exception type *klass* is raised on the first tasklet blocked on channel *self*.

.. cfunction:: PyObject *PyChannel_GetQueue(PyChannelObject *self)

  Returns the first tasklet in the channel *self*'s queue, or *NULL* in the case
  the queue is empty.

.. cfunction:: void PyChannel_Close(PyChannelObject *self)

  Marks the channel *self* as closing.  No further tasklets can be blocked on the
  it from this point, unless it is later reopened.

.. cfunction:: void PyChannel_Open(PyChannelObject *self)

  Reopens the channel *self*.  This allows tasklets to once again send and receive
  on it, if those operations would otherwise block the given tasklet.

.. cfunction:: int PyChannel_GetClosing(PyChannelObject *self)

  Returns ``1`` if the channel *self* is marked as closing, otherwise ``0``.

.. cfunction:: int PyChannel_GetClosed(PyChannelObject *self)

  Returns ``1`` if the channel *self* is marked as closing and there are no tasklets
  blocked on it, otherwise ``0``.

.. cfunction:: int PyChannel_GetPreference(PyChannelObject *self)

  Returns the current scheduling preference value of *self*.  See
  :attr:`channel.preference`.

.. cfunction:: void PyChannel_SetPreference(PyChannelObject *self, int val)

  Sets the current scheduling preference value of *self*.  See
  :attr:`channel.preference`.

.. cfunction:: int PyChannel_GetScheduleAll(PyChannelObject *self)

  Gets the *schedule_all* override flag for *self*.  See
  :attr:`channel.schedule_all`.

.. cfunction:: void PyChannel_SetScheduleAll(PyChannelObject *self, int val)

  Sets the *schedule_all* override flag for *self*.  See
  :attr:`channel.schedule_all`.

.. cfunction:: int PyChannel_GetBalance(PyChannelObject *self)

  Gets the balance for *self*.  See :attr:`channel.balance`.

stackless module
----------------

.. cfunction:: PyObject *PyStackless_Schedule(PyObject *retval, int remove)

  Suspend the current tasklet and schedule the next one in the cyclic chain.
  if remove is nonzero, the current tasklet will be removed from the chain.
  retval = success  NULL = failure

.. cfunction:: PyObject *PyStackless_Schedule_nr(PyObject *retval, int remove)

  retval = success  NULL = failure
  retval == Py_UnwindToken: soft switched

 .. cfunction:: int PyStackless_GetRunCount()

  get the number of runnable tasks, including the current one.
  -1 = failure

.. cfunction:: PyObject *PyStackless_GetCurrent()

  Get the currently running tasklet, that is, "yourself".

.. cfunction:: PyObject *PyStackless_RunWatchdog(long timeout)

  Runs the scheduler until there are no tasklets remaining within it, or until
  one of the scheduled tasklets runs for *timeout* VM instructions without
  blocking.  Returns *None* if the scheduler is empty, a tasklet object if that
  tasklet timed out, or *NULL* in the case of failure.  If a timed out tasklet
  is returned, it should be killed or reinserted.

  This function can only be called from the main tasklet.
  During the run, main is suspended, but will be invoked
  after the action. You will write your exception handler
  here, since every uncaught exception will be directed
  to main.

.. cfunction:: PyObject *PyStackless_RunWatchdogEx(long timeout, int flags)

  Wraps :cfunc:`PyStackless_RunWatchdog`, but allows its behaviour to be
  customised by the value of *flags* which may contain any of the following
  bits:
  
  ``Py_WATCHDOG_THREADBLOCK``
     Allows a thread to block if it runs out of tasklets.  Ideally
     it will be awakened by other threads using channels which its
     blocked tasklets are waiting on.
  
  ``Py_WATCHDOG_SOFT``
     Instead of interrupting a tasklet, we wait until the
     next tasklet scheduling moment to return.  Always returns
     *Py_None*, as everything is in order.
  
  ``Py_WATCHDOG_IGNORE_NESTING``
     Allows interrupts at all levels, effectively acting as
     though the *ignore_nesting* attribute were set on all
     tasklets.
  
  ``Py_WATCHDOG_TIMEOUT``
     Interprets *timeout* as a fixed run time, rather than a
     per-tasklet run limit.  The function will then attempt to
     interrupt execution once this many total opcodes have
     been executed since the call was made.
     
debugging and monitoring functions
----------------------------------

.. cfunction:: int PyStackless_SetChannelCallback(PyObject *callable)

  channel debugging.  The callable will be called on every send or receive.
  Passing NULL removes the handler.
  Parameters of the callable:
  channel, tasklet, int sendflag, int willblock
  -1 = failure

.. cfunction:: int PyStackless_SetScheduleCallback(PyObject *callable)

  scheduler monitoring.
  The callable will be called on every scheduling.
  Passing NULL removes the handler.
  Parameters of the callable: from, to
  When a tasklet dies, to is None.
  After death or when main starts up, from is None.
  -1 = failure

.. cfunction:: void PyStackless_SetScheduleFastcallback(slp_schedule_hook_func func)

  Scheduler monitoring with a faster interface.

Interface functions
-------------------

Most of the above functions can be called both from "inside"
and "outside" stackless. "inside" means there should be a running
(c)frame on top which acts as the "main tasklet". The functions
do a check whether the main tasklet exists, and wrap themselves
if it is necessary.
The following routines are used to support this, and you may use
them as well if you need to make your specific functions always
available.

.. cfunction:: PyObject *PyStackless_Call_Main(PyObject *func, PyObject *args, PyObject *kwds)

  Run any callable as the "main" |PY| function.  Returns a |PY| object, or
  *NULL* in the case of failure.

.. cfunction:: PyObject *PyStackless_CallMethod_Main(PyObject *o, char *name, char *format, ...)

  Convenience: Run any method as the "main" |PY| function.  Wraps PyStackless_Call_Main.

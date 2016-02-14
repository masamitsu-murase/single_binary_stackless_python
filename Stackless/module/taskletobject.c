/******************************************************

  The Tasklet

 ******************************************************/

#include "Python.h"

#ifdef STACKLESS
#include "core/stackless_impl.h"

void
slp_current_insert(PyTaskletObject *task)
{
    PyThreadState *ts = task->cstate->tstate;
    PyTaskletObject **chain = &ts->st.current;
    assert(ts);

    SLP_CHAIN_INSERT(PyTaskletObject, chain, task, next, prev);
    ++ts->st.runcount;
}

void
slp_current_insert_after(PyTaskletObject *task)
{
    PyThreadState *ts = task->cstate->tstate;
    PyTaskletObject *hold = ts->st.current;
    PyTaskletObject **chain = &ts->st.current;
    assert(ts);

    *chain = hold->next;
    SLP_CHAIN_INSERT(PyTaskletObject, chain, task, next, prev);
    *chain = hold;
    ++ts->st.runcount;
}

void
slp_current_uninsert(PyTaskletObject *task)
{
    slp_current_remove_tasklet(task);
}

PyTaskletObject *
slp_current_remove(void)
{
    PyThreadState *ts = PyThreadState_GET();
    PyTaskletObject **chain = &ts->st.current, *ret;

    /* make sure tasklet belongs to this thread */
    assert((*chain)->cstate->tstate == ts);

    --ts->st.runcount;
    assert(ts->st.runcount >= 0);
    SLP_CHAIN_REMOVE(PyTaskletObject, chain, ret, next, prev);
    if (ts->st.runcount == 0)
        assert(ts->st.current == NULL);
    return ret;
}

void
slp_current_remove_tasklet(PyTaskletObject *task)
{
    PyThreadState *ts = task->cstate->tstate;
    PyTaskletObject **chain = &ts->st.current, *ret, *hold;

    --ts->st.runcount;
    assert(ts->st.runcount >= 0);
    hold = ts->st.current;
    ts->st.current = task;
    SLP_CHAIN_REMOVE(PyTaskletObject, chain, ret, next, prev);
    if (hold != task)
        ts->st.current = hold;
    if (ts->st.runcount == 0)
        assert(ts->st.current == NULL);
}

void
slp_current_unremove(PyTaskletObject* task)
{
    PyThreadState *ts = task->cstate->tstate;
    slp_current_insert(task);
    ts->st.current = task;
}

/*
 * Determine if a tasklet has C stack, and thus needs to
 * be switched to (killed) before it can be deleted.
 * Tasklets without C stack (in a soft switched state)
 * need only be released.
 * If a tasklet's thread has been killed, but the
 * tasklet still lingers, it has no restorable c state and may
 * as well be thrown away.  But this may of course cause
 * problems elsewhere (why didn't the tasklet die when instructed to?)
 */
static int
tasklet_has_c_stack(PyTaskletObject *t)
{
    return t->f.frame && t->cstate && t->cstate->tstate && t->cstate->nesting_level != 0 ;
}

static int
tasklet_traverse(PyTaskletObject *t, visitproc visit, void *arg)
{
    PyFrameObject *f;
    PyThreadState *ts = PyThreadState_GET();
    if (t->cstate->tstate) {
        /* its thread is alive */
        if (ts != t->cstate->tstate)
            /* can't collect from this thread! */
            PyObject_GC_Collectable((PyObject *)t, visit, arg, 0);
    }

    /* we own the "execute reference" of all the frames */
    for (f = t->f.frame; f != NULL; f = f->f_back) {
        Py_VISIT(f);
    }
    Py_VISIT(t->tempval);
    Py_VISIT(t->cstate);
    return 0;
}

static void
tasklet_clear_frames(PyTaskletObject *t)
{
    /* release frame chain, we own the "execute reference" of all the frames */
    PyFrameObject *f;
    f = t->f.frame;
    t->f.frame = NULL;
    while (f != NULL) {
        PyFrameObject *tmp = f->f_back;
        f->f_back = NULL;
        Py_DECREF(f);
        f = tmp;
    }
}

/*
 * the following function tries to ensure that a tasklet is
 * really killed. It is called in a context where we can't
 * afford that it will not be dead afterwards.
 * Reason: When clearing or resurrecting and killing, the
 * tasklet is in fact already dead, and the only case that
 * could revive it was that __del_ was defined.
 * But in the context of __del__, we can't do anything but rely
 * on proper destruction, since nobody will listen to an exception.
 */

static void
kill_finally (PyObject *ob)
{
    PyThreadState *ts = PyThreadState_GET();
    PyTaskletObject *self = (PyTaskletObject *) ob;
    int is_mine = ts == self->cstate->tstate;

    /* this could happen if we have a refcount bug, so catch it here.
    assert(self != ts->st.current);
    It also gets triggered on interpreter exit when we kill the tasks
    with stacks (PyStackless_kill_tasks_with_stacks) and there is no
    way to differentiate that case.. so it just gets commented out.
    */

    self->flags.is_zombie = 1;
    while (self->f.frame != NULL) {
        PyTasklet_Kill(self);
        if (!is_mine)
            return; /* will be killed elsewhere */
    }
}


/* destructing a tasklet without destroying it */

static void
tasklet_clear(PyTaskletObject *t)
{
    /* if (slp_get_frame(t) != NULL) */
    if (t->f.frame != NULL)
        kill_finally((PyObject *) t);
    TASKLET_SETVAL(t, Py_None); /* always non-zero */
    /* unlink task from cstate */
    if (t->cstate != NULL && t->cstate->task == t)
        t->cstate->task = NULL;
}


static void
tasklet_dealloc(PyTaskletObject *t)
{
    PyObject_GC_UnTrack(t);
    if (t->f.frame != NULL) {
        /*
         * we want to cleanly kill the tasklet in the case it
         * was forgotten. One way would be to resurrect it,
         * but this is quite ugly with many ifdefs, see
         * classobject/typeobject.
         * Well, we do it.
         */
        if (slp_resurrect_and_kill((PyObject *) t, kill_finally)) {
            /* the beast has grown new references */
            PyObject_GC_Track(t);
            return;
        }
    }
    if (t->tsk_weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *)t);
    assert(t->f.frame == NULL);
    if (t->cstate != NULL) {
        assert(t->cstate->task != t || Py_SIZE(t->cstate) == 0);
        Py_DECREF(t->cstate);
    }
    Py_DECREF(t->tempval);
    Py_XDECREF(t->def_globals);
    Py_TYPE(t)->tp_free((PyObject*)t);
}

PyTaskletObject *
PyTasklet_New(PyTypeObject *type, PyObject *func)
{
    if (!PyType_IsSubtype(type, &PyTasklet_Type)) {
        PyErr_SetNone(PyExc_TypeError);
        return NULL;
    }
    if (func)
        return (PyTaskletObject*)PyObject_CallFunctionObjArgs((PyObject*)type, func, NULL);
    else
        return (PyTaskletObject*)PyObject_CallFunction((PyObject*)type, NULL);
}

static int
impl_tasklet_setup(PyTaskletObject *task, PyObject *args, PyObject *kwds, int insert);

int
PyTasklet_BindEx(PyTaskletObject *task, PyObject *func, PyObject *args, PyObject *kwargs)
{
    PyThreadState *ts = task->cstate->tstate;
    if (func == Py_None)
        func = NULL;
    if (args == Py_None)
        args = NULL;
    if (kwargs == Py_None)
        kwargs = NULL;
    
    if (func != NULL && !PyCallable_Check(func))
        TYPE_ERROR("tasklet function must be a callable or None", -1);
    if (ts && ts->st.current == task) {
        RUNTIME_ERROR("can't (re)bind the current tasklet", -1);
    }
    if (PyTasklet_Scheduled(task)) {
        RUNTIME_ERROR("tasklet is scheduled", -1);
    }
    if (tasklet_has_c_stack(task)) {
        RUNTIME_ERROR("tasklet has C state on its stack", -1);
    }

    tasklet_clear_frames(task);
    assert(task->f.frame == NULL);

    /* cstate is set by bind_tasklet_to_frame() later on */

    if ( args == NULL && kwargs == NULL) {
        /* just binding or unbinding the function */
        if (func == NULL)
            func = Py_None;
        TASKLET_SETVAL(task, func);
    } else {
        /* adding arguments.  Absence of func means leave tmpval alone */
        PyObject *old = NULL;
        int result;
        if (func != NULL) {
            TASKLET_CLAIMVAL(task, &old);
            TASKLET_SETVAL(task, func);
        }
        if (args == NULL) {
            args = PyTuple_New(0);
            if (args == NULL)
                goto err;
        } else
            Py_INCREF(args);
        if (kwargs == NULL) {
            kwargs = PyDict_New();
            if (kwargs == NULL) {
                Py_DECREF(args);
                goto err;
            }
        } else
            Py_INCREF(kwargs);
        result = impl_tasklet_setup(task, args, kwargs, 0);
        Py_DECREF(args);
        Py_DECREF(kwargs);
        if (result)
            goto err;
        Py_XDECREF(old);
        return 0;
err:
        if (old != NULL)
            TASKLET_SETVAL_OWN(task, old);
        return -1;
    }
    return 0;
}

PyTaskletObject *
PyTasklet_Bind(PyTaskletObject *task, PyObject *func)
{
    if(PyTasklet_BindEx(task, func, NULL, NULL))
        return NULL;
    Py_INCREF(task);
    return task;
}


PyDoc_STRVAR(tasklet_bind__doc__,
"bind(func=None, args=None, kwargs=None)\n\
Binding a tasklet to a callable object, and arguments.\n\
The callable is usually passed in to the constructor.\n\
In some cases, it makes sense to be able to re-bind a tasklet,\n\
after it has been run, in order to keep its identity.\n\
This function can also be used, in place of setup() or __call__()\n\
to supply arguments to the bound function.  The difference is that\n\
this will not cause the tasklet to become runnable.\n\
If all the argument are None, this method unbinds the tasklet.\n\
Note that a tasklet can only be (un)bound if it doesn't have C-state\n\
and is not scheduled and is not the current tasklet.\
");

static PyObject *
tasklet_bind(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *func = Py_None;
    PyObject *fargs = Py_None;
    PyObject *fkwargs = Py_None;
    char *kwds[] = {"func", "args", "kwargs", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OOO:bind", kwds,
        &func, &fargs, &fkwargs))
        return NULL;
    if (PyTasklet_BindEx((PyTaskletObject *)self, func, fargs, fkwargs))
        return NULL;
    Py_INCREF(self);
    return self;
}

#define TASKLET_TUPLEFMT "iOiO"

static PyObject *
tasklet_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyThreadState *ts = PyThreadState_GET();
    PyTaskletObject *t;

    /* we always need a cstate, so be sure to initialize */
    if (ts->st.initial_stub == NULL) {
        PyMethodDef def = {"__new__", (PyCFunction)tasklet_new, METH_NOARGS};
        return PyStackless_CallCMethod_Main(&def, (PyObject*)type, NULL);
    }
    if (type == NULL)
        type = &PyTasklet_Type;
    t = (PyTaskletObject *) type->tp_alloc(type, 0);
    if (t == NULL)
        return NULL;
    *(int*)&t->flags = 0;
    t->next = NULL;
    t->prev = NULL;
    t->f.frame = NULL;
    Py_INCREF(Py_None);
    t->tempval = Py_None;
    t->tsk_weakreflist = NULL;
    Py_INCREF(ts->st.initial_stub);
    t->cstate = ts->st.initial_stub;
    t->def_globals = PyEval_GetGlobals();
    Py_XINCREF(t->def_globals);
    if (ts != slp_initial_tstate) {
        /* make sure to kill tasklets with their thread */
        if (slp_ensure_linkage(t)) {
            Py_DECREF(t);
            return NULL;
        }
    }
    return (PyObject*) t;
}

static int
tasklet_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *result = tasklet_bind(self, args, kwds);
    if (NULL == result)
        return -1;
    Py_DECREF(result);
    return 0;
}


/* tasklet pickling support */

PyDoc_STRVAR(tasklet_reduce__doc__,
"Pickling a tasklet for later re-animation.\n\
Note that a tasklet can always be pickled, unless it is current.\n\
Whether it can be run after unpickling depends on the state of the\n\
involved frames. In general, you cannot run a frame with a C state.\
");

/*
Notes on pickling:
We get into trouble with the normal __reduce__ protocol, since
tasklets tend to have tasklets in tempval, and this creates
infinite recursion on pickling.
We therefore adopt the 3-element protocol of __reduce__, where
the third thing is the argument tuple for __setstate__.
Note that we don't use None as the second tuple.
As explained in 'Pickling and unpickling extension types', this
would call a property __basicnew__. This is more complicated,
since __basicnew__ has no parameters, and we need to track
the tasklet type.
The easiest solution was to just use an empty tuple, which causes
simply the tasklet() call without parameters.
*/

static PyObject *
tasklet_reduce(PyTaskletObject * t)
{
    PyObject *tup = NULL, *lis = NULL;
    PyFrameObject *f;
    PyThreadState *ts = t->cstate->tstate;

    if (ts && t == ts->st.current)
        RUNTIME_ERROR("You cannot __reduce__ the tasklet which is"
                      " current.", NULL);
    lis = PyList_New(0);
    if (lis == NULL) goto err_exit;
    f = t->f.frame;
    while (f != NULL) {
        int append_frame = 1;
        if (PyCFrame_Check(f) && ((PyCFrameObject *)f)->f_execute == slp_restore_tracing) {
            append_frame = slp_pickle_with_tracing_state();
            if (-1 == append_frame)
                goto err_exit;
        }
        if (append_frame) {
            if (PyList_Append(lis, (PyObject *) f)) goto err_exit;
        }
        f = f->f_back;
    }
    if (PyList_Reverse(lis)) goto err_exit;
    assert(t->cstate != NULL);
    tup = Py_BuildValue("(O()(" TASKLET_TUPLEFMT "))",
                        Py_TYPE(t),
                        t->flags,
                        t->tempval,
                        t->cstate->nesting_level,
                        lis
                        );
err_exit:
    Py_XDECREF(lis);
    return tup;
}


PyDoc_STRVAR(tasklet_setstate__doc__,
"Tasklets are first created without parameters, and then __setstate__\n\
is called. This was necessary, since pickle has problems pickling\n\
extension types when they reference themselves.\
");

/* note that args is a tuple, although we use METH_O */

static PyObject *
tasklet_setstate(PyObject *self, PyObject *args)
{
    PyTaskletObject *t = (PyTaskletObject *) self;
    PyObject *tempval, *lis;
    int flags, nesting_level;
    PyFrameObject *f;
    Py_ssize_t i, nframes;
    int j;

    if (!PyArg_ParseTuple(args, "iOiO!:tasklet",
                          &flags,
                          &tempval,
                          &nesting_level,
                          &PyList_Type, &lis))
        return NULL;

    nframes = PyList_GET_SIZE(lis);
    TASKLET_SETVAL(t, tempval);

    /* There is a unpickling race condition.  While it is rare,
     * sometimes tasklets get their setstate call after the
     * channel they are blocked on.  If this happens and we
     * do not account for it, they will be left in a broken
     * state where they are on the channels chain, but have
     * cleared their blocked flag.
     *
     * We will assume that the presence of a chain, can only
     * mean that the chain is that of a channel, rather than
     * that of the main tasklet/scheduler. And therefore
     * they can leave their blocked flag in place because the
     * channel would have set it.
     */
    j = t->flags.blocked;
    *(int *)&t->flags = flags;
    if (t->next == NULL) {
        t->flags.blocked = 0;
    } else {
        t->flags.blocked = j;
    }

    /* t->nesting_level = nesting_level;
       XXX how do we handle this?
       XXX to be done: pickle the cstate without a ref to the task.
       XXX This should make it not runnable in the future.
     */
    if (nframes > 0) {
        PyFrameObject *back;
        f = (PyFrameObject *) PyList_GET_ITEM(lis, 0);

        if ((f = slp_ensure_new_frame(f)) == NULL)
            return NULL;
        back = f;
        for (i=1; i<nframes; ++i) {
            f = (PyFrameObject *) PyList_GET_ITEM(lis, i);
            if ((f = slp_ensure_new_frame(f)) == NULL)
                return NULL;
            Py_INCREF(back);
            f->f_back = back;
            back = f;
        }
        t->f.frame = f;
    }
    /* walk frames again and calculate recursion_depth */
    for (f = t->f.frame; f != NULL; f = f->f_back) {
        if (PyFrame_Check(f) && f->f_execute != PyEval_EvalFrameEx_slp) {
            /*
             * we count running frames which *have* added
             * to recursion_depth
             */
            ++t->recursion_depth;
        }
    }
    Py_INCREF(self);
    return self;
}

PyDoc_STRVAR(tasklet_bind_thread__doc__,
"Attempts to re-bind the tasklet to the current thread.\n\
If the tasklet has non-trivial c state, a RuntimeError is\n\
raised.\n\
");


static PyObject *
tasklet_bind_thread(PyObject *self, PyObject *args)
{
    PyTaskletObject *task = (PyTaskletObject *) self;
    PyThreadState *ts = task->cstate->tstate;
    PyThreadState *cts = PyThreadState_GET();
    PyObject *old;
    long target_tid = -1;
    if (!PyArg_ParseTuple(args, "|l:bind_thread", &target_tid))
        return NULL;

    if (target_tid == -1 && ts == cts)
        Py_RETURN_NONE; /* already bound to current thread*/

    if (PyTasklet_Scheduled(task) && !task->flags.blocked) {
        RUNTIME_ERROR("can't (re)bind a runnable tasklet", NULL);
    }
    if (tasklet_has_c_stack(task)) {
        RUNTIME_ERROR("tasklet has C state on its stack", NULL);
    }
    if (target_tid != -1) {
        /* find the correct thread state */
        for(cts = PyInterpreterState_ThreadHead(cts->interp);
            cts != NULL;
            cts = PyThreadState_Next(cts))
        {
            if (cts->thread_id == target_tid)
                break;
        }
    }
    if (cts == NULL || cts->st.initial_stub == NULL) {
        PyErr_SetString(PyExc_ValueError, "bad hread");
        return NULL;
    }
    old = (PyObject*)task->cstate;
    task->cstate = cts->st.initial_stub;
    Py_INCREF(task->cstate);
    Py_DECREF(old);
    Py_RETURN_NONE;
}

/* other tasklet methods */

PyDoc_STRVAR(tasklet_remove__doc__,
"Removing a tasklet from the runnables queue.\n\
Note: If this tasklet has a non-trivial C-state attached, Stackless\n\
will kill the tasklet when the containing thread terminates.\n\
Since this will happen in some unpredictable order, it may cause unwanted\n\
side-effects. Therefore it is recommended to either run tasklets to the\n\
end or to explicitly kill() them.\
");

static PyObject *
tasklet_remove(PyObject *self);

static int
PyTasklet_Remove_M(PyTaskletObject *task)
{
    PyObject *ret;
    PyMethodDef def = {"remove", (PyCFunction)tasklet_remove, METH_NOARGS};
    ret = PyStackless_CallCMethod_Main(&def, (PyObject *) task, NULL);
    return slp_return_wrapper_hard(ret);
}

static int
impl_tasklet_remove(PyTaskletObject *task)
{
    PyThreadState *ts = PyThreadState_GET();
    
    assert(PyTasklet_Check(task));
    if (ts->st.main == NULL) return PyTasklet_Remove_M(task);
    assert(ts->st.current != NULL);

    /* now, operate on the correct thread state */
    ts = task->cstate->tstate;
    if (ts == NULL)
        return 0;

    if (task->flags.blocked)
        RUNTIME_ERROR("You cannot remove a blocked tasklet.", -1);
    if (task == ts->st.current)
        RUNTIME_ERROR("The current tasklet cannot be removed.", -1);
    if (task->next == NULL)
        return 0;
    slp_current_remove_tasklet(task);
    Py_DECREF(task);
    return 0;
}

int
PyTasklet_Remove(PyTaskletObject *task)
{
    return impl_tasklet_remove(task);
}

static PyObject *
tasklet_remove(PyObject *self)
{
    if (impl_tasklet_remove((PyTaskletObject*) self))
        return NULL;
    Py_INCREF(self);
    return self;
}


PyDoc_STRVAR(tasklet_insert__doc__,
"Insert this tasklet at the end of the scheduler list,\n\
given that it isn't blocked.\n\
Blocked tasklets need to be reactivated by channels.");

static PyObject *
tasklet_insert(PyObject *self);

static int
PyTasklet_Insert_M(PyTaskletObject *task)
{
    PyMethodDef def = {"insert", (PyCFunction)tasklet_insert, METH_NOARGS};
    PyObject *ret = PyStackless_CallCMethod_Main(&def, (PyObject *) task, NULL);
    return slp_return_wrapper_hard(ret);
}

static int
impl_tasklet_insert(PyTaskletObject *task)
{
    PyThreadState *ts = PyThreadState_GET();

    assert(PyTasklet_Check(task));
    if (ts->st.main == NULL)
        return PyTasklet_Insert_M(task);
    if (task->flags.blocked)
        RUNTIME_ERROR("You cannot run a blocked tasklet", -1);
    if (task->next == NULL) {
        if (task->f.frame == NULL && task != ts->st.current)
            RUNTIME_ERROR("You cannot run an unbound(dead) tasklet", -1);
        if (task->cstate->tstate == NULL || task->cstate->tstate->st.main == NULL)
            RUNTIME_ERROR("Target thread isn't initialized", -1);
        Py_INCREF(task);
        slp_current_insert(task);
        /* The tasklet may belong to a different thread, and that thread may
         * be blocked, waiting for something to do!
         */
        slp_thread_unblock(task->cstate->tstate);
    }
    return 0;
}

int
PyTasklet_Insert(PyTaskletObject *task)
{
    return impl_tasklet_insert(task);
}

static PyObject *
tasklet_insert(PyObject *self)
{
    if (impl_tasklet_insert((PyTaskletObject*) self))
        return NULL;
    Py_INCREF(self);
    return self;
}


PyDoc_STRVAR(tasklet_run__doc__,
"Run this tasklet, given that it isn't blocked.\n\
Blocked tasks need to be reactivated by channels.");

static PyObject *
tasklet_run(PyObject *self);

static PyObject *
PyTasklet_Run_M(PyTaskletObject *task)
{
    PyMethodDef def = {"run", (PyCFunction)tasklet_run, METH_NOARGS};
    return PyStackless_CallCMethod_Main(&def, (PyObject *) task, NULL);
}

static PyObject *
impl_tasklet_run_remove(PyTaskletObject *task, int remove);

int
PyTasklet_Run_nr(PyTaskletObject *task)
{
    slp_try_stackless = 1;
    return slp_return_wrapper(impl_tasklet_run_remove(task, 0));
}

int
PyTasklet_Run(PyTaskletObject *task)
{
    return slp_return_wrapper_hard(impl_tasklet_run_remove(task, 0));
}

static PyObject *
PyTasklet_Switch_M(PyTaskletObject *task);

static PyObject *
impl_tasklet_run_remove(PyTaskletObject *task, int remove)
{
    STACKLESS_GETARG();
    PyThreadState *ts = PyThreadState_GET();
    PyObject *ret;
    int inserted, fail, removed=0;
    PyTaskletObject *prev = ts->st.current;
    
    assert(PyTasklet_Check(task));
    if (ts->st.main == NULL) {
        if (!remove)
            return PyTasklet_Run_M(task);
        else
            return PyTasklet_Switch_M(task);
    }
    

    /* we always call impl_tasklet_insert, so we must
     * also uninsert in case of failure
     */
    inserted = task->next == NULL;

    if (ts == task->cstate->tstate) {
        /* same thread behaviour.  Insert at the end of the queue and then
         * switch to that task.  Notice that this behaviour upsets FIFO
         * order
         */
        fail = impl_tasklet_insert(task);
        if (!fail && remove) {
            /* if we remove (tasklet.switch), then the current is effecively
             * replaced by the target.
             * move the reference of the current task to del_post_switch */
            assert(ts->st.del_post_switch == NULL);
            ts->st.del_post_switch = (PyObject*)prev;
            slp_current_remove();
            removed = 1;
        }
    } else {
        /* interthread. */
        PyThreadState *rts = task->cstate->tstate;
        PyTaskletObject *current;
        if (rts == NULL)
            RUNTIME_ERROR("tasklet has no thread", NULL);
        current = rts->st.current;
        /* switching only makes sense on the same thread. */
        if (remove)
            RUNTIME_ERROR("can't switch to a different thread.", NULL);

        if (rts->st.thread.is_idle) {
            /* remote thread is blocked, or unblocked and hasn't got the GIL yet.
             * insert it before the "current"
             */
            fail = impl_tasklet_insert(task);
            if (!fail)
                rts->st.current = task;
        } else if (rts->st.current) {
            /* remote thread is executing, put target after the current one */
            rts->st.current = rts->st.current->next;
            fail = impl_tasklet_insert(task);
            rts->st.current = current;
        } else {
            /* remote thread is in a weird state.  Just insert it */
            fail = impl_tasklet_insert(task);
        }
    }
    if (fail)
        return NULL;
    /* this is redundant in the interthread case, since insert already did the work */
    fail = slp_schedule_task(&ret, prev, task, stackless, 0);
    if (fail) {
        if (removed) {
            ts->st.del_post_switch = NULL;
            slp_current_unremove(prev);
        }
        if (inserted) {
            /* we must undo the insertion that we did */
            slp_current_uninsert(task);
            Py_DECREF(task);
        }
    }
    return ret;
}

static PyObject *
tasklet_run(PyObject *self)
{
    return impl_tasklet_run_remove((PyTaskletObject *) self, 0);
}

PyDoc_STRVAR(tasklet_switch__doc__,
"Similar to 'run', but additionally 'remove' the current tasklet\n\
atomically.  This primitive can be used to implement\n\
custom scheduling behaviour.  Only works for tasklets of the\n\
same thread.");

static PyObject *
tasklet_switch(PyObject *self);

static PyObject *
PyTasklet_Switch_M(PyTaskletObject *task)
{
    PyMethodDef def = {"switch", (PyCFunction)tasklet_switch, METH_NOARGS};
    return PyStackless_CallCMethod_Main(&def, (PyObject *) task, NULL);
}

int
PyTasklet_Switch_nr(PyTaskletObject *task)
{
    slp_try_stackless = 1;
    return slp_return_wrapper(impl_tasklet_run_remove(task, 1));
}

int
PyTasklet_Switch(PyTaskletObject *task)
{
    return slp_return_wrapper_hard(impl_tasklet_run_remove(task, 1));
}

static PyObject *
tasklet_switch(PyObject *self)
{
    return impl_tasklet_run_remove((PyTaskletObject *) self, 1);
}

PyDoc_STRVAR(tasklet_set_atomic__doc__,
"t.set_atomic(flag) -- set tasklet atomic status and return current one.\n\
If set, the tasklet will not be auto-scheduled.\n\
This flag is useful for critical sections which should not be interrupted.\n\
usage:\n\
    tmp = t.set_atomic(1)\n\
    # do critical stuff\n\
    t.set_atomic(tmp)\n\
Note: Whenever a new tasklet is created, the atomic flag is initialized\n\
with the atomic flag of the current tasklet.\
Atomic behavior is additionally influenced by the interpreter nesting level.\
See set_ignore_nesting.\
");

int
PyTasklet_SetAtomic(PyTaskletObject *task, int flag)
{
    int ret = task->flags.atomic;

    task->flags.atomic = flag ? 1 : 0;
    if (task->flags.pending_irq && task == PyThreadState_GET()->st.current)
        slp_check_pending_irq();
    return ret;
}

static PyObject *
tasklet_set_atomic(PyObject *self, PyObject *flag)
{
    if (! (flag && PyLong_Check(flag)) )
        TYPE_ERROR("set_atomic needs exactly one bool or integer",
                   NULL);
    return PyBool_FromLong(PyTasklet_SetAtomic(
        (PyTaskletObject*)self, PyLong_AsLong(flag)));
}


PyDoc_STRVAR(tasklet_set_ignore_nesting__doc__,
"t.set_ignore_nesting(flag) -- set tasklet ignore_nesting status and return current one.\n\
If set, the tasklet may be be auto-scheduled, even if its nesting_level is > 0.\n\
This flag makes sense if you know that nested interpreter levels are safe\n\
for auto-scheduling. This is on your own risk, handle with care!\n\
usage:\n\
    tmp = t.set_ignore_nesting(1)\n\
    # do critical stuff\n\
    t.set_ignore_nesting(tmp)\
");

int
PyTasklet_SetIgnoreNesting(PyTaskletObject *task, int flag)
{
    int ret = task->flags.ignore_nesting;

    task->flags.ignore_nesting = flag ? 1 : 0;
    if (task->flags.pending_irq && task == PyThreadState_GET()->st.current)
        slp_check_pending_irq();
    return ret;
}

static PyObject *
tasklet_set_ignore_nesting(PyObject *self, PyObject *flag)
{
    if (! (flag && PyLong_Check(flag)) )
        TYPE_ERROR("set_ignore_nesting needs exactly one bool or integer",
                   NULL);
    return PyBool_FromLong(PyTasklet_SetIgnoreNesting(
    (PyTaskletObject*)self, PyLong_AsLong(flag)));
}


static int
bind_tasklet_to_frame(PyTaskletObject *task, PyFrameObject *frame)
{
    PyThreadState *ts = task->cstate->tstate;
    if (ts == NULL)
        RUNTIME_ERROR("tasklet has no thread", -1);
    if (task->f.frame != NULL)
        RUNTIME_ERROR("tasklet is already bound to a frame", -1);
    task->f.frame = frame;
    if (task->cstate != ts->st.initial_stub) {
        PyCStackObject *hold = task->cstate;
        task->cstate = ts->st.initial_stub;
        Py_INCREF(task->cstate);
        Py_DECREF(hold);
        if (ts != slp_initial_tstate)
            if (slp_ensure_linkage(task))
                return -1;
    }
    return 0;
    /* note: We expect that f_back is NULL, or will be adjusted immediately */
}

/* this is also the setup method */

PyDoc_STRVAR(tasklet_setup__doc__, "supply the parameters for the callable");

static int
PyTasklet_Setup_M(PyTaskletObject *task, PyObject *args, PyObject *kwds)
{
    PyObject *ret = PyStackless_Call_Main((PyObject*)task, args, kwds);

    return slp_return_wrapper(ret);
}

int PyTasklet_Setup(PyTaskletObject *task, PyObject *args, PyObject *kwds)
{
    PyObject *ret = Py_TYPE(task)->tp_call((PyObject *) task, args, kwds);

    return slp_return_wrapper(ret);
}

static int
impl_tasklet_setup(PyTaskletObject *task, PyObject *args, PyObject *kwds, int insert)
{
    PyThreadState *ts = PyThreadState_GET();
    PyFrameObject *frame;
    PyObject *func;

    assert(PyTasklet_Check(task));
    if (ts->st.main == NULL) return PyTasklet_Setup_M(task, args, kwds);

    func = task->tempval;
    if (func == NULL || func == Py_None)
        RUNTIME_ERROR("the tasklet was not bound to a function", -1);
    if ((frame = (PyFrameObject *)
                 slp_cframe_newfunc(func, args, kwds, 0)) == NULL) {
        return -1;
    }
    if (bind_tasklet_to_frame(task, frame)) {
        Py_DECREF(frame);
        return -1;
    }
    TASKLET_SETVAL(task, Py_None);
    if (insert) {
        Py_INCREF(task);
        slp_current_insert(task);
    }
    return 0;
}

static PyObject *
tasklet_setup(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyTaskletObject *task = (PyTaskletObject *) self;

    if (impl_tasklet_setup(task, args, kwds, 1))
        return NULL;
    Py_INCREF(task);
    return (PyObject*) task;
}


PyDoc_STRVAR(tasklet_throw__doc__,
             "tasklet.throw(exc, val=None, tb=None, immediate=True) -- raise an exception for the tasklet.\n\
             'exc', 'val' and 'tb' have the same semantics as the 'raise' statement of the Python(r) language.\n\
             If 'pending' is True, the tasklet is not immediately activated, just\n\
             merely made runnable, ready to raise the exception when run.");

static PyObject *
tasklet_throw(PyObject *myself, PyObject *args, PyObject *kwds);

static PyObject *
PyTasklet_Throw_M(PyTaskletObject *self, int pending, PyObject *exc,
                           PyObject *val, PyObject *tb)
{
    PyMethodDef def = {"throw", (PyCFunction)tasklet_throw, METH_VARARGS|METH_KEYWORDS};
    if (!val)
        val = Py_None;
    if (!tb)
        tb = Py_None;
    return PyStackless_CallCMethod_Main(&def, (PyObject *) self, "OOOi",  exc, val, tb, pending);
}

static PyObject *
impl_tasklet_throw(PyTaskletObject *self, int pending, PyObject *exc, PyObject *val, PyObject *tb)
{
    STACKLESS_GETARG();
    PyThreadState *ts = PyThreadState_GET();
    PyObject *ret, *bomb, *tmpval;
    int fail;

    if (ts->st.main == NULL)
        return PyTasklet_Throw_M(self, pending, exc, val, tb);

    bomb = slp_exc_to_bomb(exc, val, tb);
    if (bomb == NULL)
        return NULL;

    /* raise it directly if target is ourselves.  delayed exception makes
     * no sense in this case
     */
    if (ts->st.current == self)
        return slp_bomb_explode(bomb);

    /* don't attempt to send to a dead tasklet.
     * f.frame is null for the running tasklet and a dead tasklet
     * A new tasklet has a CFrame
     */
    if (self->cstate->tstate == NULL || (self->f.frame == NULL && self != self->cstate->tstate->st.current)) {
        /* however, allow tasklet exit errors for already dead tasklets */
        if (PyObject_IsSubclass(((PyBombObject*)bomb)->curexc_type, PyExc_TaskletExit)) {
            Py_DECREF(bomb);
            Py_RETURN_NONE;
        }
        Py_DECREF(bomb);
        RUNTIME_ERROR("You cannot throw to a dead tasklet", NULL);
    }
    TASKLET_CLAIMVAL(self, &tmpval);
    TASKLET_SETVAL_OWN(self, bomb);
    if (!pending) {
        fail = slp_schedule_task(&ret, ts->st.current, self, stackless, 0);
    } else {
        /* pending throw.  Make the tasklet runnable */
        PyChannelObject *u_chan = NULL;
        PyTaskletObject *u_next;
        int u_dir;
        /* Unblock it if required*/
        if (self->flags.blocked) {
            /* we claim the channel's reference */
            slp_channel_remove_slow(self, &u_chan, &u_dir, &u_next);
        }
        fail = PyTasklet_Insert(self);
        if (u_chan) {
            if (fail)
                slp_channel_insert(u_chan, self, u_dir, u_next);
            else
                Py_DECREF(self);
        }
        ret = fail ? NULL : Py_None;
        Py_XINCREF(ret);
    }
    if (fail)
        TASKLET_SETVAL_OWN(self, tmpval);
    else
        Py_DECREF(tmpval);
    return ret;
}

int PyTasklet_Throw(PyTaskletObject *self, int pending, PyObject *exc,
                             PyObject *val, PyObject *tb)
{
    return slp_return_wrapper_hard(impl_tasklet_throw(self, pending, exc, val, tb));
}

static PyObject *
tasklet_throw(PyObject *myself, PyObject *args, PyObject *kwds)
{
    STACKLESS_GETARG();
    PyObject *result = NULL;
    PyObject *exc, *val=Py_None, *tb=Py_None;
    int pending;
    PyObject *pendingO = Py_False;
    char *kwlist[] = {"exc", "val", "tb", "pending", 0};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO:throw", kwlist, &exc, &val, &tb, &pendingO))
        return NULL;
    pending = PyObject_IsTrue(pendingO);
    if (pending == -1)
        return NULL;
    STACKLESS_PROMOTE_ALL();
    result = impl_tasklet_throw(
        (PyTaskletObject*)myself, pending, exc, val, tb);
    STACKLESS_ASSERT();
    return result;
}


PyDoc_STRVAR(tasklet_raise_exception__doc__,
"tasklet.raise_exception(exc, value) -- raise an exception for the tasklet.\n\
exc must be a subclass of Exception.\n\
The tasklet is immediately activated.");

static PyObject *
tasklet_raise_exception(PyObject *myself, PyObject *args);

static PyObject *
PyTasklet_RaiseException_M(PyTaskletObject *self, PyObject *klass,
                           PyObject *args)
{
    PyMethodDef def = {"raise_exception", (PyCFunction)tasklet_raise_exception, METH_VARARGS};
    return PyStackless_CallCMethod_Main(&def, (PyObject *) self, "OO", klass, args);
}

static PyObject *
impl_tasklet_raise_exception(PyTaskletObject *self, PyObject *klass, PyObject *args)

{
    STACKLESS_GETARG();
    PyThreadState *ts = PyThreadState_GET();
    PyObject *ret, *bomb, *tmpval;
    int fail;

    if (ts->st.main == NULL)
        return PyTasklet_RaiseException_M(self, klass, args);
    bomb = slp_make_bomb(klass, args, "tasklet.raise_exception");
    if (bomb == NULL)
        return NULL;
    if (ts->st.current == self)
        return slp_bomb_explode(bomb);
    /* if the tasklet is dead, do not run it (no frame) but explode */
    if (slp_get_frame(self) == NULL)
        return slp_bomb_explode(bomb);
    
    TASKLET_CLAIMVAL(self, &tmpval);
    TASKLET_SETVAL_OWN(self, bomb);
    fail = slp_schedule_task(&ret, ts->st.current, self, stackless, 0);
    if (fail)
        TASKLET_SETVAL_OWN(self, tmpval);
    else
        Py_DECREF(tmpval);
    return ret;
}

int PyTasklet_RaiseException(PyTaskletObject *self, PyObject *klass,
                             PyObject *args)
{
    return slp_return_wrapper_hard(impl_tasklet_raise_exception(self, klass, args));
}

static PyObject *
tasklet_raise_exception(PyObject *myself, PyObject *args)
{
    STACKLESS_GETARG();
    PyObject *result = NULL;
    PyObject *klass = PySequence_GetItem(args, 0);

    if (klass == NULL)
        TYPE_ERROR("raise_exception() takes at least 1 argument", NULL);
    args = PySequence_GetSlice(args, 1, PySequence_Size(args));
    if (!args) goto err_exit;
    STACKLESS_PROMOTE_ALL();
    result = impl_tasklet_raise_exception(
        (PyTaskletObject*)myself, klass, args);
    STACKLESS_ASSERT();
err_exit:
    Py_DECREF(klass);
    Py_XDECREF(args);
    return result;
}


PyDoc_STRVAR(tasklet_kill__doc__,
"tasklet.kill(pending=False) -- raise a TaskletExit exception for the tasklet.\n\
Note that this is a regular exception that can be caught.\n\
The tasklet is immediately activated.\n\
If the exception passes the toplevel frame of the tasklet,\n\
the tasklet will silently die.");

static PyObject *
impl_tasklet_kill(PyTaskletObject *task, int pending)
{
    STACKLESS_GETARG();
    PyObject *ret;

    /*
     * silently do nothing if the tasklet is dead.
     * simple raising would kill ourself in this case.
     */
    if (slp_get_frame(task) == NULL) {
        /* it can still be a new tasklet and not a dead one */
        Py_CLEAR(task->f.cframe);
        if (task->next) {
            /* remove it from the run queue */
            assert(!task->flags.blocked);
            slp_current_remove_tasklet(task);
            Py_DECREF(task);
        }
        Py_RETURN_NONE;
    }

    /* we might be called after exceptions are gone */
    if (PyExc_TaskletExit == NULL) {
        PyExc_TaskletExit = PyUnicode_FromString("zombie");
        if (PyExc_TaskletExit == NULL)
            return NULL; /* give up */
    }
    STACKLESS_PROMOTE_ALL();
    ret = impl_tasklet_throw(task, pending, PyExc_TaskletExit, NULL, NULL);
    STACKLESS_ASSERT();
    return ret;
}

int PyTasklet_Kill(PyTaskletObject *task)
{
    return slp_return_wrapper_hard(impl_tasklet_kill(task, 0));
}

static PyObject *
tasklet_kill(PyObject *self, PyObject *args, PyObject *kwds)
{
    int pending;
    PyObject *pendingO = Py_False;
    char *kwlist[] = {"pending", 0};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O:kill", kwlist, &pendingO))
        return NULL;
    pending = PyObject_IsTrue(pendingO);
    if (pending == -1)
        return NULL;
    return impl_tasklet_kill((PyTaskletObject*)self, pending);
}


/* attributes which are hiding in small fields */

static PyObject *
tasklet_get_blocked(PyTaskletObject *task)
{
    return PyBool_FromLong(task->flags.blocked);
}

int PyTasklet_GetBlocked(PyTaskletObject *task)
{
    return task->flags.blocked;
}


static PyObject *
tasklet_get_atomic(PyTaskletObject *task)
{
    return PyBool_FromLong(task->flags.atomic);
}

int PyTasklet_GetAtomic(PyTaskletObject *task)
{
    return task->flags.atomic;
}


static PyObject *
tasklet_get_ignore_nesting(PyTaskletObject *task)
{
    return PyBool_FromLong(task->flags.ignore_nesting);
}

int PyTasklet_GetIgnoreNesting(PyTaskletObject *task)
{
    return task->flags.ignore_nesting;
}


static PyObject *
tasklet_get_frame(PyTaskletObject *task)
{
    PyObject *ret = (PyObject*) PyTasklet_GetFrame(task);
    if (ret)
		return ret;
	Py_RETURN_NONE;
}

PyObject *
PyTasklet_GetFrame(PyTaskletObject *task)
{
    PyFrameObject *f = (PyFrameObject *) slp_get_frame(task);

    while (f != NULL && !PyFrame_Check(f)) {
        f = f->f_back;
    }
    Py_XINCREF(f);
    return (PyObject *) f;
}


static PyObject *
tasklet_get_block_trap(PyTaskletObject *task)
{
    return PyBool_FromLong(task->flags.block_trap);
}

int PyTasklet_GetBlockTrap(PyTaskletObject *task)
{
    return task->flags.block_trap;
}


static int
tasklet_set_block_trap(PyTaskletObject *task, PyObject *value)
{
    if (!PyLong_Check(value))
        TYPE_ERROR("block_trap must be set to a bool or integer", -1);
    task->flags.block_trap = PyLong_AsLong(value) ? 1 : 0;
    return 0;
}

void PyTasklet_SetBlockTrap(PyTaskletObject *task, int value)
{
    task->flags.block_trap = value ? 1 : 0;
}


static PyObject *
tasklet_is_main(PyTaskletObject *task)
{
    PyThreadState *ts = task->cstate->tstate;
    return PyBool_FromLong(ts && task == ts->st.main);
}

int
PyTasklet_IsMain(PyTaskletObject *task)
{
    PyThreadState *ts = task->cstate->tstate;
    return ts && task == ts->st.main;
}


static PyObject *
tasklet_is_current(PyTaskletObject *task)
{
    PyThreadState *ts = task->cstate->tstate;
    return PyBool_FromLong(ts && task == ts->st.current);
}

int
PyTasklet_IsCurrent(PyTaskletObject *task)
{
    PyThreadState *ts = task->cstate->tstate;
    return ts && task == ts->st.current;
}


static PyObject *
tasklet_get_recursion_depth(PyTaskletObject *task)
{
    PyThreadState *ts;

    assert(task->cstate != NULL);
    ts = task->cstate->tstate;
    return PyLong_FromLong((ts && ts->st.current == task) ? ts->recursion_depth
                                                  : task->recursion_depth);
}

int
PyTasklet_GetRecursionDepth(PyTaskletObject *task)
{
    PyThreadState *ts;

    assert(task->cstate != NULL);
    ts = task->cstate->tstate;
    return (ts && ts->st.current == task) ? ts->recursion_depth
                                  : task->recursion_depth;
}


static PyObject *
tasklet_get_nesting_level(PyTaskletObject *task)
{
    PyThreadState *ts;

    assert(task->cstate != NULL);
    ts = task->cstate->tstate;
    return PyLong_FromLong(
        (ts && ts->st.current == task) ? ts->st.nesting_level
                               : task->cstate->nesting_level);
}

int
PyTasklet_GetNestingLevel(PyTaskletObject *task)
{
    PyThreadState *ts;

    assert(task->cstate != NULL);
    ts = task->cstate->tstate;
    return (ts && ts->st.current == task) ? ts->st.nesting_level
                                  : task->cstate->nesting_level;
}


/* attributes which are handy, but easily computed */

static PyObject *
tasklet_alive(PyTaskletObject *task)
{
    return PyBool_FromLong(slp_get_frame(task) != NULL);
}

int
PyTasklet_Alive(PyTaskletObject *task)
{
    return slp_get_frame(task) != NULL;
}


static PyObject *
tasklet_paused(PyTaskletObject *task)
{
    return PyBool_FromLong(
        slp_get_frame(task) != NULL && task->next == NULL);
}

int
PyTasklet_Paused(PyTaskletObject *task)
{
    return slp_get_frame(task) != NULL && task->next == NULL;
}


static PyObject *
tasklet_scheduled(PyTaskletObject *task)
{
    return PyBool_FromLong(task->next != NULL);
}

int
PyTasklet_Scheduled(PyTaskletObject *task)
{
    return task->next != NULL;
}

static PyObject *
tasklet_restorable(PyTaskletObject *task)
{
    PyThreadState *ts;

    assert(task->cstate != NULL);
    ts = task->cstate->tstate;
    return PyBool_FromLong(
        0 == ((ts && ts->st.current == task) ? ts->st.nesting_level
                                     : task->cstate->nesting_level) );
}

int
PyTasklet_Restorable(PyTaskletObject *task)
{
    PyThreadState *ts;

    assert(task->cstate != NULL);
    ts = task->cstate->tstate;
    return 0 == ((ts && ts->st.current == task) ? ts->st.nesting_level
                                        : task->cstate->nesting_level);
}

static PyObject *
tasklet_get_channel(PyTaskletObject *task)
{
    PyTaskletObject *prev = task->prev;
    PyObject *ret = Py_None;
    if (prev != NULL && task->flags.blocked) {
        /* search left, optimizing in-oder access */
        while (!PyChannel_Check(prev))
            prev = prev->prev;
        ret = (PyObject *) prev;
    }
    Py_INCREF(ret);
    return ret;
}

static PyObject *
tasklet_get_next(PyTaskletObject *task)
{
    PyObject *ret = Py_None;

    if (task->next != NULL && PyTasklet_Check(task->next))
        ret = (PyObject *) task->next;
    Py_INCREF(ret);
    return ret;
}

static PyObject *
tasklet_get_prev(PyTaskletObject *task)
{
    PyObject *ret = Py_None;

    if (task->prev != NULL && PyTasklet_Check(task->prev))
        ret = (PyObject *) task->prev;
    Py_INCREF(ret);
    return ret;
}

static PyObject *
tasklet_thread_id(PyTaskletObject *task)
{
    long id = task->cstate->tstate ? task->cstate->tstate->thread_id : -1;
    return PyLong_FromLong(id);
}

static PyObject *
tasklet_get_trace_function(PyTaskletObject *task)
{
    PyThreadState *ts = task->cstate->tstate;
    PyCFrameObject *f;
    PyTaskletTStateStruc *saved_ts;

    if (ts && ts->st.current == task) {
        /* current tasklet */
        PyObject *temp = ts->c_traceobj;
        if (temp == NULL)
            temp = Py_None;
        Py_INCREF(temp);
        return temp;
    }
    
    f = task->f.cframe;
    while (NULL != f && PyCFrame_Check(f)) {
        if (f->f_execute == slp_restore_tracing) {
            /* we found a restore tracing frame */
            PyObject *temp = f->ob1;
            if (temp == NULL)
                temp = Py_None;
            Py_INCREF(temp);
            return temp;
        }
        f = (PyCFrameObject *)(f->f_back);
    }

    /* try the saved tstate of a hard switched tasklet */
    saved_ts = slp_get_saved_tstate(task);
    if (NULL != saved_ts) {
        PyObject *temp;
        temp = saved_ts->c_traceobj;
        if (NULL == temp)
            temp = Py_None;
        Py_INCREF(temp);
        return temp;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static int
tasklet_set_trace_function(PyTaskletObject *task, PyObject *value)
{
    PyThreadState *ts = task->cstate->tstate;
    PyCFrameObject *f;
    Py_tracefunc tf = NULL;
    PyTaskletTStateStruc *saved_ts;

    if (Py_None == value)
        value = NULL;
    if (value) {
        tf = slp_get_sys_trace_func();
        if (NULL == tf)
            return -1;
    }
    if (ts && ts->st.current == task) {
        /* current tasklet */
        if (PyThreadState_GET() != ts)
            RUNTIME_ERROR("You cannot set the trace function of the current tasklet of another thread", -1);
        PyEval_SetTrace(tf, value);
        return 0;
    }
    
    f = task->f.cframe;
    while (NULL != f && PyCFrame_Check(f)) {
        if (f->f_execute == slp_restore_tracing) {
            /* we found a restore tracing frame */
            PyObject *temp;
            int c_functions = slp_encode_ctrace_functions(tf, f->any2);
            if (c_functions < 0)
                return -1;
            temp = f->ob1;
            Py_XINCREF(value);
            f->ob1 = value;
            f->any1 = tf;
            f->n = c_functions;
            Py_XDECREF(temp);

            return 0;
        }
        f = (PyCFrameObject *)(f->f_back);
    }

    /* try the saved tstate of a hard switched tasklet */
    saved_ts = slp_get_saved_tstate(task);
    if (NULL != saved_ts) {
        PyObject *temp;
        temp = saved_ts->c_traceobj;
        Py_XINCREF(value);
        saved_ts->c_traceobj = value;
        saved_ts->c_tracefunc = tf;
        Py_XDECREF(temp);

        return 0;
    }

    /* task is neither current nor has a restore_tracing frame.
       ==> tracing is currently off */
    if (NULL == value)
        /* nothing to do */ 
        return 0;

    /* here we must add an restore tracing cframe */
    f = task->f.cframe;
    if (NULL != f) {
        /* Insert a new cframe */
        PyCFrameObject *cf;
        int c_functions = slp_encode_ctrace_functions(tf, f->any2);
        if (c_functions < 0)
            return -1;
        cf = slp_cframe_new(slp_restore_tracing, 0);
        if (cf == NULL)
            return -1;
        Py_INCREF(f);
        cf->f_back = (PyFrameObject *)f;
        task->f.cframe = cf;
        Py_INCREF(value);
        assert(NULL == cf->ob1);
        cf->ob1 = value;
        cf->any1 = tf;
        cf->n = c_functions;
        assert(0 == cf->i);        /* ts->tracing */
        assert(NULL == cf->any2);  /* ts->c_profilefunc */
        assert(NULL == cf->ob2);   /* ts->c_profileobj */
        return 0;
    }

    RUNTIME_ERROR("tasklet is not alive", -1);
}

static PyObject *
tasklet_get_profile_function(PyTaskletObject *task)
{
    PyThreadState *ts = task->cstate->tstate;
    PyCFrameObject *f;
    PyTaskletTStateStruc *saved_ts;

    if (ts && ts->st.current == task) {
        /* current tasklet */
        PyObject *temp = ts->c_profileobj;
        if (temp == NULL)
            temp = Py_None;
        Py_INCREF(temp);
        return temp;
    }
    
    f = task->f.cframe;
    while (NULL != f && PyCFrame_Check(f)) {
        if (f->f_execute == slp_restore_tracing) {
            /* we found a restore tracing frame */
            PyObject *temp = f->ob2;
            if (temp == NULL)
                temp = Py_None;
            Py_INCREF(temp);
            return temp;
        }
        f = (PyCFrameObject *)(f->f_back);
    }

    /* try the saved tstate of a hard switched tasklet */
    saved_ts = slp_get_saved_tstate(task);
    if (NULL != saved_ts) {
        PyObject *temp;
        temp = saved_ts->c_profileobj;
        if (NULL == temp)
            temp = Py_None;
        Py_INCREF(temp);
        return temp;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static int
tasklet_set_profile_function(PyTaskletObject *task, PyObject *value)
{
    PyThreadState *ts = task->cstate->tstate;
    PyCFrameObject *f;
    Py_tracefunc tf = NULL;
    PyTaskletTStateStruc *saved_ts;

    if (Py_None == value)
        value = NULL;
    if (value) {
        tf = slp_get_sys_profile_func();
        if (NULL == tf)
            return -1;
    }
    if (ts && ts->st.current == task) {
        /* current tasklet */
        if (PyThreadState_GET() != ts)
            RUNTIME_ERROR("You cannot set the profile function of the current tasklet of another thread", -1);
        PyEval_SetProfile(tf, value);
        return 0;
    }
    
    f = task->f.cframe;
    while (NULL != f && PyCFrame_Check(f)) {
        if (f->f_execute == slp_restore_tracing) {
            PyObject *temp;
            int c_functions = slp_encode_ctrace_functions(f->any1, tf);
            if (c_functions < 0)
                return -1;
            /* we found a restore tracing frame */
            temp = f->ob2;
            Py_XINCREF(value);
            f->ob2 = value;
            f->any2 = tf;
            f->n = c_functions;
            Py_XDECREF(temp);

            return 0;
        }
        f = (PyCFrameObject *)(f->f_back);
    }

    /* try the saved tstate of a hard switched tasklet */
    saved_ts = slp_get_saved_tstate(task);
    if (NULL != saved_ts) {
        PyObject *temp;
        temp = saved_ts->c_profileobj;
        Py_XINCREF(value);
        saved_ts->c_profileobj = value;
        saved_ts->c_profilefunc = tf;
        Py_XDECREF(temp);

        return 0;
    }

    /* task is neither current nor has a restore_tracing frame.
       ==> tracing is currently off */
    if (NULL == value)
        /* nothing to do */ 
        return 0;

    /* here we must add an restore tracing cframe */
    f = task->f.cframe;
    if (NULL != f) {
        /* Insert a new cframe */
        PyCFrameObject *cf;
        int c_functions = slp_encode_ctrace_functions(f->any1, tf);
        if (c_functions < 0)
            return -1;
        cf = slp_cframe_new(slp_restore_tracing, 0);
        if (cf == NULL)
            return -1;
        Py_INCREF(f);
        cf->f_back = (PyFrameObject *)f;
        task->f.cframe = cf;
        Py_INCREF(value);
        assert(NULL == cf->ob2);
        cf->ob2 = value;
        cf->any2 = tf;
        cf->n = c_functions;
        assert(0 == cf->i);        /* ts->tracing */
        assert(NULL == cf->any1);  /* ts->c_tracefunc */
        assert(NULL == cf->ob1);   /* ts->c_traceobj */
        return 0;
    }

    RUNTIME_ERROR("tasklet is not alive", -1);
}

static PyMemberDef tasklet_members[] = {
    {"cstate", T_OBJECT, offsetof(PyTaskletObject, cstate), READONLY,
     PyDoc_STR("the C stack object associated with the tasklet.\n\
     Every tasklet has a cstate, even if it is a trivial one.\n\
     Please see the cstate doc and the stackless documentation.")},
    {"tempval", T_OBJECT, offsetof(PyTaskletObject, tempval), 0},
    /* blocked, slicing_lock, atomic and such are treated by tp_getset */
    {0}
};

static PyGetSetDef tasklet_getsetlist[] = {
    {"next", (getter)tasklet_get_next, NULL,
     PyDoc_STR("the next tasklet in a a circular list of tasklets.")},

    {"prev", (getter)tasklet_get_prev, NULL,
     PyDoc_STR("the previous tasklet in a circular list of tasklets")},

    {"_channel", (getter)tasklet_get_channel, NULL,
     PyDoc_STR("The channel this tasklet is blocked on, or None if it is not blocked.\n"
     "This computed attribute may cause a linear search and should normally\n"
     "not be used, or be replaced by a real attribute in a derived type.")
    },

    {"blocked", (getter)tasklet_get_blocked, NULL,
     PyDoc_STR("Nonzero if waiting on a channel (1: send, -1: receive).\n"
     "Part of the flags word.")},

    {"atomic", (getter)tasklet_get_atomic, NULL,
     PyDoc_STR("atomic inhibits scheduling of this tasklet. See set_atomic()\n"
     "Part of the flags word.")},

    {"ignore_nesting", (getter)tasklet_get_ignore_nesting, NULL,
     PyDoc_STR("unless ignore_nesting is set, any nesting level > 0 inhibits\n"
     "auto-scheduling of this tasklet. See set_ignore_nesting()\n"
     "Part of the flags word.")},

    {"frame", (getter)tasklet_get_frame, NULL,
     PyDoc_STR("the current frame of this tasklet. For the running tasklet,\n"
     "this is redirected to tstate.frame.")},

    {"block_trap", (getter)tasklet_get_block_trap,
                   (setter)tasklet_set_block_trap,
     PyDoc_STR("An individual lock against blocking on a channel.\n"
     "This is used as a debugging aid to find out undesired blocking.\n"
     "Instead of trying to block, an exception is raised.")},

    {"is_main", (getter)tasklet_is_main, NULL,
     PyDoc_STR("There always exists exactly one tasklet per thread which acts as\n"
     "main. It receives all uncaught exceptions and can act as a watchdog.\n"
     "This attribute is computed.")},

    {"is_current", (getter)tasklet_is_current, NULL,
     PyDoc_STR("There always exists exactly one tasklet per thread which is "
     "currently running.\n"
     "This attribute is computed.")},

    {"paused", (getter)tasklet_paused, NULL,
     PyDoc_STR("A tasklet is said to be paused if it is neither in the runnables list\n"
     "nor blocked, but alive. This state is entered after a t.remove()\n"
     "or by the main tasklet, when it is acting as a watchdog.\n"
     "This attribute is computed.")},

    {"scheduled", (getter)tasklet_scheduled, NULL,
     PyDoc_STR("A tasklet is said to be scheduled if it is either in the runnables list\n"
     "or waiting in a channel.\n"
     "This attribute is computed.")},

    {"recursion_depth", (getter)tasklet_get_recursion_depth, NULL,
     PyDoc_STR("The system recursion_depth is replicated for every tasklet.\n"
     "They all start running with a recursion_depth of zero.")},

    {"nesting_level", (getter)tasklet_get_nesting_level, NULL,
     PyDoc_STR("The interpreter nesting level is monitored by every tasklet.\n"
     "They all start running with a nesting level of zero.")},

    {"restorable", (getter)tasklet_restorable, NULL,
     PyDoc_STR("True, if the tasklet can be completely restored by pickling/unpickling.\n"
     "All tasklets can be pickled for debugging/inspection purposes, but an \n"
     "unpickled tasklet might have lost runtime information (C stack).")},

    {"alive", (getter)tasklet_alive, NULL,
     PyDoc_STR("A tasklet is alive if it has an associated frame.\n"
     "This attribute is computed.")},

    {"thread_id", (getter)tasklet_thread_id, NULL,
     PyDoc_STR("Return the thread id of the thread the tasklet belongs to.")},

    {"trace_function", (getter)tasklet_get_trace_function,
                   (setter)tasklet_set_trace_function,
     PyDoc_STR("The trace function of this tasklet. None by default.\n"
     "For the current tasklet this property is equivalent to sys.gettrace()\n"
     "and sys.settrace().")},

    {"profile_function", (getter)tasklet_get_profile_function,
                   (setter)tasklet_set_profile_function,
     PyDoc_STR("The trace function of this tasklet. None by default.\n"
     "For the current tasklet this property is equivalent to sys.gettrace()\n"
     "and sys.settrace().")},

    {0},
};

#define PCF PyCFunction
#define METH_VS METH_VARARGS | METH_STACKLESS
#define METH_KS METH_VARARGS | METH_KEYWORDS | METH_STACKLESS
#define METH_NS METH_NOARGS | METH_STACKLESS

static PyMethodDef tasklet_methods[] = {
    {"insert",                  (PCF)tasklet_insert,        METH_NOARGS,
      tasklet_insert__doc__},
    {"run",                     (PCF)tasklet_run,           METH_NS,
     tasklet_run__doc__},
    {"switch",                  (PCF)tasklet_switch,        METH_NS,
     tasklet_switch__doc__},
    {"remove",                  (PCF)tasklet_remove,        METH_NOARGS,
     tasklet_remove__doc__},
    {"set_atomic",              (PCF)tasklet_set_atomic,    METH_O,
     tasklet_set_atomic__doc__},
    {"set_ignore_nesting", (PCF)tasklet_set_ignore_nesting, METH_O,
     tasklet_set_ignore_nesting__doc__},
    {"throw",                   (PCF)tasklet_throw,         METH_KS,
    tasklet_throw__doc__},
    {"raise_exception",         (PCF)tasklet_raise_exception, METH_VS,
    tasklet_raise_exception__doc__},
    {"kill",                    (PCF)tasklet_kill,          METH_KS,
     tasklet_kill__doc__},
    {"bind",                    (PCF)tasklet_bind,          METH_VARARGS | METH_KEYWORDS,
     tasklet_bind__doc__},
    {"setup",                   (PCF)tasklet_setup,         METH_VARARGS | METH_KEYWORDS,
     tasklet_setup__doc__},
    {"__reduce__",              (PCF)tasklet_reduce,        METH_NOARGS,
     tasklet_reduce__doc__},
    {"__reduce_ex__",           (PCF)tasklet_reduce,        METH_VARARGS,
     tasklet_reduce__doc__},
    {"__setstate__",            (PCF)tasklet_setstate,      METH_O,
     tasklet_setstate__doc__},
    {"bind_thread",              (PCF)tasklet_bind_thread,  METH_VARARGS,
    tasklet_bind_thread__doc__},
    {NULL,     NULL}             /* sentinel */
};

PyDoc_STRVAR(tasklet__doc__,
"A tasklet object represents a tiny task in a Python(r) thread.\n\
At program start, there is always one running main tasklet.\n\
New tasklets can be created with methods from the stackless\n\
module.\n\
");


PyTypeObject PyTasklet_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    "_stackless.tasklet",
    sizeof(PyTaskletObject),
    0,
    (destructor)tasklet_dealloc,        /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    tasklet_setup,                      /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    PyObject_GenericSetAttr,            /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,            /* tp_flags */
    tasklet__doc__,                     /* tp_doc */
    (traverseproc)tasklet_traverse,     /* tp_traverse */
    (inquiry) tasklet_clear,            /* tp_clear */
    0,                                  /* tp_richcompare */
    offsetof(PyTaskletObject, tsk_weakreflist), /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    tasklet_methods,                    /* tp_methods */
    tasklet_members,                    /* tp_members */
    tasklet_getsetlist,                 /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    tasklet_init,                       /* tp_init */
    0,                                  /* tp_alloc */
    tasklet_new,                        /* tp_new */
    PyObject_GC_Del,                    /* tp_free */
};
#endif

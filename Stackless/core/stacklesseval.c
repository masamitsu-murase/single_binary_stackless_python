#include "Python.h"
#ifdef STACKLESS

#include "compile.h"
#include "frameobject.h"
#include "structmember.h"

#include "stackless_impl.h"
#include "pickling/prickelpit.h"

/* platform specific constants */
#include "platf/slp_platformselect.h"

/* Stackless extension for ceval.c */

/******************************************************

  Static Global Variables

*******************************************************/

/* the flag which decides whether we try to use soft switching */

int slp_enable_softswitch = 1;

/* compatibility mask for Psyco. It will be set to nonzero when
 * psyco-compiled code is run. Suppresses soft-switching.
 */
int slp_in_psyco = 0;

/*
 * flag whether the next call should try to be stackless.
 * The protocol is: This flag may be only set if the called
 * thing supports it. It doesn't matter whether it uses the
 * chance, but it *must* set it to zero before returning.
 * This flags in a way serves as a parameter that we don't have.
 */
int slp_try_stackless = 0;

/* the list of all stacks of all threads */
struct _cstack *slp_cstack_chain = NULL;


/******************************************************

  The C Stack

 ******************************************************/

static PyCStackObject *cstack_cache[CSTACK_SLOTS] = { NULL };
static int cstack_cachecount = 0;

/* this function will get called by PyStacklessEval_Fini */
static void slp_cstack_cacheclear(void)
{
    int i;
    PyCStackObject *stack;

    for (i=0; i < CSTACK_SLOTS; i++) {
        while (cstack_cache[i] != NULL) {
            stack = cstack_cache[i];
            cstack_cache[i] = (PyCStackObject *) stack->startaddr;
            PyObject_Del(stack);
        }
    }
    cstack_cachecount = 0;
}

static void
cstack_dealloc(PyCStackObject *cst)
{
    slp_cstack_chain = cst;
    SLP_CHAIN_REMOVE(PyCStackObject, &slp_cstack_chain, cst, next,
                     prev);
    if (Py_SIZE(cst) >= CSTACK_SLOTS) {
        PyObject_Del(cst);
    }
    else {
        if (cstack_cachecount >= CSTACK_MAXCACHE)
            slp_cstack_cacheclear();
    cst->startaddr = (intptr_t *) cstack_cache[Py_SIZE(cst)];
        cstack_cache[Py_SIZE(cst)] = cst;
        ++cstack_cachecount;
    }
}


PyCStackObject *
slp_cstack_new(PyCStackObject **cst, intptr_t *stackref, PyTaskletObject *task)
{
    PyThreadState *ts;
    intptr_t *stackbase;
    ptrdiff_t size;

    ts = NULL;
    if (task && task->cstate) {
        /* usually a tasklet with a cstate has a valid
           tstate, but during shutdown (function slp_kill_tasks_with_stacks)
           the tstate may be NULL. The exact conditions are complicated. */
        ts = task->cstate->tstate;
    }
    if (ts == NULL) {
        ts = PyThreadState_GET();
    }

    stackbase = ts->st.cstack_base;
    size = stackbase - stackref;

    assert(size == 0 || ts == PyThreadState_GET());
    assert(size >= 0);

    if (*cst != NULL) {
        if ((*cst)->task == task)
            (*cst)->task = NULL;
        Py_DECREF(*cst);
    }
    if (size < CSTACK_SLOTS && ((*cst) = cstack_cache[size])) {
        /* take stack from cache */
        cstack_cache[size] = (PyCStackObject *) (*cst)->startaddr;
        --cstack_cachecount;
        _Py_NewReference((PyObject *)(*cst));
    }
    else
        *cst = PyObject_NewVar(PyCStackObject, &PyCStack_Type, size);
    if (*cst == NULL) return NULL;

    (*cst)->startaddr = stackbase;
    (*cst)->next = (*cst)->prev = NULL;
    SLP_CHAIN_INSERT(PyCStackObject, &slp_cstack_chain, *cst, next, prev);
    (*cst)->serial = ts->st.serial_last_jump;
    (*cst)->task = task;
    (*cst)->tstate = ts;
    (*cst)->nesting_level = ts->st.nesting_level;
#ifdef _SEH32
    //save the SEH handler
    (*cst)->exception_list = 0;
#endif
    return *cst;
}

size_t
slp_cstack_save(PyCStackObject *cstprev)
{
    size_t stsizeb = Py_SIZE(cstprev) * sizeof(intptr_t);

    memcpy((cstprev)->stack, (cstprev)->startaddr -
                             Py_SIZE(cstprev), stsizeb);
#ifdef _SEH32
    //save the SEH handler
    cstprev->exception_list = (DWORD)
                __readfsdword(FIELD_OFFSET(NT_TIB, ExceptionList));
    assert(cstprev->exception_list);
#endif
    return stsizeb;
}

void
#ifdef _SEH32
#pragma warning(disable:4733) /* disable warning about modifying FS[0] */
#endif
slp_cstack_restore(PyCStackObject *cst)
{
    cst->tstate->st.nesting_level = cst->nesting_level;
    /* mark task as no longer responsible for cstack instance */
    cst->task = NULL;
    memcpy(cst->startaddr - Py_SIZE(cst), &cst->stack,
           Py_SIZE(cst) * sizeof(intptr_t));
#ifdef _SEH32
    //restore the SEH handler
    assert(cst->exception_list);
    __writefsdword(FIELD_OFFSET(NT_TIB, ExceptionList), (DWORD)(cst->exception_list));
    #pragma warning(default:4733)
#endif
}


static char cstack_doc[] =
"A CStack object serves to save the stack slice which is involved\n\
during a recursive Python(r) call. It will also be used for pickling\n\
of program state. This structure is highly platform dependant.\n\
Note: For inspection, str() can dump it as a string.\
";

#if SIZEOF_VOIDP == SIZEOF_INT
#define T_ADDR T_UINT
#else
#define T_ADDR T_ULONG
#endif


static PyMemberDef cstack_members[] = {
    {"size", T_INT, offsetof(PyCStackObject, ob_base.ob_size), READONLY},
    {"next", T_OBJECT, offsetof(PyCStackObject, next), READONLY},
    {"prev", T_OBJECT, offsetof(PyCStackObject, prev), READONLY},
    {"task", T_OBJECT, offsetof(PyCStackObject, task), READONLY},
    {"startaddr", T_ADDR, offsetof(PyCStackObject, startaddr), READONLY},
    {0}
};

/* simple string interface for inspection */

static PyObject *
cstack_str(PyObject *o)
{
    PyCStackObject *cst = (PyCStackObject*)o;
    return PyUnicode_FromStringAndSize((char*)&cst->stack,
        Py_SIZE(cst)*sizeof(cst->stack[0]));
}

PyTypeObject PyCStack_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    "_stackless.cstack",
    sizeof(PyCStackObject),
    sizeof(PyObject *),
    (destructor)cstack_dealloc,         /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    (reprfunc)cstack_str,               /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    PyObject_GenericSetAttr,            /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    cstack_doc,                         /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
    cstack_members,                     /* tp_members */
};


static int
make_initial_stub(void)
{
    PyThreadState *ts = PyThreadState_GET();
    int result;

    if (ts->st.initial_stub != NULL) {
        Py_DECREF(ts->st.initial_stub);
        ts->st.initial_stub = NULL;
    }
    ts->st.serial_last_jump = ++ts->st.serial;
    result = slp_transfer(&ts->st.initial_stub, NULL, NULL);
    if (result < 0)
        return result;
    /*
     * from here, we always arrive with a compatible cstack
     * that also can be used by main, if it is running
     * in soft-switching mode.
     * To insure that, it was necessary to re-create the
     * initial stub for *every* run of a new main.
     * This will vanish with greenlet-like stack management.
     */

    return result;
}

static PyObject *
climb_stack_and_eval_frame(PyFrameObject *f)
{
    /*
     * a similar case to climb_stack_and_transfer,
     * but here we need to incorporate a gap in the
     * stack into main and keep this gap on the stack.
     * This way, initial_stub is always valid to be
     * used to return to the main c stack.
     */
    PyThreadState *ts = PyThreadState_GET();
    intptr_t probe;
    ptrdiff_t needed = &probe - ts->st.cstack_base;
    /* in rare cases, the need might have vanished due to the recursion */
    intptr_t *goobledigoobs;
    if (needed > 0) {
    goobledigoobs = alloca(needed * sizeof(intptr_t));
        if (goobledigoobs == NULL)
            return NULL;
    }
    return slp_eval_frame(f);
}


PyObject *
slp_eval_frame(PyFrameObject *f)
{
    PyThreadState *ts = PyThreadState_GET();
    PyFrameObject *fprev = f->f_back;
    intptr_t * stackref;

    if (fprev == NULL && ts->st.main == NULL) {
        int returning;
        /* this is the initial frame, so mark the stack base */

        /*
         * careful, this caused me a major headache.
         * it is *not* sufficient to just check for fprev == NULL.
         * Reason: (observed with wxPython):
         * A toplevel frame is run as a tasklet. When its frame
         * is deallocated (in tasklet_end), a Python(r) object
         * with a __del__ method is destroyed. This __del__
         * will run as a toplevel frame, with f_back == NULL!
         */

        stackref = STACK_REFPLUS + (intptr_t *) &f;
        if (ts->st.cstack_base == NULL)
            ts->st.cstack_base = stackref - CSTACK_GOODGAP;
        if (stackref > ts->st.cstack_base)
            return climb_stack_and_eval_frame(f);

        returning = make_initial_stub();
        if (returning < 0)
            return NULL;
        /* returning will be 1 if we "switched back" to this stub, and 0
         * if this is the original call that just created the stub.
         * If the stub is being reused, the argument, i.e. the frame,
         * is in ts->frame
         */
        if (returning == 1) {
            f = ts->frame;
            ts->frame = NULL;
        }
        return slp_run_tasklet(f);
    }
    Py_INCREF(Py_None);
    return slp_frame_dispatch(f, fprev, 0, Py_None);
}

static void
kill_pending_current_main_and_watchdogs(PyThreadState *ts)
{
    PyTaskletObject *t;

    assert(ts != PyThreadState_GET());  /* don't kill ourself */

    /* kill watchdogs */
    if (ts->st.watchdogs && PyList_CheckExact(ts->st.watchdogs)) {
        Py_ssize_t i;
        /* we don't kill the "intterupt" slot, number 0 */
        for(i = PyList_GET_SIZE(ts->st.watchdogs) - 1; i > 0; i--) {
            PyObject * item = PyList_GET_ITEM(ts->st.watchdogs, i);
            assert(item && PyTasklet_Check(item));
            t = (PyTaskletObject *) item;
            Py_INCREF(t);  /* it is a borrowed ref */
            PyTasklet_KillEx(t, 1);
            PyErr_Clear();
            Py_DECREF(t);
        }
    }
    /* kill main */
    t = ts->st.main;
    if (t != NULL) {
        Py_INCREF(t);  /* it is a borrowed ref */
        PyTasklet_KillEx(t, 1);
        PyErr_Clear();
        Py_DECREF(t);
    }
    /* kill current */
    t = ts->st.current;
    if (t != NULL) {
        Py_INCREF(t);  /* it is a borrowed ref */
        PyTasklet_KillEx(t, 1);
        PyErr_Clear();
        Py_DECREF(t);
    }
}

static void
run_other_threads(PyObject **sleep, int count)
{
    if (count == 0) {
        /* shortcut */
        return;
    }

    assert(sleep != NULL);
    if (*sleep == NULL) {
        /* get a reference to time.sleep() */
        PyObject *mod_time;
        assert(Py_IsInitialized());
        mod_time = PyImport_ImportModule("time");
        if (mod_time != NULL) {
            *sleep = PyObject_GetAttrString(mod_time, "sleep");
            Py_DECREF(mod_time);
        }
        if (*sleep == NULL) {
            PyErr_Clear();
        }
    }
    while(count-- > 0) {
        if (*sleep == NULL) {
            Py_BEGIN_ALLOW_THREADS
            Py_END_ALLOW_THREADS
        } else {
            PyObject *res = PyObject_CallFunction(*sleep, "(f)", (float)0.001);
            if (res != NULL) {
                Py_DECREF(res);
            } else {
                PyErr_Clear();
            }
        }
    }
}

/* a thread (or threads) is exiting.  After this call, no tasklet may
 * refer to target_ts, if target_ts != NULL.
 * Also inactivate all other threads during interpreter shut down (target_ts == NULL).
 * Later in the shutdown sequence Python clears the tstate structure. This
 * causes access violations, if another thread is still active.
 */
void slp_kill_tasks_with_stacks(PyThreadState *target_ts)
{
    PyThreadState *cts = PyThreadState_GET();
    int in_loop = 0;

    if (target_ts == NULL || target_ts == cts) {
        /* a loop to kill tasklets on the local thread */
        while (1) {
            PyCStackObject *csfirst = slp_cstack_chain, *cs;
            PyTaskletObject *t;

            if (csfirst == NULL)
                goto other_threads;
            for (cs = csfirst; ; cs = cs->next) {
                if (in_loop && cs == csfirst) {
                    /* nothing found */
                    goto other_threads;
                }
                in_loop = 1;
                /* has tstate already been cleared or is it a foreign thread? */
                if (cs->tstate != cts)
                    continue;

                if (cs->task == NULL) {
                    cs->tstate = NULL;
                    continue;
                }
                /* is it already dead? */
                if (cs->task->f.frame == NULL) {
                    cs->tstate = NULL;
                    continue;
                }
                break;
            }
            in_loop = 0;
            t = cs->task;
            Py_INCREF(t); /* cs->task is a borrowed ref */
            assert(t->cstate == cs);

            /* If a thread ends, the thread no longer has a main tasklet and
             * the thread is not in a valid state. tstate->st.current is
             * undefined. It may point to a tasklet, but the other fields in
             * tstate have wrong values.
             *
             * Therefore we need to ensure, that t is not tstate->st.current.
             * Convert t into a free floating tasklet. PyTasklet_Kill works
             * for floating tasklets too.
             */
            if (t->next && !t->flags.blocked) {
                assert(t->prev);
                slp_current_remove_tasklet(t);
                assert(Py_REFCNT(t) > 1);
                Py_DECREF(t);
                assert(t->next == NULL);
                assert(t->prev == NULL);
            }
            assert(t != cs->tstate->st.current);

            /* has the tasklet nesting_level > 0? The Stackles documentation
             * specifies: "When a thread dies, only tasklets with a C-state are actively killed.
             * Soft-switched tasklets simply stop."
             */
            if ((cts->st.current == cs->task ? cts->st.nesting_level : cs->nesting_level) > 0) {
                /* Is is hard switched. */
                PyTasklet_Kill(t);
                PyErr_Clear();
            }

            /* must clear the tstate */
            t->cstate->tstate = NULL;
            Py_DECREF(t);
        } /* while(1) */
    } /* if(...) */

other_threads:
    /* and a separate simple loop to kill tasklets on foreign threads.
     * Since foreign tasklets are scheduled in their own good time,
     * there is no guarantee that they are actually dead when we
     * exit this function.  Therefore, we also can't clear their thread
     * states.  That will hopefully happen when their threads exit.
     */
    {
        PyCStackObject *csfirst, *cs;
        PyTaskletObject *t;
        PyObject *sleepfunc = NULL;
        int count;

        /* other threads, first pass: kill (pending) current, main and watchdog tasklets */
        if (target_ts == NULL) {
            PyThreadState *ts;
            count = 0;
            for (ts = cts->interp->tstate_head; ts != NULL; ts = ts->next) {
                if (ts != cts) {
                    /* Inactivate thread ts. In case the thread is active,
                     * it will be killed. If the thread is sleping, it
                     * continues to sleep.
                     */
                    count++;
                    kill_pending_current_main_and_watchdogs(ts);
                    /* It helps to inactivate threads reliably */
                    if (PyExc_TaskletExit)
                        PyThreadState_SetAsyncExc(ts->thread_id, PyExc_TaskletExit);
                }
            }
            /* We must not release the GIL while we might hold the HEAD-lock.
             * Otherwise another thread (usually the thread of the killed tasklet)
             * could try to get the HEAD lock. The result would be a wonderful dead lock.
             * If target_ts is NULL, we know for sure, that we don't hold the HEAD-lock.
             */
            run_other_threads(&sleepfunc, count);
            /* The other threads might have modified the thread state chain, but fortunately we
             * are done with it.
             */
        } else if (target_ts != cts) {
            kill_pending_current_main_and_watchdogs(target_ts);
            /* Here it is not safe to release the GIL. */
        }

        /* other threads, second pass: kill tasklets with nesting-level > 0 and
         * clear tstate if target_ts != NULL && target_ts != cts. */
        csfirst = slp_cstack_chain;
        if (csfirst == NULL) {
            Py_XDECREF(sleepfunc);
            return;
        }

        count = 0;
        in_loop = 0;
        for (cs = csfirst; !(in_loop && cs == csfirst); cs = cs->next) {
            in_loop = 1;
            t = cs->task;
            if (t == NULL)
                continue;
            Py_INCREF(t);  /* cs->task is a borrowed ref */
            assert(t->cstate == cs);
            if (cs->tstate == cts) {
                Py_DECREF(t);
                continue;  /* already handled */
            }
            if (target_ts != NULL && cs->tstate != target_ts) {
                Py_DECREF(t);
                continue;  /* we are not interested in this thread */
            }
            if (((cs->tstate && cs->tstate->st.current == t) ? cs->tstate->st.nesting_level : cs->nesting_level) > 0) {
                /* Kill only tasklets with nesting level > 0 */
                count++;
                PyTasklet_Kill(t);
                PyErr_Clear();
            }
            Py_DECREF(t);
            if (target_ts != NULL) {
                cs->tstate = NULL;
            }
        }
        if (target_ts == NULL) {
            /* We must not release the GIL while we might hold the HEAD-lock.
             * Otherwise another thread (usually the thread of the killed tasklet)
             * could try to get the HEAD lock. The result would be a wonderful dead lock.
             * If target_ts is NULL, we know for sure, that we don't hold the HEAD-lock.
             */
            run_other_threads(&sleepfunc, count);
        }
        Py_XDECREF(sleepfunc);
    }
}

void PyStackless_kill_tasks_with_stacks(int allthreads)
{
    PyThreadState *ts = PyThreadState_Get();

    if (ts->st.main == NULL) {
        if (initialize_main_and_current()) {
            PyObject *s = PyUnicode_FromString("tasklet cleanup");
            PyErr_WriteUnraisable(s);
            Py_XDECREF(s);
            return;
        }
    }
    slp_kill_tasks_with_stacks(allthreads ? NULL : ts);
}


/* cstack spilling for recursive calls */

static PyObject *
eval_frame_callback(PyFrameObject *f, int exc, PyObject *retval)
{
    PyThreadState *ts = PyThreadState_GET();
    PyTaskletObject *cur = ts->st.current;
    PyCStackObject *cst;
    PyCFrameObject *cf = (PyCFrameObject *) f;
    intptr_t *saved_base;

    //make sure we don't try softswitching out of this callstack
    ts->st.nesting_level = cf->n + 1;
    ts->frame = f->f_back;

    //this tasklet now runs in this tstate.
    cst = cur->cstate; //The calling cstate
    cur->cstate = ts->st.initial_stub;
    Py_INCREF(cur->cstate);

    /* We must base our new stack from here, because otherwise we might find
     * ourselves in an infinite loop of stack spilling.
     */
    saved_base = ts->st.cstack_root;
    ts->st.cstack_root = STACK_REFPLUS + (intptr_t *) &f;

    /* pull in the right retval and tempval from the arguments */
    Py_DECREF(retval);
    retval = cf->ob1;
    cf->ob1 = NULL;
    TASKLET_SETVAL_OWN(cur, cf->ob2);
    cf->ob2 = NULL;

    retval = PyEval_EvalFrameEx_slp(ts->frame, exc, retval);
    ts->st.cstack_root = saved_base;

    /* store retval back into the cstate object */
    if (retval == NULL)
        retval = slp_curexc_to_bomb();
    if (retval == NULL)
        goto fatal;
    cf->ob1 = retval;

    /* jump back */
    Py_DECREF(cur->cstate);
    cur->cstate = cst;
    slp_transfer_return(cst);
    /* should never come here */
fatal:
    Py_DECREF(cf); /* since the caller won't do it */
    return NULL;
}

PyObject *
slp_eval_frame_newstack(PyFrameObject *f, int exc, PyObject *retval)
{
    PyThreadState *ts = PyThreadState_GET();
    PyTaskletObject *cur = ts->st.current;
    PyCFrameObject *cf = NULL;
    PyCStackObject *cst;

    if (cur == NULL || PyErr_Occurred()) {
        /* Bypass this during early initialization or if we have a pending
         * exception, such as the one set via gen_close().  Doing the stack
         * magic here will clear that exception.
         */
        intptr_t *old = ts->st.cstack_root;
        ts->st.cstack_root = STACK_REFPLUS + (intptr_t *) &f;
        retval = PyEval_EvalFrameEx_slp(f,exc, retval);
        ts->st.cstack_root = old;
        return retval;
    }
    if (ts->st.cstack_root == NULL) {
        /* this is a toplevel call.  Store the root of stack spilling */
        ts->st.cstack_root = STACK_REFPLUS + (intptr_t *) &f;
        retval = PyEval_EvalFrameEx_slp(f, exc, retval);
        /* and reset it.  We may reenter stackless at a completely different
         * depth later
         */
        ts->st.cstack_root = NULL;
        return retval;
    }

    ts->frame = f;
    cf = slp_cframe_new(eval_frame_callback, 1);
    if (cf == NULL)
        return NULL;
    cf->n = ts->st.nesting_level;
    cf->ob1 = retval;
    /* store the tmpval here so that it won't get clobbered
     * by slp_run_tasklet()
     */
    TASKLET_CLAIMVAL(cur, &(cf->ob2));
    ts->frame = (PyFrameObject *) cf;
    cst = cur->cstate;
    cur->cstate = NULL;
    if (slp_transfer(&cur->cstate, NULL, cur) < 0)
        goto finally; /* fatal */
    Py_XDECREF(cur->cstate);

    retval = cf->ob1;
    cf->ob1 = NULL;
    if (PyBomb_Check(retval))
        retval = slp_bomb_explode(retval);
finally:
    Py_DECREF(cf);
    cur->cstate = cst;
    return retval;
}

/******************************************************

  Generator re-implementation for Stackless

*******************************************************/

typedef struct {
    PyObject_HEAD
    /* The gi_ prefix is intended to remind of generator-iterator. */

    PyFrameObject *gi_frame;

    /* True if generator is being executed. */
    char gi_running;

    /* List of weak reference. */
    PyObject *gi_weakreflist;
} genobject;

/*
 * Note:
 * Generators are quite a bit slower in Stackless, because
 * we are jumping in and out so much.
 * I had an implementation with no extra cframe, but it
 * was not faster, but considerably slower than this solution.
 */

PyObject* gen_iternext_callback(PyFrameObject *f, int exc, PyObject *result);

PyObject *
slp_gen_send_ex(PyGenObject *ob, PyObject *arg, int exc)
{
    STACKLESS_GETARG();
    genobject *gen = (genobject *) ob;
    PyThreadState *ts = PyThreadState_GET();
    PyFrameObject *f = gen->gi_frame;
    PyFrameObject *stopframe = ts->frame;
    PyObject *retval;

    if (gen->gi_running) {
        PyErr_SetString(PyExc_ValueError,
                        "generator already executing");
        return NULL;
    }
    if (f==NULL || f->f_stacktop == NULL) {
        /* Only set exception if called from send() */
        if (arg && !exc)
            PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }

    if (f->f_back == NULL &&
        (f->f_back = (PyFrameObject *)
                 slp_cframe_new(gen_iternext_callback, 0)) == NULL)
        return NULL;

    if (f->f_lasti == -1) {
        if (arg && arg != Py_None) {
            PyErr_SetString(PyExc_TypeError,
                            "can't send non-None value to a "
                            "just-started generator");
            return NULL;
        }
    } else {
        /* Push arg onto the frame's value stack */
        retval = arg ? arg : Py_None;
        Py_INCREF(retval);
        *(f->f_stacktop++) = retval;
    }

    /* Generators always return to their most recent caller, not
     * necessarily their creator. */
    Py_XINCREF(ts->frame);
    assert(f->f_back != NULL);
    assert(f->f_back->f_back == NULL);
    f->f_back->f_back = ts->frame;

    gen->gi_running = 1;

    f->f_execute = PyEval_EvalFrameEx_slp;

    /* make refcount compatible to frames for tasklet unpickling */
    Py_INCREF(f->f_back);

    Py_INCREF(gen);
    Py_XINCREF(arg);
    ((PyCFrameObject *) f->f_back)->ob1 = (PyObject *) gen;
    ((PyCFrameObject *) f->f_back)->ob2 = arg;
    Py_INCREF(f);
    ts->frame = f;

    retval = Py_None;
    Py_INCREF(retval);

    if (stackless)
        return STACKLESS_PACK(retval);
    return slp_frame_dispatch(f, stopframe, exc, retval);
}

PyObject*
gen_iternext_callback(PyFrameObject *f, int exc, PyObject *result)
{
    PyThreadState *ts = PyThreadState_GET();
    PyCFrameObject *cf = (PyCFrameObject *) f;
    genobject *gen = (genobject *) cf->ob1;
    PyObject *arg = cf->ob2;

    gen->gi_running = 0;
    /* make refcount compatible to frames for tasklet unpickling */
    Py_DECREF(f);

    /* Don't keep the reference to f_back any longer than necessary.  It
     * may keep a chain of frames alive or it could create a reference
     * cycle. */
    ts->frame = f->f_back;
    Py_XDECREF(f->f_back);
    f->f_back = NULL;

    f = gen->gi_frame;
    /* If the generator just returned (as opposed to yielding), signal
     * that the generator is exhausted. */
	if (result && f->f_stacktop == NULL) {
        if (result == Py_None) {
            /* Delay exception instantiation if we can */
            PyErr_SetNone(PyExc_StopIteration);
        } else {
            PyObject *e = PyObject_CallFunctionObjArgs(
                               PyExc_StopIteration, result, NULL);
            if (e != NULL) {
                PyErr_SetObject(PyExc_StopIteration, e);
                Py_DECREF(e);
            }
        }
        Py_CLEAR(result);
    }

    /* We hold references to things in the cframe, if we release it
       before we clear the references, they get incorrectly and
       prematurely free. */
    cf->ob1 = NULL;
    cf->ob2 = NULL;

    if (!result || f->f_stacktop == NULL) {
        /* generator can't be rerun, so release the frame */
        /* first clean reference cycle through stored exception traceback */
        PyObject *t, *v, *tb;
        t = f->f_exc_type;
        v = f->f_exc_value;
        tb = f->f_exc_traceback;
        f->f_exc_type = NULL;
        f->f_exc_value = NULL;
        f->f_exc_traceback = NULL;
        Py_XDECREF(t);
        Py_XDECREF(v);
        Py_XDECREF(tb);
        f->f_gen = NULL;
        gen->gi_frame = NULL;
        Py_DECREF(f);
    }

    Py_DECREF(gen);
    Py_XDECREF(arg);
    return result;
}


/******************************************************

  Rebirth of software stack avoidance

*******************************************************/

static PyObject *
unwind_repr(PyObject *op)
{
    return PyUnicode_FromString(
        "The invisible unwind token. If you ever should see this,\n"
        "please report the error to tismer@tismer.com"
    );
}

/* dummy deallocator, just in case */
static void unwind_dealloc(PyObject *op) {
}

static PyTypeObject PyUnwindToken_Type = {
    PyObject_HEAD_INIT(&PyUnwindToken_Type)
    "UnwindToken",
    0,
    0,
    (destructor)unwind_dealloc, /*tp_dealloc*/ /*should never be called*/
    0,                  /*tp_print*/
    0,                  /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_compare*/
    (reprfunc)unwind_repr, /*tp_repr*/
    0,                  /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    0,                  /*tp_hash */
};

static PyUnwindObject unwind_token = {
    PyObject_HEAD_INIT(&PyUnwindToken_Type)
    NULL
};

PyUnwindObject *Py_UnwindToken = &unwind_token;

/*
    the frame dispatcher will execute frames and manage
    the frame stack until the "previous" frame reappears.
    The "Mario" code if you know that game :-)
 */

PyObject *
slp_frame_dispatch(PyFrameObject *f, PyFrameObject *stopframe, int exc, PyObject *retval)
{
    PyThreadState *ts = PyThreadState_GET();

    ++ts->st.nesting_level;

/*
    frame protocol:
    If a frame returns the Py_UnwindToken object, this
    indicates that a different frame will be run.
    Semantics of an appearing Py_UnwindToken:
    The true return value is in its tempval field.
    We always use the topmost tstate frame and bail
    out when we see the frame that issued the
    originating dispatcher call (which may be a NULL frame).
 */

    while (1) {
        retval = f->f_execute(f, exc, retval);
        if (STACKLESS_UNWINDING(retval))
            STACKLESS_UNPACK(retval);
        /* A soft switch is only complete here */
        Py_CLEAR(ts->st.del_post_switch);
        f = ts->frame;
        if (f == stopframe)
            break;
        exc = 0;
    }
    --ts->st.nesting_level;
    /* see whether we need to trigger a pending interrupt */
    /* note that an interrupt handler guarantees current to exist */
    if (ts->st.interrupt != NULL &&
        ts->st.current->flags.pending_irq)
        slp_check_pending_irq();
    return retval;
}

PyObject *
slp_frame_dispatch_top(PyObject *retval)
{
    PyThreadState *ts = PyThreadState_GET();
    PyFrameObject *f = ts->frame;

    if (f==NULL) return retval;

    while (1) {

        retval = f->f_execute(f, 0, retval);
        if (STACKLESS_UNWINDING(retval))
            STACKLESS_UNPACK(retval);
        /* A soft switch is only complete here */
        Py_CLEAR(ts->st.del_post_switch);
        f = ts->frame;
        if (f == NULL)
            break;
    }
    return retval;
}

/* Clear out the free list */

void
slp_stacklesseval_fini(void)
{
    slp_cstack_cacheclear();
}

#endif /* STACKLESS */

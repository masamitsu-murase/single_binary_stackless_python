#ifndef STACKLESS_IMPL_H
#define STACKLESS_IMPL_H

#include "Python.h"

#ifdef STACKLESS

#ifdef __cplusplus
extern "C" {
#endif

#include "structmember.h"
#include "compile.h"
#include "frameobject.h"

#include "core/stackless_structs.h"
#include "pickling/prickelpit.h"

/* CPython added these two macros in object.h for 2.7 and 3.5 */
#ifndef Py_SETREF
#define Py_SETREF(op, op2)                      \
    do {                                        \
        PyObject *_py_tmp = (PyObject *)(op);   \
        (op) = (op2);                           \
        Py_DECREF(_py_tmp);                     \
    } while (0)
#endif

#ifndef Py_XSETREF
#define Py_XSETREF(op, op2)                     \
    do {                                        \
        PyObject *_py_tmp = (PyObject *)(op);   \
        (op) = (op2);                           \
        Py_XDECREF(_py_tmp);                    \
    } while (0)
#endif

#undef STACKLESS_SPY
/*
 * if a platform wants to support self-inspection via _peek,
 * it must provide a function or macro CANNOT_READ_MEM(adr, len)
 * which allows to spy at memory without causing exceptions.
 * This would usually be done in place with the assembly macros.
 */

/********************************************************************
 *
 * This section defines/references stuff from stacklesseval.c
 *
 ********************************************************************/

/*** access to system-wide globals from stacklesseval.c ***/

/* variables for the stackless protocol */
PyAPI_DATA(int) slp_enable_softswitch;
PyAPI_DATA(int) slp_try_stackless;

extern PyCStackObject * slp_cstack_chain;

PyCStackObject * slp_cstack_new(PyCStackObject **cst, intptr_t *stackref, PyTaskletObject *task);
size_t slp_cstack_save(PyCStackObject *cstprev);
void slp_cstack_restore(PyCStackObject *cst);

int slp_transfer(PyCStackObject **cstprev, PyCStackObject *cst, PyTaskletObject *prev);

#ifdef Py_DEBUG
int slp_transfer_return(PyCStackObject *cst);
#else
#define slp_transfer_return(cst) \
                slp_transfer(NULL, (cst), NULL)
#endif

PyAPI_FUNC(int) _PyStackless_InitTypes(void);
PyAPI_FUNC(void) _PyStackless_Init(void);

/* clean-up up at the end */

void slp_stacklesseval_fini(void);
void slp_scheduling_fini(void);
void slp_cframe_fini(void);

void PyStackless_Fini(void);

void PyStackless_kill_tasks_with_stacks(int allthreads);

/* the special version of eval_frame */
PyObject * slp_eval_frame(struct _frame *f);

/* the frame dispatcher */
PyObject * slp_frame_dispatch(PyFrameObject *f,
                              PyFrameObject *stopframe, int exc,
                              PyObject *retval);

/* the frame dispatcher for toplevel tasklets */
PyObject * slp_frame_dispatch_top(PyObject *retval);

/* the now exported eval_frame */
PyAPI_FUNC(PyObject *) PyEval_EvalFrameEx_slp(struct _frame *, int, PyObject *);

/* eval_frame with stack overflow, triggered there with a macro */
PyObject * slp_eval_frame_newstack(struct _frame *f, int throwflag, PyObject *retval);

/* the new eval_frame loop with or without value or resuming an iterator
   or setting up or cleaning up a with block */
PyObject * slp_eval_frame_value(struct _frame *f,  int throwflag, PyObject *retval);
PyObject * slp_eval_frame_noval(struct _frame *f,  int throwflag, PyObject *retval);
PyObject * slp_eval_frame_iter(struct _frame *f,  int throwflag, PyObject *retval);
PyObject * slp_eval_frame_setup_with(struct _frame *f,  int throwflag, PyObject *retval);
PyObject * slp_eval_frame_with_cleanup(struct _frame *f,  int throwflag, PyObject *retval);
/* other eval_frame functions from module/scheduling.c */
PyObject * slp_restore_exception(PyFrameObject *f, int exc, PyObject *retval);
PyObject * slp_restore_tracing(PyFrameObject *f, int exc, PyObject *retval);
/* other eval_frame functions from Objects/typeobject.c */
PyObject * slp_tp_init_callback(PyFrameObject *f, int exc, PyObject *retval);

/* rebirth of software stack avoidance */

typedef struct {
    PyObject_HEAD
    PyObject *tempval;
} PyUnwindObject;

PyAPI_DATA(PyUnwindObject *) Py_UnwindToken;

/* frame cloning both needed in tasklets and generators */

struct _frame * slp_clone_frame(struct _frame *f);
struct _frame * slp_ensure_new_frame(struct _frame *f);

/* exposing some hidden types */
PyObject * slp_gen_send_ex(PyGenObject *gen, PyObject *arg, int exc);

PyAPI_DATA(PyTypeObject) PyMethodDescr_Type;
PyAPI_DATA(PyTypeObject) PyClassMethodDescr_Type;

#define PyMethodWrapper_Check(op) PyObject_TypeCheck(op, &_PyMethodWrapper_Type)

/* access to the current watchdog tasklet */
PyTaskletObject * slp_get_watchdog(PyThreadState *ts, int interrupt);


/* fast (release) and safe (debug) access to the unwind token and retval */

#ifdef Py_DEBUG

#define STACKLESS_PACK(retval) \
    (assert(Py_UnwindToken->tempval == NULL), \
     Py_UnwindToken->tempval = (retval), \
     (PyObject *) Py_UnwindToken)

#define STACKLESS_UNPACK(retval) \
    ((void)(assert(STACKLESS_UNWINDING(retval)), \
     retval = Py_UnwindToken->tempval, \
     Py_UnwindToken->tempval = NULL, retval))

#else

#define STACKLESS_PACK(retval) \
    (Py_UnwindToken->tempval = (retval), \
     (PyObject *) Py_UnwindToken)

#define STACKLESS_UNPACK(retval) \
    ((void)(retval = Py_UnwindToken->tempval, retval))

#endif

#define STACKLESS_UNWINDING(obj) \
    ((PyObject *) (obj) == (PyObject *) Py_UnwindToken)

/* an arbitrary positive number */
#define STACKLESS_UNWINDING_MAGIC 0x7fedcba9

#define STACKLESS_RETVAL(obj) \
    (STACKLESS_UNWINDING(obj) ? Py_UnwindToken->tempval : (obj))

#define STACKLESS_ASSERT_UNWINDING_VALUE_IS_NOT(obj, val) \
    assert(!STACKLESS_UNWINDING(obj) || ((Py_UnwindToken->tempval) != (val)))

/* macros for setting/resetting the stackless flag */

#define STACKLESS_GETARG() int stackless = (stackless = slp_try_stackless, \
                           slp_try_stackless = 0, stackless)

#define STACKLESS_PROMOTE(func) \
    (stackless ? slp_try_stackless = \
     (func)->ob_type->tp_flags & Py_TPFLAGS_HAVE_STACKLESS_CALL : 0)

#define STACKLESS_PROMOTE_FLAG(flag) \
    (stackless ? slp_try_stackless = (flag) : 0)

#define STACKLESS_PROMOTE_METHOD(obj, meth) do { \
    if ((obj->ob_type->tp_flags & Py_TPFLAGS_HAVE_STACKLESS_EXTENSION) && \
         obj->ob_type->tp_as_mapping) \
        slp_try_stackless = stackless & obj->ob_type->tp_as_mapping->slpflags.meth; \
} while (0)

#define STACKLESS_PROMOTE_WRAPPER(wp) \
    (slp_try_stackless = stackless & wp->descr->d_slpmask)

#define STACKLESS_PROMOTE_ALL() ((void)(slp_try_stackless = stackless, NULL))

#define STACKLESS_PROPOSE(func) {int stackless = slp_enable_softswitch; \
                 STACKLESS_PROMOTE(func);}

#define STACKLESS_PROPOSE_FLAG(flag) {int stackless = slp_enable_softswitch; \
                      STACKLESS_PROMOTE_FLAG(flag);}

#define STACKLESS_PROPOSE_METHOD(obj, meth) {int stackless = slp_enable_softswitch; \
                 STACKLESS_PROMOTE_METHOD(obj, meth);}

#define STACKLESS_PROPOSE_ALL() slp_try_stackless = slp_enable_softswitch;

#define STACKLESS_RETRACT() slp_try_stackless = 0;

#define STACKLESS_ASSERT() assert(!slp_try_stackless)

/* this is just a tag to denote which methods are stackless */

#define STACKLESS_DECLARE_METHOD(type, meth)

/* This can be set to 1 to completely disable the augmentation of
 * type info with stackless property.  For debugging.
 */
#define STACKLESS_NO_TYPEINFO 0

/*

  How this works:
  There is one global variable slp_try_stackless which is used
  like an implicit parameter. Since we don't have a real parameter,
  the flag is copied into the local variable "stackless" and cleared.
  This is done by the GETARG() macro, which should be added to
  the top of the function's declarations.
  The idea is to keep the chances to introduce error to the minimum.
  A function can safely do some tests and return before calling
  anything, since the flag is in a local variable.
  Depending on context, this flag is propagated to other called
  functions. They *must* obey the protocol. To make this sure,
  the ASSERT() macro has to be called after every such call.

  Many internal functions have been patched to support this protocol.

  GETARG()

    move the slp_try_stackless flag into the local variable "stackless".

  PROMOTE(func)

    if stackless was set and the function's type has set
    Py_TPFLAGS_HAVE_STACKLESS_CALL, then this flag will be
    put back into slp_try_stackless, and we expect that the
    function handles it correctly.

  PROMOTE_FLAG(flag)

    is used for special cases, like PyCFunction objects. PyCFunction_Type
    says that it supports a stackless call, but the final action depends
    on the METH_STACKLESS flag in the object to be called. Therefore,
    PyCFunction_Call uses PROMOTE_FLAG(flags & METH_STACKLESS) to
    take care of PyCFunctions which don't care about it.

    Another example is the "next" method of iterators. To support this,
    the wrapperobject's type has the Py_TPFLAGS_HAVE_STACKLESS_CALL
    flag set, but wrapper_call then examines the wrapper descriptors
    flags if PyWrapperFlag_STACKLESS is set. "next" has it set.
    It also checks whether Py_TPFLAGS_HAVE_STACKLESS_CALL is set
    for the iterator's type.

  PROMOTE_ALL()

    is used for cases where we know that the called function will take
    care of our object, and we need no test. For example, PyObject_Call
    uses PROMOTE, itself, so we don't need to check further.

  ASSERT()

    make sure that slp_try_stackless was cleared. This debug feature
    tries to ensure that no unexpected nonrecursive call can happen.

  Some functions which are known to be stackless by nature
  just use the PROPOSE macros. They do not care about prior state.
  Most of them are used in ceval.c and other contexts which are
  stackless by definition. All possible nonrecursive calls are
  initiated by these macros.

*/


/********************************************************************
 *
 * This section defines/references stuff from stacklessmodule.c
 *
 ********************************************************************/

/* generic ops for chained objects */

/*  Tasklets are in doubly linked lists. We support
    deletion and insertion */

#define SLP_CHAIN_INSERT(__objtype, __chain, __task, __next, __prev) \
do { \
    __objtype *l, *r; \
    assert((__task)->__next == NULL); \
    assert((__task)->__prev == NULL); \
    if (*(__chain) == NULL) { \
        (__task)->__next = (__task)->__prev = (__task); \
        *(__chain) = (__task); \
    } \
    else { \
        /* insert at end */ \
        r = *(__chain); \
        l = r->__prev; \
        l->__next = r->__prev = (__task); \
        (__task)->__prev = l; \
        (__task)->__next = r; \
    } \
} while(0)

#define SLP_CHAIN_REMOVE(__objtype, __chain, __task, __next, __prev) \
do { \
    __objtype *l, *r; \
    if (*(__chain) == NULL) { \
        (__task) = NULL; \
    } \
    else { \
        /* remove current */ \
        (__task) = *(__chain); \
        l = (__task)->__prev; \
        r = (__task)->__next; \
        l->__next = r; \
        r->__prev = l; \
        *(__chain) = r; \
        if (*(__chain)==(__task)) \
            *(__chain) = NULL;  /* short circuit */ \
        (__task)->__prev = NULL; \
        (__task)->__next = NULL; \
    } \
} while(0)

/* these versions operate on an embedded head, which channels use now */

#define SLP_HEADCHAIN_INSERT(__objtype, __chan, __task, __next, __prev) \
do { \
    __objtype *__head = (__objtype *) __chan; \
    assert((__task)->__next == NULL); \
    assert((__task)->__prev == NULL); \
    /* insert at end */ \
    (__task)->__prev = (__head)->__prev; \
    (__task)->__next = (__head); \
    (__head)->__prev->next = (__task); \
    (__head)->__prev = (__task); \
} while(0)

#define SLP_HEADCHAIN_REMOVE(__task, __next, __prev) \
do { \
    assert((__task)->__next != NULL); \
    assert((__task)->__prev != NULL); \
    /* remove at front */ \
    (__task)->__next->__prev = (__task)->prev; \
    (__task)->__prev->__next = (__task)->next; \
    (__task)->__next = (__task)->__prev = NULL; \
} while(0)

/* operations on chains */

void slp_current_insert(PyTaskletObject *task);
void slp_current_insert_after(PyTaskletObject *task);
void slp_current_uninsert(PyTaskletObject *task);
PyTaskletObject * slp_current_remove(void);
void slp_current_remove_tasklet(PyTaskletObject *task);
void slp_current_unremove(PyTaskletObject *task);
void slp_channel_insert(PyChannelObject *channel,
                        PyTaskletObject *task,
                        int dir, PyTaskletObject *next);
PyTaskletObject * slp_channel_remove(PyChannelObject *channel,
                                     PyTaskletObject *task,
                                     int *dir, PyTaskletObject **next);
void slp_channel_remove_slow(PyTaskletObject *task,
                             PyChannelObject **u_chan,
                             int *dir, PyTaskletObject **next);

/* recording the main thread state */
extern PyThreadState * slp_initial_tstate;

/* protecting soft-switched tasklets in other threads */
int slp_ensure_linkage(PyTaskletObject *task);

/* tasklet/scheduling operations */
PyObject * slp_tasklet_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

int slp_schedule_task(PyObject **result,
                      PyTaskletObject *prev,
                      PyTaskletObject *next,
                      int stackless,
                      int *did_switch);

void slp_thread_unblock(PyThreadState *ts);

int slp_initialize_main_and_current(void);

/* setting the tasklet's tempval, optimized for no change */

#define TASKLET_SETVAL(task, val) \
do { \
    if ((task)->tempval != (PyObject *) val) { \
        Py_INCREF(val); \
        TASKLET_SETVAL_OWN(task, val); \
    } \
} while(0)

/* ditto, without incref. Made no sense to optimize. */

#define TASKLET_SETVAL_OWN(task, val) \
do { \
    PyObject *hold = (task)->tempval; \
    assert(val != NULL); \
    (task)->tempval = (PyObject *) val; \
    Py_DECREF(hold); \
} while(0)

/* exchanging values with safety check */

#define TASKLET_SWAPVAL(prev, next) \
do { \
    PyObject *hold = (prev)->tempval; \
    assert((prev)->tempval != NULL); \
    assert((next)->tempval != NULL); \
    (prev)->tempval = (next)->tempval; \
    (next)->tempval = hold; \
} while(0)

/* Get the value and replace it with a None */

#define TASKLET_CLAIMVAL(task, val) \
do { \
    *(val) = (task)->tempval; \
    (task)->tempval = Py_None; \
    Py_INCREF(Py_None); \
} while(0)


/* exception handling */

PyObject * slp_make_bomb(PyObject *klass, PyObject *args, char *msg);
PyObject * slp_exc_to_bomb(PyObject *exc, PyObject *val, PyObject *tb);
PyObject * slp_curexc_to_bomb(void);
PyObject * slp_nomemory_bomb(void);
PyObject * slp_bomb_explode(PyObject *bomb);
int slp_init_bombtype(void);

/* tasklet startup */

PyObject * slp_run_tasklet(PyFrameObject *f);

/* handy abbrevations */

PyObject * slp_type_error(const char *msg);
PyObject * slp_runtime_error(const char *msg);
PyObject * slp_value_error(const char *msg);
PyObject * slp_null_error(void);

/* this seems to be needed for gcc */

/* Define NULL pointer value */

#undef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif

#define TYPE_ERROR(str, ret) return (slp_type_error(str), ret)
#define RUNTIME_ERROR(str, ret) return (slp_runtime_error(str), ret)
#define VALUE_ERROR(str, ret) return (slp_value_error(str), ret)

PyCFrameObject * slp_cframe_new(PyFrame_ExecFunc *exec,
                                unsigned int linked);
PyCFrameObject * slp_cframe_newfunc(PyObject *func,
                                    PyObject *args,
                                    PyObject *kwds,
                                    unsigned int linked);

PyFrameObject * slp_get_frame(PyTaskletObject *task);
void slp_check_pending_irq(void);
int slp_return_wrapper(PyObject *retval);
int slp_return_wrapper_hard(PyObject *retval);
int slp_int_wrapper(PyObject *retval);
int slp_current_wrapper(int(*func)(PyTaskletObject*),
                        PyTaskletObject *task);
int slp_resurrect_and_kill(PyObject *self,
                           void(*killer)(PyObject *));

/* stackless pickling support */

int slp_safe_pickling(int(*save)(PyObject *, PyObject *, int),
                      PyObject *self, PyObject *args,
                     int pers_save);
/* utility function used by the reduce methods of tasklet and frame */
int slp_pickle_with_tracing_state(void);

/* debugging/monitoring */

typedef int (slp_schedule_hook_func) (PyTaskletObject *from,
                                       PyTaskletObject *to);
extern slp_schedule_hook_func* _slp_schedule_fasthook;
extern PyObject* _slp_schedule_hook;
int slp_schedule_callback(PyTaskletObject *prev, PyTaskletObject *next);

int slp_prepare_slots(PyTypeObject*);

Py_tracefunc slp_get_sys_profile_func(void);
Py_tracefunc slp_get_sys_trace_func(void);
int slp_encode_ctrace_functions(Py_tracefunc c_tracefunc, Py_tracefunc c_profilefunc);
PyTaskletTStateStruc * slp_get_saved_tstate(PyTaskletObject *task);

PyObject * slp_get_channel_callback(void);

/* macro for use when interrupting tasklets from watchdog */
#define TASKLET_NESTING_OK(task) \
    (ts->st.nesting_level == 0 || \
     (task)->flags.ignore_nesting || \
     (ts->st.runflags & PY_WATCHDOG_IGNORE_NESTING))


#include "stackless_api.h"

#else /* STACKLESS */

/* turn the stackless flag macros into dummies */

#define STACKLESS_GETARG() int stackless = 0
#define STACKLESS_PROMOTE(func) stackless = 0
#define STACKLESS_PROMOTE_FLAG(flag) stackless = 0
#define STACKLESS_PROMOTE_METHOD(obj, meth) stackless = 0
#define STACKLESS_PROMOTE_WRAPPER(wp) stackless = 0
#define STACKLESS_PROMOTE_ALL() stackless = 0
#define STACKLESS_PROPOSE(func) assert(1)
#define STACKLESS_PROPOSE_FLAG(flag) assert(1)
#define STACKLESS_PROPOSE_ALL() assert(1)
#define STACKLESS_RETRACT() assert(1)
#define STACKLESS_ASSERT() assert(1)

#define STACKLESS_RETVAL(obj) (obj)
#define STACKLESS_ASSERT_UNWINDING_VALUE_IS_NOT(obj, val) assert(1)

#define STACKLESS_DECLARE_METHOD(type, meth)

#endif /* STACKLESS */

#ifdef __cplusplus
}
#endif

#endif

/*
 *
 * Copyright 2018 Anselm Kruis <anselm.kruis@atos.net>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "Python.h"
#include "frameobject.h"
#include "stackless_api.h"
#ifdef MS_WINDOWS
#include "malloc.h" /* for alloca */
#endif

 /******************************************************

 some test functions

 ******************************************************/


PyDoc_STRVAR(test_cframe__doc__,
    "test_cframe(switches, words=0) -- a builtin testing function that does nothing\n\
but tasklet switching. The function will call PyStackless_Schedule() for switches\n\
times and then finish.\n\
If words is given, as many words will be allocated on the C stack.\n\
Usage: Create two tasklets for test_cframe and run them by run().\n\
\n\
    t1 = tasklet(test_cframe)(500000)\n\
    t2 = tasklet(test_cframe)(500000)\n\
    run()\n\
This can be used to measure the execution time of 1.000.000 switches.");

/* we define the stack max as the typical recursion limit
* times the typical number of words per recursion.
* That is 1000 * 64
*/

#define STACK_MAX_USEFUL 64000
#define STACK_MAX_USESTR "64000"

#define VALUE_ERROR(str, ret) return (PyErr_SetString(PyExc_ValueError, (str)), ret)

static
PyObject *
test_cframe(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *argnames[] = { "switches", "words", NULL };
    long switches, extra = 0;
    long i;
    PyObject *ret = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "l|l:test_cframe",
        argnames, &switches, &extra))
        return NULL;
    if (extra < 0 || extra > STACK_MAX_USEFUL)
        VALUE_ERROR("test_cframe: words are limited by 0 and " \
            STACK_MAX_USESTR, NULL);
    if (extra > 0)
        alloca(extra * sizeof(PyObject*));
    Py_INCREF(ret);
    for (i = 0; i<switches; i++) {
        Py_DECREF(ret);
        ret = PyStackless_Schedule(Py_None, 0);
        if (ret == NULL) return NULL;
    }
    return ret;
}


PyDoc_STRVAR(test_cstate__doc__,
    "test_cstate(callable) -- a builtin testing function that simply returns the\n\
result of calling its single argument.  Used for testing to force stackless\n\
into using hard switching.");

static
PyObject *
test_cstate(PyObject *f, PyObject *callable)
{
    return PyObject_CallObject(callable, NULL);
}


PyDoc_STRVAR(test_PyEval_EvalFrameEx__doc__,
    "test_PyEval_EvalFrameEx(code, globals, args=()) -- a builtin testing function.\n\
This function tests the C-API function PyEval_EvalFrameEx(), which is not used\n\
by Stackless Python.\n\
The function creates a frame from code, globals and args and executes the frame.");

static PyObject* test_PyEval_EvalFrameEx(PyObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = { "code", "globals", "args", "alloca", "throw", "oldcython",
        "code2", NULL };
    PyThreadState *tstate = PyThreadState_GET();
    PyCodeObject *co, *code2 = NULL;
    PyObject *globals, *co_args = NULL;
    Py_ssize_t alloca_size = 0;
    PyObject *exc = NULL;
    PyObject *oldcython = NULL;
    PyFrameObject *f;
    PyObject *result = NULL;
    void *p;
    Py_ssize_t na;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!O!|O!nOO!O!:test_PyEval_EvalFrameEx", kwlist,
        &PyCode_Type, &co, &PyDict_Type, &globals, &PyTuple_Type, &co_args, &alloca_size,
        &exc, &PyBool_Type, &oldcython, &PyCode_Type, &code2))
        return NULL;
    if (exc && !PyExceptionInstance_Check(exc)) {
        PyErr_SetString(PyExc_TypeError, "exc must be an exception instance");
        return NULL;
    }
    p = alloca(alloca_size);
    assert(globals != NULL);
    assert(tstate != NULL);
    na = PyTuple_Size(co->co_freevars);
    if (na == -1)
        return NULL;
    if (na > 0) {
        PyErr_Format(PyExc_ValueError, "code requires cell variables");
        return NULL;
    }
    f = PyFrame_New(tstate, co, globals, NULL);
    if (f == NULL) {
        return NULL;
    }
    if (co_args) {
        PyObject **fastlocals;
        Py_ssize_t i;
        na = PyTuple_Size(co_args);
        if (na == -1)
            goto exit;
        if (na > co->co_argcount) {
            PyErr_Format(PyExc_ValueError, "too many items in tuple 'args'");
            goto exit;
        }
        fastlocals = f->f_localsplus;
        if (oldcython == Py_True) {
            /* Use the f_localsplus offset from regular C-Python. Old versions of cython used to
            * access f_localplus directly. Current versions compute the field offset for
            * f_localsplus at run-time.
            */
            fastlocals--;
        }
        for (i = 0; i < na; i++) {
            PyObject *arg = PyTuple_GetItem(co_args, i);
            if (arg == NULL) {
                goto exit;
            }
            Py_INCREF(arg);
            fastlocals[i] = arg;
        }
        if (alloca_size > 0 && na > 0) {
            Py_SETREF(fastlocals[0], PyLong_FromVoidPtr(p));
        }
    }
    if (exc) {
        PyErr_SetObject(PyExceptionInstance_Class(exc), exc);
    }
    if (code2) {
        Py_INCREF(code2);
        Py_SETREF(f->f_code, code2);
    }
    result = PyEval_EvalFrameEx(f, exc != NULL);
    /* result = Py_None; Py_INCREF(Py_None); */
exit:
    ++tstate->recursion_depth;
    Py_DECREF(f);
    --tstate->recursion_depth;
    return result;
}


/* ---------- */


/* List of functions defined in the module */

static PyMethodDef _teststackless_methods[] = {
    { "test_cframe", (PyCFunction) test_cframe, METH_VARARGS | METH_KEYWORDS,
    test_cframe__doc__ },
    { "test_cstate", (PyCFunction) test_cstate, METH_O,
    test_cstate__doc__ },
    { "test_PyEval_EvalFrameEx", (PyCFunction) test_PyEval_EvalFrameEx, METH_VARARGS | METH_KEYWORDS,
    test_PyEval_EvalFrameEx__doc__ },
    {NULL,              NULL}           /* sentinel */
};

PyDoc_STRVAR(module_doc,
"This is a test module for Stackless.");

static PyModuleDef _teststacklessmodule;
static int
_teststackless_exec(PyObject *m)
{
    /* Finalize the type object including setting type of the new type
     * object; doing it here is required for portability, too. */
    if (0 /* add calls here */)
        goto fail;

    return 0;
 fail:
    Py_XDECREF(m);
    return -1;
}

static struct PyModuleDef_Slot _teststackless_slots[] = {
    {Py_mod_exec, _teststackless_exec},
    {0, NULL},
};

static PyModuleDef _teststacklessmodule = {
    PyModuleDef_HEAD_INIT,
    "_teststackless",
    module_doc,
    0,
    _teststackless_methods,
    _teststackless_slots,
    NULL,
    NULL,
    NULL
};

/* Export function for the module (*must* be called PyInit__teststackless) */

PyMODINIT_FUNC
PyInit__teststackless(void)
{
    return PyModuleDef_Init(&_teststacklessmodule);
}

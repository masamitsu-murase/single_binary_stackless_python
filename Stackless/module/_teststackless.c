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
#include "stackless_api.h"


/* ---------- */


/* List of functions defined in the module */

static PyMethodDef _teststackless_methods[] = {
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

#include "Python.h"
#include "structmember.h"
#include "osdefs.h"
#include "marshal.h"

#include "zlib.h"

struct st_embedded_searchorder {
    char suffix[14];
    int package;
};

static struct st_embedded_searchorder embedded_searchorder[] = {
    {"/__init__.py", 1},
    {".py", 0},
    {"", 0}
};


typedef struct _embeddedimporter EmbeddedImporter;

struct _embeddedimporter {
    PyObject_HEAD
    PyObject *dict;  // { filename: (data_offset) }
    PyObject *prefix;
};

extern char embeddedimporter_filename[];
extern const size_t embeddedimporter_raw_data_size;
extern const unsigned char embeddedimporter_raw_data_compressed[];
extern const size_t embeddedimporter_raw_data_compressed_size;
extern const size_t embeddedimporter_data_offset[];
static char *embeddedimporter_raw_data;

static PyObject *EmbeddedImportError;

#define EmbeddedImporter_Check(op) PyObject_TypeCheck(op, &EmbeddedImporter_Type)

static int
construct_filedata(EmbeddedImporter *self)
{
    static PyObject *embeddedimporter_data_dict = NULL;

    PyObject *tuple;
    size_t name_index, file_index;
    uLongf raw_data_size;

    if (embeddedimporter_data_dict != NULL) {
        Py_INCREF(embeddedimporter_data_dict);
        self->dict = embeddedimporter_data_dict;
        return 0;
    }

    embeddedimporter_raw_data = PyMem_Malloc(embeddedimporter_raw_data_size);
    if (embeddedimporter_raw_data == NULL) {
        return -1;
    }

    raw_data_size = embeddedimporter_raw_data_size;
    if (uncompress((Bytef *)embeddedimporter_raw_data, &raw_data_size,
                   embeddedimporter_raw_data_compressed, embeddedimporter_raw_data_compressed_size) != Z_OK
          || raw_data_size != embeddedimporter_raw_data_size) {
        PyMem_Free(embeddedimporter_raw_data);
        return -1;
    }

    self->dict = embeddedimporter_data_dict = PyDict_New();
    if (self->dict == NULL) {
        return -1;
    }

    for (file_index=0, name_index=0; embeddedimporter_filename[name_index]; file_index++) {
        char *name = &embeddedimporter_filename[name_index];

        for (; embeddedimporter_filename[name_index]; name_index++) {
            if (embeddedimporter_filename[name_index] == '/') {
                embeddedimporter_filename[name_index] = SEP;
            }
        }
        name_index++;

        tuple = PyTuple_New(1);
        if (tuple == NULL
            || PyTuple_SetItem(tuple, 0,
                               PyInt_FromSize_t(embeddedimporter_data_offset[file_index])) < 0) {
            Py_XDECREF(tuple);
            return -1;
        }
        if (PyDict_SetItemString(self->dict, name, tuple) < 0) {
            Py_DECREF(tuple);
            return -1;
        }
        Py_DECREF(tuple);
    }

    return 0;
}

static int
embeddedimporter_init(EmbeddedImporter *self, PyObject *args, PyObject *kwds)
{
    char *path;
    char prefix[MAXPATHLEN + 2];
    char program_full_path[MAXPATHLEN + 1];
    size_t len;

    strcpy(program_full_path, Py_GetProgramFullPath());
    len = strlen(program_full_path);

    if (!_PyArg_NoKeywords("embeddedimporter()", kwds)) {
        return -1;
    }

    if (!PyArg_ParseTuple(args, "s:embeddedimporter", &path)) {
        return -1;
    }

    if (strncmp(path, program_full_path, len) != 0 || (path[len] != '\0' && path[len] != SEP)) {
        PyErr_SetString(EmbeddedImportError, "not a executable file");
        return -1;
    }

    if (path[len] == SEP) {
        char *p;
        strcpy(prefix, path + len + 1);
        p = prefix;
        while (*p) {
            if (*p == '.') {
                *p = SEP;
            }
            p++;
        }
        self->prefix = PyString_FromFormat("%s%c", prefix, SEP);
    } else {
        self->prefix = NULL;
    }

    if (construct_filedata(self) < 0) {
        return -1;
    }

    return 0;
}

/* GC support. */
static int
embeddedimporter_traverse(PyObject *obj, visitproc visit, void *arg)
{
    EmbeddedImporter *self = (EmbeddedImporter *)obj;
    Py_VISIT(self->dict);
    if (self->prefix) {
        Py_VISIT(self->prefix);
    }
    return 0;
}

static void
embeddedimporter_dealloc(EmbeddedImporter *self)
{
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->dict);
    Py_XDECREF(self->prefix);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
embeddedimporter_repr(EmbeddedImporter *self)
{
    return PyString_FromString("<embeddedimporter object>");
}

static PyObject *
find_tuple(EmbeddedImporter *self, const char *subname, int *is_package)
{
    char buf[MAXPATHLEN + 1];
    size_t prefix_len, len;
    struct st_embedded_searchorder *eso;
    PyObject *tuple;
    const char *prefix = "";

    if (self->prefix) {
        prefix = PyString_AsString(self->prefix);
    }
    prefix_len = strlen(prefix);
    len = strlen(subname);
    //                  /__init__.py
    if (prefix_len + len + 13 >= MAXPATHLEN) {
        return NULL;
    }

    strcpy(buf, prefix);
    strcpy(buf + prefix_len, subname);
    for (eso = embedded_searchorder; *eso->suffix; eso++) {
        strcpy(buf + prefix_len + len, eso->suffix);
        tuple = PyDict_GetItemString(self->dict, buf);
        if (tuple != NULL) {
            if (is_package != NULL) {
                *is_package = eso->package;
            }
            return tuple;
        }
    }

    return NULL;
}

static char *
get_subname(char *fullname)
{
    char *subname = strrchr(fullname, '.');
    if (subname == NULL) {
        subname = fullname;
    } else {
        subname++;
    }
    return subname;
}

/* Check whether we can satisfy the import of the module named by
   'fullname'. Return self if we can, None if we can't. */
static PyObject *
embeddedimporter_find_module(PyObject *obj, PyObject *args)
{
    EmbeddedImporter *self = (EmbeddedImporter *)obj;
    PyObject *path = NULL;
    char *fullname, *subname;

    if (!PyArg_ParseTuple(args, "s|O:embeddedimporter.find_module",
                          &fullname, &path)) {
        return NULL;
    }

    subname = get_subname(fullname);
    if (find_tuple(self, subname, NULL) == NULL) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    Py_INCREF(self);
    return (PyObject *)self;
}

/* Load and return the module named by 'fullname'. */
static PyObject *
embeddedimporter_load_module(PyObject *obj, PyObject *args)
{
    EmbeddedImporter *self = (EmbeddedImporter *)obj;
    PyObject *tuple, *code, *module, *dict, *modpath;
    char *fullname, *subname;
    int is_package = 0;
    long data_offset;

    if (!PyArg_ParseTuple(args, "s:embeddedimporter.load_module",
                          &fullname)) {
        return NULL;
    }

    subname = get_subname(fullname);
    tuple = find_tuple(self, subname, &is_package);
    if (tuple == NULL) {
        PyErr_SetString(EmbeddedImportError, "not found");
        return NULL;
    }

    data_offset = PyInt_AsLong(PyTuple_GetItem(tuple, 0));
    code = Py_CompileString(&embeddedimporter_raw_data[data_offset], fullname, Py_file_input);
    if (code == NULL) {
        return NULL;
    }

    module = PyImport_AddModule(fullname);
    if (module == NULL) {
        Py_DECREF(code);
        return NULL;
    }

    dict = PyModule_GetDict(module);

    /* mod.__loader__ = self */
    if (PyDict_SetItemString(dict, "__loader__", (PyObject *)self) != 0) {
        goto error;
    }

    if (is_package) {
        /* add __path__ to the module *before* the code gets executed */
        PyObject *pkgpath, *fullpath;
        int err;

        fullpath = PyString_FromFormat("%s%c%s%s",
                                       Py_GetProgramFullPath(),
                                       SEP,
                                       (self->prefix ? PyString_AsString(self->prefix) : ""),
                                       subname);
        if (fullpath == NULL) {
            goto error;
        }

        pkgpath = Py_BuildValue("[O]", fullpath);
        Py_DECREF(fullpath);
        if (pkgpath == NULL) {
            goto error;
        }
        err = PyDict_SetItemString(dict, "__path__", pkgpath);
        Py_DECREF(pkgpath);
        if (err != 0) {
            goto error;
        }
    }

    if (is_package) {
        modpath = PyString_FromFormat("%s%c%s%s%c%s",
                                      Py_GetProgramFullPath(),
                                      SEP,
                                      (self->prefix ? PyString_AsString(self->prefix) : ""),
                                      subname,
                                      SEP,
                                      "__init__.py");
    } else {
        modpath = PyString_FromFormat("%s%c%s%s.py",
                                      Py_GetProgramFullPath(),
                                      SEP,
                                      (self->prefix ? PyString_AsString(self->prefix) : ""),
                                      subname);
    }

    module = PyImport_ExecCodeModuleEx(fullname, code, PyString_AsString(modpath));
    Py_DECREF(modpath);
    Py_DECREF(code);
    if (Py_VerboseFlag)
        PySys_WriteStderr("import %s # loaded from Embedded %s\n",
                          fullname, PyString_AsString(modpath));
    return module;

error:
    Py_DECREF(code);
    Py_DECREF(module);
    return NULL;
}

/* Return a bool signifying whether the module is a package or not. */
static PyObject *
embeddedimporter_is_package(PyObject *obj, PyObject *args)
{
    EmbeddedImporter *self = (EmbeddedImporter *)obj;
    char *fullname, *subname;
    int is_package = 0;
    PyObject *tuple;

    if (!PyArg_ParseTuple(args, "s:embeddedimporter.is_package",
                          &fullname)) {
        return NULL;
    }

    subname = get_subname(fullname);
    tuple = find_tuple(self, subname, &is_package);
    if (tuple == NULL) {
        PyErr_SetString(EmbeddedImportError, "not found");
        return NULL;
    }

    return PyBool_FromLong(is_package);
}

static PyObject *
embeddedimporter_get_code(PyObject *obj, PyObject *args)
{
    EmbeddedImporter *self = (EmbeddedImporter *)obj;
    char *fullname, *subname;
    PyObject *tuple;
    long data_offset;

    if (!PyArg_ParseTuple(args, "s:embeddedimporter.get_code", &fullname)) {
        return NULL;
    }

    subname = get_subname(fullname);
    tuple = find_tuple(self, subname, NULL);
    if (tuple == NULL) {
        PyErr_SetString(EmbeddedImportError, "not found");
        return NULL;
    }

    data_offset = PyInt_AsLong(PyTuple_GetItem(tuple, 0));
    return Py_CompileString(&embeddedimporter_raw_data[data_offset], fullname, Py_file_input);
}

PyDoc_STRVAR(doc_find_module,
"find_module(fullname, path=None) -> self or None.\n\
\n\
Search for a module specified by 'fullname'. 'fullname' must be the\n\
fully qualified (dotted) module name. It returns the embeddedimporter\n\
instance itself if the module was found, or None if it wasn't.\n\
The optional 'path' argument is ignored -- it's there for compatibility\n\
with the importer protocol.");

PyDoc_STRVAR(doc_load_module,
"load_module(fullname) -> module.\n\
\n\
Load the module specified by 'fullname'. 'fullname' must be the\n\
fully qualified (dotted) module name. It returns the imported\n\
module, or raises EmbeddedImportError if it wasn't found.");

PyDoc_STRVAR(doc_is_package,
"is_package(fullname) -> bool.\n\
\n\
Return True if the module specified by fullname is a package.\n\
Raise EmbeddedImportError if the module couldn't be found.");

PyDoc_STRVAR(doc_get_code,
"get_code(fullname) -> code object.\n\
\n\
Return the code object for the specified module. Raise EmbeddedImportError\n\
if the module couldn't be found.");

static PyMethodDef embeddedimporter_methods[] = {
    {"find_module", embeddedimporter_find_module, METH_VARARGS,
     doc_find_module},
    {"load_module", embeddedimporter_load_module, METH_VARARGS,
     doc_load_module},
    {"is_package", embeddedimporter_is_package, METH_VARARGS,
     doc_is_package},
    {"get_code", embeddedimporter_get_code, METH_VARARGS,
     doc_get_code},
    {NULL,              NULL}   /* sentinel */
};

static PyMemberDef embeddedimporter_members[] = {
    {NULL}
};

PyDoc_STRVAR(embeddedimporter_doc,
"embeddedimporter(archivepath) -> embeddedimporter object\n\
\n\
Create a new embeddedimporter instance. 'archivepath' must be a path to\n\
a python.exe itself, or to a specific path inside a embeddedfile. For example, it can be\n\
'/user/bin/python', or '/user/bin/python/mydirectory', if mydirectory is a\n\
valid directory inside the archive.\n\
\n\
'EmbeddedImportError is raised if 'archivepath' doesn't point to a valid Embedded\n\
archive.\n\
\n\
The 'archive' attribute of embeddedimporter objects contains the name of the\n\
embeddedfile targeted.");

#define DEFERRED_ADDRESS(ADDR) 0

static PyTypeObject EmbeddedImporter_Type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
    "embeddedimport.embeddedimporter",
    sizeof(EmbeddedImporter),
    0,                                          /* tp_itemsize */
    (destructor)embeddedimporter_dealloc,            /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_compare */
    (reprfunc)embeddedimporter_repr,                 /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
        Py_TPFLAGS_HAVE_GC,                     /* tp_flags */
    embeddedimporter_doc,                            /* tp_doc */
    embeddedimporter_traverse,                       /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    embeddedimporter_methods,                        /* tp_methods */
    embeddedimporter_members,                        /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)embeddedimporter_init,                 /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    PyType_GenericNew,                          /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};

/* Module init */

PyDoc_STRVAR(embeddedimport_doc,
"embeddedimport provides support for importing Python modules from python.exe itself.\n\
\n\
This module exports three objects:\n\
- embeddedimporter: a class; its constructor takes a path to a Embedded archive.\n\
- EmbeddedImportError: exception raised by embeddedimporter objects. It's a\n\
  subclass of ImportError, so it can be caught as ImportError, too.\n\
\n\
It is usually not needed to use the embeddedimport module explicitly; it is\n\
used by the builtin import mechanism for sys.path items that are paths\n\
to Embedded archives.");

PyMODINIT_FUNC
initembeddedimport(void)
{
    PyObject *mod, *path, *fullpath;

    if (PyType_Ready(&EmbeddedImporter_Type) < 0) {
        return;
    }

    embedded_searchorder[0].suffix[0] = SEP;

    mod = Py_InitModule4("embeddedimport", NULL, embeddedimport_doc,
                         NULL, PYTHON_API_VERSION);
    if (mod == NULL) {
        return;
    }

    EmbeddedImportError = PyErr_NewException("embeddedimport.EmbeddedImportError",
                                             PyExc_ImportError, NULL);
    if (EmbeddedImportError == NULL) {
        return;
    }

    Py_INCREF(EmbeddedImportError);
    if (PyModule_AddObject(mod, "EmbeddedImportError",
                           EmbeddedImportError) < 0) {
        return;
    }

    Py_INCREF(&EmbeddedImporter_Type);
    if (PyModule_AddObject(mod, "embeddedimporter",
                           (PyObject *)&EmbeddedImporter_Type) < 0) {
        return;
    }

    path = PySys_GetObject("path");
    if (path == NULL) {
        return;
    }

    fullpath = PyString_FromString(Py_GetProgramFullPath());
    if (fullpath == NULL) {
        return;
    }

//    if (PyList_Append(path, fullpath) < 0)
//        PyErr_Clear();
    if (PyList_Insert(path, 0, fullpath) < 0)
        PyErr_Clear();
    Py_DECREF(fullpath);
}

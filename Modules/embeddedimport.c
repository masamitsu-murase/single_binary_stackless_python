#include "Python.h"
#include "structmember.h"
#include "osdefs.h"
#include "marshal.h"

#include "zlib.h"

#include <windows.h>

#define USER_SOURCE_ID  200

struct st_embedded_searchorder {
    wchar_t suffix[14];
    int package;
};

static struct st_embedded_searchorder embedded_searchorder[] = {
    {L"/__init__.py", 1},
    {L".py", 0},
    {L"", 0}
};


typedef struct _embeddedimporter EmbeddedImporter;

struct _embeddedimporter {
    PyObject_HEAD
    PyObject *dict;  // { filename: (data_offset) }
    PyObject *dict_resource;  // { filename: (data_offset) }
    PyObject *prefix;
};

extern char embeddedimporter_filename[];
extern const size_t embeddedimporter_raw_data_size;
extern const unsigned char embeddedimporter_raw_data_compressed[];
extern const size_t embeddedimporter_raw_data_compressed_size;
extern const size_t embeddedimporter_data_offset[];
static char *embeddedimporter_raw_data;

static char *embeddedimporter_raw_data_resource;

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
        PyObject *name_obj;

        for (; embeddedimporter_filename[name_index]; name_index++) {
            if (embeddedimporter_filename[name_index] == '/') {
                embeddedimporter_filename[name_index] = SEP;
            }
        }
        name_index++;

        tuple = PyTuple_New(1);
        if (tuple == NULL
            || PyTuple_SetItem(tuple, 0,
                               PyLong_FromSize_t(embeddedimporter_data_offset[file_index])) < 0) {
            Py_XDECREF(tuple);
            return -1;
        }

        name_obj = PyUnicode_FromString(name);
        if (name_obj == NULL || PyDict_SetItem(self->dict, name_obj, tuple) < 0) {
            Py_DECREF(tuple);
            Py_XDECREF(name_obj);
            return -1;
        }
        Py_DECREF(tuple);
        Py_DECREF(name_obj);
    }

    return 0;
}

#if Py_BUILD_RESOURCE_EMBEDDED_MODULE
static int
construct_filedata_from_resource(EmbeddedImporter *self)
{
    // Data structure
    //   uint32_t compressed_size;
    //   uint32_t uncompressed_size;
    //   uint32_t file_count;
    //   uint32_t [] file_offset;
    //   unsigned char [] compressed_file_data;  // separated by '\0'
    //   char [] file_name;  // separated by '\0'.

    static PyObject *embeddedimporter_data_dict_resource = NULL;
    static int resource_checked = 0;

    uint32_t compressed_size;
    uint32_t uncompressed_size;
    uint32_t file_count;
    uint32_t *file_offset;
    char *file_name;
    unsigned char *compressed_file_data;
    HRSRC resource_handle;
    HGLOBAL resource_data_handle;
    unsigned char *resource_data;

    PyObject *tuple;
    size_t name_index, file_index;
    uLongf raw_data_size;

    if (embeddedimporter_data_dict_resource != NULL) {
        Py_INCREF(embeddedimporter_data_dict_resource);
        self->dict_resource = embeddedimporter_data_dict_resource;
        return 0;
    }

    if (resource_checked) {
        return -1;
    }
    resource_checked = 1;

    resource_handle = FindResource(NULL, MAKEINTRESOURCE(USER_SOURCE_ID), RT_RCDATA);
    if (resource_handle == NULL) {
        return -1;
    }

    resource_data_handle = LoadResource(NULL, resource_handle);
    if (resource_data_handle == NULL) {
        return -1;
    }

    resource_data = LockResource(resource_data_handle);
    if (resource_data == NULL) {
        return -1;
    }

    //   uint32_t compressed_size;
    //   uint32_t uncompressed_size;
    //   uint32_t file_count;
    //   uint32_t [] file_offset;
    //   unsigned char [] compressed_file_data;  // separated by '\0'
    //   char [] file_name;  // separated by '\0'.
    compressed_size = *(uint32_t *)resource_data;
    resource_data += sizeof(compressed_size);
    uncompressed_size = *(uint32_t *)resource_data;
    resource_data += sizeof(uncompressed_size);
    file_count = *(uint32_t *)resource_data;
    resource_data += sizeof(file_count);
    file_offset = (uint32_t *)resource_data;
    resource_data += sizeof(uint32_t) * file_count;
    compressed_file_data = (unsigned char *)resource_data;
    resource_data += compressed_size;
    file_name = resource_data;

    embeddedimporter_raw_data_resource = PyMem_Malloc(uncompressed_size);
    if (embeddedimporter_raw_data_resource == NULL) {
        return -1;
    }

    raw_data_size = uncompressed_size;
    if (uncompress((Bytef *)embeddedimporter_raw_data_resource, &raw_data_size,
                   compressed_file_data, compressed_size) != Z_OK
          || raw_data_size != uncompressed_size) {
        PyMem_Free(embeddedimporter_raw_data_resource);
        return -1;
    }

    self->dict_resource = embeddedimporter_data_dict_resource = PyDict_New();
    if (self->dict_resource == NULL) {
        return -1;
    }

    for (file_index=0, name_index=0; file_index<file_count; file_index++) {
        char *name = &file_name[name_index];
        PyObject *name_obj;

        tuple = PyTuple_New(1);
        if (tuple == NULL
            || PyTuple_SetItem(tuple, 0,
                               PyLong_FromSize_t(file_offset[file_index])) < 0) {
            Py_XDECREF(tuple);
            return -1;
        }

        name_obj = PyUnicode_FromString(name);
        if (name_obj == NULL || PyDict_SetItem(self->dict_resource, name_obj, tuple) < 0) {
            Py_DECREF(tuple);
            Py_XDECREF(name_obj);
            return -1;
        }
        Py_DECREF(tuple);
        Py_DECREF(name_obj);

        for (; file_name[name_index]; name_index++)
            ;
        name_index++;
    }

    return 0;
}
#endif

static int
embeddedimporter_init(EmbeddedImporter *self, PyObject *args, PyObject *kwds)
{
    PyObject *path_obj;
    wchar_t path[MAXPATHLEN + 2];
    wchar_t prefix[MAXPATHLEN + 2];
    wchar_t program_full_path[MAXPATHLEN + 1];
    size_t len, path_len;

    wcscpy(program_full_path, Py_GetProgramFullPath());
    len = wcslen(program_full_path);

    if (!_PyArg_NoKeywords("embeddedimporter()", kwds)) {
        return -1;
    }

    if (!PyArg_ParseTuple(args, "O&:embeddedimporter",
                          PyUnicode_FSDecoder, &path_obj)) {
        return -1;
    }

    if (PyUnicode_READY(path_obj) == -1) {
        return -1;
    }

    path_len = PyUnicode_AsWideChar(path_obj, path, sizeof(path)/sizeof(path[0]) - 1);
    if (path_len == -1) {
        return -1;
    }
    path[path_len] = L'\0';

    if (wcslen(path) != path_len) {
        // NULL is in the string.
        return -1;
    }

    if (wcsncmp(path, program_full_path, len) != 0 || (path[len] != L'\0' && path[len] != SEP)) {
        PyErr_SetString(EmbeddedImportError, "not a executable file");
        return -1;
    }

    if (path[len] == SEP) {
        wchar_t *p;
        PyObject *prefix_obj;
        wcscpy(prefix, path + len + 1);
        p = prefix;
        while (*p) {
            if (*p == L'.') {
                *p = SEP;
            }
            p++;
        }
        prefix_obj = PyUnicode_FromWideChar(prefix, -1);
        if (prefix_obj == NULL) {
            return -1;
        }
        self->prefix = PyUnicode_FromFormat("%U%c", prefix_obj, (int)SEP);
        Py_DECREF(prefix_obj);
    } else {
        self->prefix = NULL;
    }

    if (construct_filedata(self) < 0) {
        return -1;
    }
#if Py_BUILD_RESOURCE_EMBEDDED_MODULE
    construct_filedata_from_resource(self);
#endif

    return 0;
}

/* GC support. */
static int
embeddedimporter_traverse(PyObject *obj, visitproc visit, void *arg)
{
    EmbeddedImporter *self = (EmbeddedImporter *)obj;
    Py_VISIT(self->dict);
    Py_VISIT(self->dict_resource);
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
    Py_XDECREF(self->dict_resource);
    Py_XDECREF(self->prefix);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
embeddedimporter_repr(EmbeddedImporter *self)
{
    return PyUnicode_FromString("<embeddedimporter object>");
}

static PyObject *
find_tuple(EmbeddedImporter *self, const wchar_t *subname, int *is_package,
           char **raw_data)
{
    wchar_t buf[MAXPATHLEN + 1];
    size_t prefix_len, len;
    struct st_embedded_searchorder *eso;
    PyObject *tuple;
    wchar_t *prefix = NULL;

    if (self->prefix) {
        prefix = PyUnicode_AsWideCharString(self->prefix, NULL);
        prefix_len = wcslen(prefix);
    } else {
        prefix_len = 0;
    }
    len = wcslen(subname);
    //                  /__init__.py
    if (prefix_len + len + 13 >= MAXPATHLEN) {
        if (prefix != NULL) {
            PyMem_Free(prefix);
        }
        return NULL;
    }

    if (prefix != NULL) {
        wcscpy(buf, prefix);
        PyMem_Free(prefix);
    }
    wcscpy(buf + prefix_len, subname);
    for (eso = embedded_searchorder; *eso->suffix; eso++) {
        PyObject *buf_obj;
        wcscpy(buf + prefix_len + len, eso->suffix);
        buf_obj = PyUnicode_FromWideChar(buf, -1);
        if (buf_obj == NULL) {
            return NULL;
        }
        if (Py_VerboseFlag > 1) {
            PySys_FormatStderr("# trying %c%U\n", (int)SEP, buf_obj);
        }

        if (self->dict_resource) {
            tuple = PyDict_GetItem(self->dict_resource, buf_obj);
            if (tuple != NULL) {
                if (is_package != NULL) {
                    *is_package = eso->package;
                }
                if (raw_data != NULL) {
                    *raw_data = embeddedimporter_raw_data_resource;
                }
                Py_XDECREF(buf_obj);
                return tuple;
            }
        }

        tuple = PyDict_GetItem(self->dict, buf_obj);
        Py_XDECREF(buf_obj);
        if (tuple != NULL) {
            if (is_package != NULL) {
                *is_package = eso->package;
            }
            if (raw_data != NULL) {
                *raw_data = embeddedimporter_raw_data;
            }
            return tuple;
        }
    }

    return NULL;
}

static wchar_t *
get_subname(wchar_t *fullname)
{
    wchar_t *subname = wcsrchr(fullname, L'.');
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
    wchar_t *fullname, *subname;
    PyObject *fullname_obj;
    Py_ssize_t fullname_size;

    if (!PyArg_ParseTuple(args, "U|O:embeddedimporter.find_module",
                          &fullname_obj, &path)) {
        return NULL;
    }

    fullname = PyUnicode_AsWideCharString(fullname_obj, &fullname_size);
    if (fullname == NULL || wcslen(fullname) != fullname_size) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    subname = get_subname(fullname);
    if (find_tuple(self, subname, NULL, NULL) == NULL) {
        Py_INCREF(Py_None);
        PyMem_Free(fullname);
        return Py_None;
    }

    PyMem_Free(fullname);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
get_modpath(EmbeddedImporter *self, const wchar_t *subname, int is_package)
{
    PyObject *modpath;
    PyObject *program_full_path_obj, *subname_obj;
    program_full_path_obj = PyUnicode_FromWideChar(Py_GetProgramFullPath(), -1);
    subname_obj = PyUnicode_FromWideChar(subname, -1);
    if (is_package) {
        modpath = PyUnicode_FromFormat("%U%c%V%U%c%s",
                                       program_full_path_obj,
                                       (int)SEP,
                                       self->prefix, "",
                                       subname_obj,
                                       (int)SEP,
                                       "__init__.py");
    } else {
        modpath = PyUnicode_FromFormat("%U%c%V%U.py",
                                       program_full_path_obj,
                                       (int)SEP,
                                       self->prefix, "",
                                       subname_obj);
    }
    Py_XDECREF(subname_obj);
    Py_XDECREF(program_full_path_obj);
    return modpath;
}

/* Load and return the module named by 'fullname'. */
static PyObject *
embeddedimporter_load_module(PyObject *obj, PyObject *args)
{
    EmbeddedImporter *self = (EmbeddedImporter *)obj;
    PyObject *tuple, *code, *module, *dict, *modpath;
    wchar_t *fullname, *subname;
    PyObject *fullname_obj;
    Py_ssize_t fullname_size;
    int is_package = 0;
    long data_offset;
    char *raw_data;

    if (!PyArg_ParseTuple(args, "U:embeddedimporter.load_module",
                          &fullname_obj)) {
        return NULL;
    }
    fullname = PyUnicode_AsWideCharString(fullname_obj, &fullname_size);
    if (fullname == NULL || wcslen(fullname) != fullname_size) {
        return NULL;
    }

    subname = get_subname(fullname);
    tuple = find_tuple(self, subname, &is_package, &raw_data);
    if (tuple == NULL) {
        PyErr_SetString(EmbeddedImportError, "not found");
        PyMem_Free(fullname);
        return NULL;
    }

    data_offset = PyLong_AsLong(PyTuple_GetItem(tuple, 0));
    code = Py_CompileStringObject(&raw_data[data_offset], fullname_obj,
                                  Py_file_input, NULL, -1);
    if (code == NULL) {
        PyMem_Free(fullname);
        return NULL;
    }

    module = PyImport_AddModuleObject(fullname_obj);
    if (module == NULL) {
        PyMem_Free(fullname);
        Py_XDECREF(code);
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
        PyObject *program_full_path_obj, *subname_obj;
        int err;

        program_full_path_obj = PyUnicode_FromWideChar(Py_GetProgramFullPath(), -1);
        subname_obj = PyUnicode_FromWideChar(subname, -1);
        fullpath = PyUnicode_FromFormat("%U%c%V%U",
                                        program_full_path_obj,
                                        (int)SEP,
                                        self->prefix, "",
                                        subname_obj);
        Py_XDECREF(program_full_path_obj);
        Py_XDECREF(subname_obj);
        if (fullpath == NULL) {
            goto error;
        }

        pkgpath = Py_BuildValue("[N]", fullpath);
        if (pkgpath == NULL) {
            goto error;
        }
        err = PyDict_SetItemString(dict, "__path__", pkgpath);
        Py_XDECREF(pkgpath);
        if (err != 0) {
            goto error;
        }
    }

    modpath = get_modpath(self, subname, is_package);
    module = PyImport_ExecCodeModuleObject(fullname_obj, code, modpath, NULL);
    if (Py_VerboseFlag) {
        PySys_FormatStderr("import %U # loaded from Embedded %U\n",
                           fullname_obj, modpath);
    }
    PyMem_Free(fullname);  // Kept for subname.
    Py_XDECREF(modpath);
    Py_XDECREF(code);
    return module;

error:
    PyMem_Free(fullname);
    Py_XDECREF(code);
    Py_XDECREF(module);
    return NULL;
}

/* Return a bool signifying whether the module is a package or not. */
static PyObject *
embeddedimporter_is_package(PyObject *obj, PyObject *args)
{
    EmbeddedImporter *self = (EmbeddedImporter *)obj;
    wchar_t *fullname, *subname;
    PyObject *fullname_obj;
    int is_package = 0;
    PyObject *tuple;

    if (!PyArg_ParseTuple(args, "U:embeddedimporter.is_package",
                          &fullname_obj)) {
        return NULL;
    }

    fullname = PyUnicode_AsWideCharString(fullname_obj, NULL);
    subname = get_subname(fullname);
    tuple = find_tuple(self, subname, &is_package, NULL);
    PyMem_Free(fullname);
    if (tuple == NULL) {
        PyErr_SetString(EmbeddedImportError, "not found");
        return NULL;
    }

    return PyBool_FromLong(is_package);
}

/* Return a string matching __file__ for the named module */
static PyObject *
embeddedimporter_get_filename(PyObject *obj, PyObject *args)
{
    EmbeddedImporter *self = (EmbeddedImporter *)obj;
    PyObject *tuple, *modpath;
    wchar_t *fullname, *subname;
    PyObject *fullname_obj;
    int is_package = 0;

    if (!PyArg_ParseTuple(args, "U:embeddedimporter.get_filename",
                         &fullname_obj)) {
        return NULL;
    }

    fullname = PyUnicode_AsWideCharString(fullname_obj, NULL);
    subname = get_subname(fullname);
    tuple = find_tuple(self, subname, &is_package, NULL);
    if (tuple == NULL) {
        if (PyErr_Occurred() == NULL) {
            PyErr_SetString(EmbeddedImportError, "not found");
        }
        return NULL;
    }
    modpath = get_modpath(self, subname, is_package);
    PyMem_Free(fullname);
    return modpath;
}

static PyObject *
embeddedimporter_get_code(PyObject *obj, PyObject *args)
{
    EmbeddedImporter *self = (EmbeddedImporter *)obj;
    wchar_t *fullname, *subname;
    PyObject *fullname_obj;
    PyObject *tuple;
    PyObject *code;
    long data_offset;
    char *raw_data;

    if (!PyArg_ParseTuple(args, "U:embeddedimporter.get_code", &fullname_obj)) {
        return NULL;
    }

    fullname = PyUnicode_AsWideCharString(fullname_obj, NULL);
    subname = get_subname(fullname);
    tuple = find_tuple(self, subname, NULL, &raw_data);
    if (tuple == NULL) {
        PyErr_SetString(EmbeddedImportError, "not found");
        PyMem_Free(fullname);
        return NULL;
    }

    data_offset = PyLong_AsLong(PyTuple_GetItem(tuple, 0));
    code = Py_CompileStringObject(&raw_data[data_offset], fullname_obj,
                                  Py_file_input, NULL, -1);
    PyMem_Free(fullname);
    return code;
}

static PyObject *
embeddedimporter_get_source(PyObject *obj, PyObject *args)
{
    EmbeddedImporter *self = (EmbeddedImporter *)obj;
    wchar_t *fullname, *subname;
    PyObject *fullname_obj;
    PyObject *tuple;
    PyObject *code;
    long data_offset;
    char *raw_data;

    if (!PyArg_ParseTuple(args, "U:embeddedimporter_get_source", &fullname_obj)) {
        return NULL;
    }

    fullname = PyUnicode_AsWideCharString(fullname_obj, NULL);
    subname = get_subname(fullname);
    tuple = find_tuple(self, subname, NULL, &raw_data);
    if (tuple == NULL) {
        PyErr_SetString(EmbeddedImportError, "not found");
        PyMem_Free(fullname);
        return NULL;
    }

    data_offset = PyLong_AsLong(PyTuple_GetItem(tuple, 0));
    code = PyUnicode_FromString(&raw_data[data_offset]);
    PyMem_Free(fullname);
    return code;
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

PyDoc_STRVAR(doc_get_filename,
"get_filename(fullname) -> filename string.\n\
\n\
Return the filename for the specified module.");

PyDoc_STRVAR(doc_get_code,
"get_code(fullname) -> code object.\n\
\n\
Return the code object for the specified module. Raise EmbeddedImportError\n\
if the module couldn't be found.");

PyDoc_STRVAR(doc_get_source,
"get_source(fullname) -> source string.\n\
\n\
Return the source code for the specified module. Raise EmbeddedImportError\n\
if the module couldn't be found.");

static PyMethodDef embeddedimporter_methods[] = {
    {"find_module", embeddedimporter_find_module, METH_VARARGS,
     doc_find_module},
    {"load_module", embeddedimporter_load_module, METH_VARARGS,
     doc_load_module},
    {"get_filename", embeddedimporter_get_filename, METH_VARARGS,
     doc_get_filename},
    {"is_package", embeddedimporter_is_package, METH_VARARGS,
     doc_is_package},
    {"get_code", embeddedimporter_get_code, METH_VARARGS,
     doc_get_code},
    {"get_source", embeddedimporter_get_source, METH_VARARGS,
     doc_get_source},
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

static struct PyModuleDef embeddedimportmodule = {
    PyModuleDef_HEAD_INIT,
    "embeddedimport",
    embeddedimport_doc,
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_embeddedimport(void)
{
    PyObject *mod, *path, *fullpath;

    if (PyType_Ready(&EmbeddedImporter_Type) < 0) {
        return NULL;
    }

    embedded_searchorder[0].suffix[0] = SEP;

    mod = PyModule_Create(&embeddedimportmodule);
    if (mod == NULL) {
        return NULL;
    }

    EmbeddedImportError = PyErr_NewException("embeddedimport.EmbeddedImportError",
                                             PyExc_ImportError, NULL);
    if (EmbeddedImportError == NULL) {
        return NULL;
    }

    Py_INCREF(EmbeddedImportError);
    if (PyModule_AddObject(mod, "EmbeddedImportError",
                           EmbeddedImportError) < 0) {
        return NULL;
    }

    Py_INCREF(&EmbeddedImporter_Type);
    if (PyModule_AddObject(mod, "embeddedimporter",
                           (PyObject *)&EmbeddedImporter_Type) < 0) {
        return NULL;
    }

    path = PySys_GetObject("path");
    if (path == NULL) {
        return NULL;
    }

    fullpath = PyUnicode_FromWideChar(Py_GetProgramFullPath(), -1);
    if (fullpath == NULL) {
        return NULL;
    }

//    if (PyList_Append(path, fullpath) < 0)
//        PyErr_Clear();
    if (PyList_Insert(path, 0, fullpath) < 0)
        PyErr_Clear();
    Py_DECREF(fullpath);

    return mod;
}

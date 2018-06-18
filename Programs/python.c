/* Minimal main program -- everything is loaded from the library */

#include "Python.h"
#include <locale.h>

#ifdef __FreeBSD__
#include <fenv.h>
#endif

#ifdef MS_WINDOWS
#if Py_BUILD_RESOURCE_EMBEDDED_MODULE
#include <windows.h>

#define USER_COMMAND_ID 101


static wchar_t *overridden_argv[128];

static void
override_args(int argc, wchar_t **argv, int *new_argc, wchar_t ***new_argv)
{
    HRSRC resource_handle;
    HGLOBAL resource_data_handle;
    unsigned char *resource_data;
    uint32_t argc_resource, i;
    wchar_t *argv_resource;

    *new_argc = argc;
    *new_argv = argv;

    resource_handle = FindResource(NULL, MAKEINTRESOURCE(USER_COMMAND_ID), RT_RCDATA);
    if (resource_handle == NULL) {
        return;
    }

    resource_data_handle = LoadResource(NULL, resource_handle);
    if (resource_data_handle == NULL) {
        return;
    }

    resource_data = LockResource(resource_data_handle);
    if (resource_data == NULL) {
        return;
    }

    argc_resource = *(uint32_t *)resource_data;
    resource_data += sizeof(argc_resource);
    argv_resource = (wchar_t *)resource_data;

    if (argc_resource + 2 > sizeof(overridden_argv)/sizeof(overridden_argv[0])) {
        return;
    }

    overridden_argv[0] = argv[0];
    for (i=0; i<argc_resource; i++) {
        overridden_argv[i + 1] = argv_resource;
        argv_resource += wcslen(argv_resource) + 1;
    }

    *new_argc = argc_resource + 1;
    *new_argv = overridden_argv;
}
#endif

int
wmain(int argc, wchar_t **argv)
{
#if Py_BUILD_RESOURCE_EMBEDDED_MODULE
    int new_argc;
    wchar_t **new_argv;

    override_args(argc, argv, &new_argc, &new_argv);

    return Py_Main(new_argc, new_argv);
#else
    return Py_Main(argc, argv);
#endif
}
#else

int
main(int argc, char **argv)
{
    wchar_t **argv_copy;
    /* We need a second copy, as Python might modify the first one. */
    wchar_t **argv_copy2;
    int i, res;
    char *oldloc;

    /* Force malloc() allocator to bootstrap Python */
    (void)_PyMem_SetupAllocators("malloc");

    argv_copy = (wchar_t **)PyMem_RawMalloc(sizeof(wchar_t*) * (argc+1));
    argv_copy2 = (wchar_t **)PyMem_RawMalloc(sizeof(wchar_t*) * (argc+1));
    if (!argv_copy || !argv_copy2) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    /* 754 requires that FP exceptions run in "no stop" mode by default,
     * and until C vendors implement C99's ways to control FP exceptions,
     * Python requires non-stop mode.  Alas, some platforms enable FP
     * exceptions by default.  Here we disable them.
     */
#ifdef __FreeBSD__
    fedisableexcept(FE_OVERFLOW);
#endif

    oldloc = _PyMem_RawStrdup(setlocale(LC_ALL, NULL));
    if (!oldloc) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    setlocale(LC_ALL, "");
    for (i = 0; i < argc; i++) {
        argv_copy[i] = Py_DecodeLocale(argv[i], NULL);
        if (!argv_copy[i]) {
            PyMem_RawFree(oldloc);
            fprintf(stderr, "Fatal Python error: "
                            "unable to decode the command line argument #%i\n",
                            i + 1);
            return 1;
        }
        argv_copy2[i] = argv_copy[i];
    }
    argv_copy2[argc] = argv_copy[argc] = NULL;

    setlocale(LC_ALL, oldloc);
    PyMem_RawFree(oldloc);

    res = Py_Main(argc, argv_copy);

    /* Force again malloc() allocator to release memory blocks allocated
       before Py_Main() */
    (void)_PyMem_SetupAllocators("malloc");

    for (i = 0; i < argc; i++) {
        PyMem_RawFree(argv_copy2[i]);
    }
    PyMem_RawFree(argv_copy);
    PyMem_RawFree(argv_copy2);
    return res;
}
#endif

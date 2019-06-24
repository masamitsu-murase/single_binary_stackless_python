#include <stackless_api.h>


static int slp_test_schedule(void)
{
    _testembed_Py_Initialize();
    PyRun_SimpleString(
"import stackless\n"
"import sysconfig\n"
"stackless.tasklet(print)('Hello, World!')\n"
    );
    if (!PyStackless_Schedule(Py_None, 0))
        PyErr_Print();
    Py_Finalize();
    return 0;
}


static struct TestCase SlpTestCases[] = {
    { "slp_schedule", slp_test_schedule },
    { NULL, NULL }
};

int main(int argc, char *argv[])
{
    int wrapped_main(int, char **);

    if (argc > 1) {
        for (struct TestCase *tc = SlpTestCases; tc && tc->name; tc++) {
            if (strcmp(argv[1], tc->name) == 0)
                return (*tc->func)();
        }
        return wrapped_main(argc, argv);
    }

    /* No match found, or no test name provided, so display usage */
    wrapped_main(argc, argv);
    for (struct TestCase *tc = SlpTestCases; tc && tc->name; tc++) {
        printf("  %s\n", tc->name);
    }

    /* Non-zero exit code will cause test_embed.py tests to fail.
       This is intentional. */
    return -1;
}


#define main wrapped_main

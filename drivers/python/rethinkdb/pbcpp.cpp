#include <python2.7/Python.h>

static PyMethodDef PbMethods[] = {
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initrethinkdb_pbcpp() {
    PyObject *m;
    m = Py_InitModule("rethinkdb_pbcpp", PbMethods);
    if (m == NULL)
        return;
}

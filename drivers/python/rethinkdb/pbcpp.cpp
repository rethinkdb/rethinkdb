#include <python2.7/Python.h>

static PyMethodDef PbMethods[] = {
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initpbcpp() {
    PyObject *m;
    m = Py_InitModule("pbcpp", PbMethods);
    if (m == NULL)
        return;
}

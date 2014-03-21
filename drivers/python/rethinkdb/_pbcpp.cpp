// This is an empty python module. It gets linked to ql2.pb.o, whose functions get
// exposed and used by the C++ implementation of the google.protobuf package.

#include <python2.7/Python.h>

static PyMethodDef PbMethods[] = {
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC init_pbcpp() {
    PyObject *m;
    m = Py_InitModule("rethinkdb._pbcpp", PbMethods);
    if (m == NULL)
        return;
}

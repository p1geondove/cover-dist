#include <Python.h>
#include "cover_dist.h"

static PyObject* py_cover_dist(PyObject* self, PyObject* args){
    char* file_path;
    int num_digits;
    if (!PyArg_ParseTuple(args, "si", &file_path, &num_digits)){
        return NULL;
    }
    ScanResult res = cover_dist(file_path, num_digits);
    PyObject* return_tuple = PyTuple_New(2);
    PyObject* py_dist = PyLong_FromSize_t(res.dist);
    PyObject* py_last = PyLong_FromSize_t(res.last_num);
    PyTuple_SetItem(return_tuple, 0, py_dist);
    PyTuple_SetItem(return_tuple, 1, py_last);
    return return_tuple;
}

static PyMethodDef CoverDistMethods[] = {
    {"cover_dist", py_cover_dist, METH_VARARGS, "Digits needed and last digits to cover all n-digit numbers"},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef_Slot coverdist_slots[] = {
#if PY_VERSION_HEX >= 0x030D0000
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
    {0, NULL}
};

static struct PyModuleDef coverdistmodule = {
    PyModuleDef_HEAD_INIT,
    "coverdist",
    NULL,
    0,
    CoverDistMethods,
    coverdist_slots,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit_coverdist(void){
    return PyModuleDef_Init(&coverdistmodule);
}

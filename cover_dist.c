#include <Python.h>
#include "cover_dist.h"

static PyObject* py_cover_dist(PyObject* self, PyObject* args){
    PyObject* path_bytes;
    int num_digits;
    if (!PyArg_ParseTuple(args, "O&i", PyUnicode_FSConverter, &path_bytes, &num_digits)){
        return NULL;
    }

    if (num_digits < 1){
        PyErr_SetString(PyExc_ValueError, "number of digits has to be a positive nonzero integer");
        return NULL;
    }

    char* file_path = PyBytes_AsString(path_bytes);
    ScanResult res = cover_dist(file_path, num_digits);

    switch (res.status) {
        case ERR_INVALID_CHAR:
            PyErr_SetString(PyExc_ValueError, "invalid char found");
            return NULL;
        case ERR_NO_RADIX:
            PyErr_SetString(PyExc_ValueError, "can't find radix point");
            return NULL;
        case ERR_MULTIPLE_RADIX:
            PyErr_SetString(PyExc_ValueError, "found multiple radix points");
            return NULL;
        case ERR_ALLOCATION:
            PyErr_SetString(PyExc_MemoryError, "could not allocate memory for bitset");
            return NULL;
        case ERR_INSUFFICIENT_DIGITS:
            PyErr_SetString(PyExc_ValueError, "not enough digits in file");
            return NULL;
        case ERR_OPEN_FAILED:
            errno = res.save_errno;
            PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, path_bytes);
            return NULL;
        default:break;
    }

    Py_DECREF(path_bytes);
    PyObject* py_dist = PyLong_FromSize_t(res.dist);
    PyObject* py_last = PyLong_FromSize_t(res.last_num);
    PyObject* return_tuple = PyTuple_Pack(2, py_dist, py_last);
    Py_DECREF(py_dist);
    Py_DECREF(py_last);
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

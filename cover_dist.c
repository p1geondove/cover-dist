/* CAUTION VIBE CODED */
/* CAUTION VIBE CODED */
/* CAUTION VIBE CODED */

#include <Python.h>
#include "cover_dist.h"

#ifndef PYTHREAD_INVALID_THREAD_ID
#define PYTHREAD_INVALID_THREAD_ID ((unsigned long)-1) // how do you know its exactly this? is this static? defined in the python docs?
#endif

#define POLL_INTERVAL_US 50000

static volatile bool g_cancel = false;

typedef struct {
    char* path;
    size_t num_digits;
    volatile bool interrupt;
    ScanResult res;
    PyThread_type_lock done;
} ScanJob;

static void scan_thread(void* arg){
    ScanJob* job = arg;
    job->res = cover_dist(job->path, job->num_digits, &job->interrupt);
    PyThread_release_lock(job->done);
}

static PyObject* py_cancel(PyObject* self, PyObject* Py_UNUSED(ignored)){
    g_cancel = true;
    Py_RETURN_NONE;
}

static PyObject* py_cover_dist(PyObject* self, PyObject* args){
    PyObject* path_bytes;
    int num_digits;
    if (!PyArg_ParseTuple(args, "O&i", PyUnicode_FSConverter, &path_bytes, &num_digits)){
        return NULL;
    }

    if (num_digits < 1){
        PyErr_SetString(PyExc_ValueError, "number of digits has to be a positive nonzero integer");
        Py_DECREF(path_bytes);
        return NULL;
    }

    ScanJob job = {
        .path = PyBytes_AsString(path_bytes),
        .num_digits = (size_t)num_digits,
        .interrupt = false,
        .done = PyThread_allocate_lock()
    };

    if (job.done == NULL){
        Py_DECREF(path_bytes);
        return PyErr_NoMemory();
    }

    PyThread_acquire_lock(job.done, 1);
    if (PyThread_start_new_thread(scan_thread, &job) == PYTHREAD_INVALID_THREAD_ID){
        PyThread_release_lock(job.done);
        PyThread_free_lock(job.done);
        Py_DECREF(path_bytes);
        PyErr_SetString(PyExc_RuntimeError, "could not start scan thread");
        return NULL;
    }

    // wait for the scan, wake up every 50ms to check for ctlr+c or cancel()
    for (;;){
        PyLockStatus st;
        Py_BEGIN_ALLOW_THREADS
        st = PyThread_acquire_lock_timed(job.done, POLL_INTERVAL_US, 0);
        Py_END_ALLOW_THREADS
        if (st == PY_LOCK_ACQUIRED) break;

        // CheckSignals only does something on the main thread; -1 means KeyboardInterrupt is now set
        if (!job.interrupt && (g_cancel || PyErr_CheckSignals() != 0)){
            job.interrupt = true;
        }
    }
    PyThread_release_lock(job.done);
    PyThread_free_lock(job.done);

    // ctrl+c arrived (or a signal handler raised): whatever the scan returned no longer matters
    if (PyErr_Occurred()){
        Py_DECREF(path_bytes);
        return NULL;
    }

    PyObject* result = NULL;

    switch (job.res.status) {
        case OK:{
            PyObject* py_dist = PyLong_FromSize_t(job.res.dist);
            PyObject* py_last = PyLong_FromSize_t(job.res.last_num);
            if (py_dist && py_last) result = PyTuple_Pack(2, py_dist, py_last);
            Py_XDECREF(py_dist);
            Py_XDECREF(py_last);
            break;
        }
        case ERR_INTERRUPT:
            if (!PyErr_Occurred()) PyErr_SetString(PyExc_InterruptedError, "scan was interrupted");
            break;
        case ERR_INVALID_CHAR:
            PyErr_SetString(PyExc_ValueError, "invalid char found");
            break;
        case ERR_NO_RADIX:
            PyErr_SetString(PyExc_ValueError, "can't find radix point");
            break;
        case ERR_MULTIPLE_RADIX:
            PyErr_SetString(PyExc_ValueError, "found multiple radix points");
            break;
        case ERR_ALLOCATION:
            PyErr_SetString(PyExc_MemoryError, "could not allocate memory for bitset");
            break;
        case ERR_INSUFFICIENT_DIGITS:
            PyErr_SetString(PyExc_ValueError, "not enough digits in file");
            break;
        case ERR_TOO_MANY_DIGITS:
            PyErr_SetString(PyExc_ValueError, "number of digits has to be an integer between 1 and 20 (both included)");
            break;
        case ERR_OPEN_FAILED:
            errno = job.res.save_errno;
            PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, path_bytes);
            break;
        default:
            PyErr_SetString(PyExc_RuntimeError, "unknown error occured");
            break;
    }

    Py_DECREF(path_bytes);
    return result;
}

static PyMethodDef CoverDistMethods[] = {
    {"cover_dist", py_cover_dist, METH_VARARGS, "Digits needed and last digits to cover all n-digit numbers"},
    {"cancel", py_cancel, METH_NOARGS, "Stop all current and future scans"},
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

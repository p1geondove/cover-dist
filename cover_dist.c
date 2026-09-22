#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <Python.h>

#define BLOCKSIZE 1024*1024

typedef struct {
    uint8_t* bools;
    size_t remaining;
} PackedBools;

typedef struct {
    size_t dist;
    size_t last_num;
} ScanResult;

PackedBools packedbools_make(size_t num_bools){
    size_t bytes_needed = num_bools / 8 + (num_bools % 8 > 0);
    PackedBools bools;
    uint8_t* arr = (uint8_t*)malloc(bytes_needed);
    memset(arr, 0, bytes_needed);
    bools.bools = arr;
    bools.remaining = num_bools;
    return bools;
}
// sets bit to 1 and returns true if it was already set
bool packedbools_set(size_t index, PackedBools* bools){
    size_t byte_index = index >> 3; // divide by 8
    uint8_t mask = 1 << (index & 7); // bit mask
    if (bools->bools[byte_index] & mask) return false;
    bools->bools[byte_index] |= mask;
    bools->remaining--;
    return true;
}

// helper function, just free(bools->bools)
void packedbools_free(PackedBools* bools){
    free(bools->bools);
}

int file_find_radix_point(char* file_path){
    FILE* fptr = fopen(file_path, "r");
    if (fptr == NULL){
        perror("Cant open file");
        return -1;
    }

    // files made by y-crucher can have an int part of up to ~2**63 so 20 should be enough
    char window_str[20];
    char* bufp = fgets(window_str, 20, fptr);
    if (bufp == NULL){
        perror("file empty?");
        return -1;
    }

    for (int i=0; i<20; i++){
        if (window_str[i] == 46) return i;
    }

    return -1;
}

ScanResult cover_dist(char* file_path, int num_digits){
    // number assume to have a radix . somewhere, skip that
    int radix_pos = file_find_radix_point(file_path);
    if (radix_pos == -1){
        perror("Cant find radix pos\n");
        return (ScanResult){0,0};
    }

    // open file and read a block
    char block[BLOCKSIZE];
    FILE* fptr = fopen(file_path, "r");
    char* bufp;
    bufp = fgets(block, BLOCKSIZE, fptr);

    // cut out the radix ., copy the int part and paste it one later, then just start iterating over the block one later
    char intpart[20];
    memcpy(intpart, block, radix_pos);
    memcpy(block+1, intpart, radix_pos);
    size_t block_offset = 1;

    // init first window and pow10mod value
    size_t pow10mod = 1;
    size_t window = 0;
    for (int i=0; i<num_digits; i++){
        window += pow10mod * (block[num_digits+block_offset-i]-48);
        pow10mod *= 10;
    }
    block_offset += num_digits;
    size_t block_count = 0;

    // create PackedBools
    PackedBools bools = packedbools_make(pow10mod);
    packedbools_set(window, &bools);

    // main loop
    while (1){
        for (;block_offset<BLOCKSIZE;){
            size_t nextn = block[block_offset++]-48;
            window = (window * 10 + nextn) % pow10mod;
            packedbools_set(window, &bools);
            if (!bools.remaining){
                packedbools_free(&bools);
                size_t dist = block_offset - radix_pos + BLOCKSIZE * block_count - block_count;
                ScanResult res = {dist, window};
                return res;
            }
        }

        bufp = fgets(block, BLOCKSIZE, fptr);
        if (bufp == NULL){
            packedbools_free(&bools);
            perror("file ended");
            return (ScanResult){0,0};
        }

        block_offset = 0;
        block_count++;
    }
}

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

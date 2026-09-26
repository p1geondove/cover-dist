#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef _WIN32
    #include <windows.h>
#endif

#define BUFFERSIZE (1024u * 1024u)
#define min(a,b) ((a)<(b)?(a):(b))

typedef struct {
    uint8_t* bools;
    size_t remaining;
} PackedBools;

typedef enum {
    OK,
    ERR_NO_RADIX,
    ERR_MULTIPLE_RADIX,
    ERR_INVALID_CHAR,
    ERR_ALLOCATION,
    ERR_OPEN_FAILED,
    ERR_INSUFFICIENT_DIGITS
} Status;

typedef struct {
    size_t size;
    size_t radix_pos;
    size_t zero_offset;
    Status status;
} FileMeta;

typedef struct {
    size_t dist;
    size_t last_num;
    int save_errno;
    Status status;
} ScanResult;

PackedBools packedbools_make(size_t num_bools){
    size_t bytes_needed = num_bools / 8 + (num_bools % 8 > 0);
    PackedBools bools;
    void* _arr = calloc(bytes_needed,1);
    if (_arr == NULL) return (PackedBools){0,0};
    uint8_t* arr = (uint8_t*)_arr;
    bools.bools = arr;
    bools.remaining = num_bools;
    return bools;
}

// sets bit to 1 and returns true if all bits are set
bool packedbools_set(size_t index, PackedBools* bools){
    size_t byte_index = index >> 3; // divide by 8
    uint8_t mask = 1 << (index & 7); // bit mask
    if (bools->bools[byte_index] & mask) return false;
    bools->bools[byte_index] |= mask;
    return --bools->remaining == 0;
}

// helper function, just free(bools->bools)
void packedbools_free(PackedBools* bools){
    free(bools->bools);
}

bool is_dir(char* path){
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES){
        errno = ENOENT; // collapse all Win32 "couldn't stat" cases to ENOENT
        return false;
    }
    if (attr & FILE_ATTRIBUTE_DIRECTORY){
        errno = EISDIR;
        return true;
    }
    return false;
#else
    struct stat st;
    if (stat(path, &st) != 0) return false;
    if (S_ISDIR(st.st_mode)){
        errno = EISDIR;
        return true;
    }
    return false;
#endif
}

// checks if the file is valid (only)
FileMeta get_metadata(FILE* fptr){
    char buffer[BUFFERSIZE] = {0}; // allocate a block
    size_t radix_count = 0; // used to check wether ther is only exactly one
    FileMeta meta = {0}; // yeah... return data

    long off = ftell(fptr); // first store the current position
    fseek(fptr, 0, SEEK_SET); // seek to the start
    fread(buffer, 1, BUFFERSIZE, fptr); // get a block
    fseek(fptr, 0, SEEK_END); // quickly go to the end
    meta.size = (size_t)ftell(fptr); // and get the position
    fseek(fptr, off, SEEK_SET); // go back to where you came from (technically always 0, but whatever)

    // scan the entire first block for invalid chars and also determin position of radix . and if there are multiple
    // int buffer_size = min(strlen(buffer), BUFFERSIZE);
    size_t buffer_size = min(meta.size, BUFFERSIZE);

    for (size_t i=0; i<buffer_size; i++){
        if (buffer[i] == 46){ // 46 = .
            radix_count++;
            if (radix_count > 1){
                return (FileMeta){.status = ERR_MULTIPLE_RADIX};
            }
            meta.radix_pos = i;
        }

        if (buffer[i] != 46 && (buffer[i]<48 || buffer[i]>57)){ // check if invalid char
            return (FileMeta){.status = ERR_INVALID_CHAR};
        }
    }

    if (radix_count == 0){
        return (FileMeta){.status = ERR_NO_RADIX};
    }

    memmove(buffer+1, buffer, meta.radix_pos);
    meta.zero_offset = 1;
    for (; meta.zero_offset < BUFFERSIZE; meta.zero_offset++){
        if (buffer[meta.zero_offset] != 48) break;
    }
    meta.zero_offset--; // -- because we used it as an index but have to skip the first char

    return meta;
}

ScanResult cover_dist(char* file_path, size_t num_digits){
    if (is_dir(file_path)) return (ScanResult){.status = ERR_OPEN_FAILED, .save_errno = errno};
    FILE* fptr = fopen(file_path, "r");
    if (fptr == NULL) return (ScanResult){.status = ERR_OPEN_FAILED, .save_errno = errno};
    FileMeta meta = get_metadata(fptr);
    if (meta.status != OK) return (ScanResult){.status = meta.status};
    if (meta.size <= num_digits) return (ScanResult){.status = ERR_INSUFFICIENT_DIGITS}; // >= since we expect a radix point

    // this uses 2 buffers since we read the file in chunks and need the last chunk
    // technically only the last "num_digits" digits of the previous chunk
    // instead of copying the buffers i opted to swap pointers which should be faster
    char buffera[BUFFERSIZE];
    char bufferb[BUFFERSIZE];
    char* buffer = buffera;
    char* buffer_prev = bufferb;
    char* buffer_tmp = NULL;

    fread(buffera, 1, BUFFERSIZE, fptr);
    size_t window = 0;
    size_t pow10mod = 1;


    // move the integer part to the right, overwriting the radix . to give a clean start to the sequence
    memmove(buffer+1, buffer, meta.radix_pos);

    // generate the first window
    for (size_t i = num_digits + meta.zero_offset; i > meta.zero_offset; i--){
        window += (size_t)(buffer[i]-48) * pow10mod;
        pow10mod *= 10;
    }

    // bitset i think its called by the professionals...
    PackedBools bools = packedbools_make(pow10mod);
    if (bools.remaining != pow10mod){ // allocation error, probably out of ram
        return (ScanResult){.status = ERR_ALLOCATION};
    }

    if (packedbools_set(window, &bools)){
        return (ScanResult){.dist = num_digits, .last_num = window};
    }

    // some more variables
    size_t file_offset, nextn, droppedn;
    size_t buffer_offset = num_digits + 1 + meta.zero_offset;
    file_offset = buffer_offset - meta.zero_offset;

    // main loop
    while (meta.size > file_offset+buffer_offset+BUFFERSIZE){ // loop until file ends to not segfault
        // actual hot loop
        for (; buffer_offset<BUFFERSIZE; buffer_offset++){
            nextn = (size_t)buffer[buffer_offset]-48;
            droppedn = (size_t)buffer[buffer_offset-num_digits]-48;
            window = window * 10 + nextn - droppedn * pow10mod;

            if (packedbools_set(window, &bools)){
                packedbools_free(&bools);
                size_t dist = file_offset + buffer_offset - num_digits - 1 - meta.zero_offset;
                return (ScanResult){.dist = dist, .last_num = window};
            }
        }

        // buffer ended, swap buffers and manually do the last digits that live in both buffers
        // love to use mmap, but is a pain for cross compat
        file_offset += BUFFERSIZE;
        buffer_tmp = buffer;
        buffer = buffer_prev;
        buffer_prev = buffer_tmp;

        // read the next chunk and just walk "num_digits" steps, nextn from new buffer, droppedn from prev buffer
        fread(buffer, 1, BUFFERSIZE, fptr);
        buffer_offset = 0;
        for (size_t i=0; i<num_digits; i++){
            nextn = (size_t)buffer[buffer_offset]-48;
            droppedn = (size_t)buffer_prev[BUFFERSIZE-num_digits+i]-48;
            window = window * 10 + nextn - droppedn * pow10mod;

            if (packedbools_set(window, &bools)){
                packedbools_free(&bools);
                size_t dist = file_offset + buffer_offset - num_digits - 1 - meta.zero_offset;
                return (ScanResult){.dist=dist, .last_num=window};
            }
            buffer_offset++;
        }
    }

    // last bit of the buffer
    for (;buffer_offset < meta.size % BUFFERSIZE; buffer_offset++){
        nextn = (size_t)buffer[buffer_offset]-48;
        droppedn = (size_t)buffer[buffer_offset-num_digits]-48;
        window = window * 10 + nextn - droppedn * pow10mod;
        if (packedbools_set(window, &bools)){
            packedbools_free(&bools);
            size_t dist = file_offset + buffer_offset - num_digits - 1 - meta.zero_offset;
            return (ScanResult){.dist=dist, .last_num=window};
        }
    }

    return (ScanResult){.status = ERR_INSUFFICIENT_DIGITS};
}

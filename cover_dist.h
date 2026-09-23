#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
void packedbools_set(size_t index, PackedBools* bools){
    size_t byte_index = index >> 3; // divide by 8
    uint8_t mask = 1 << (index & 7); // bit mask
    if (bools->bools[byte_index] & mask) return;
    bools->bools[byte_index] |= mask;
    bools->remaining--;
}

// helper function, just free(bools->bools)
void packedbools_free(PackedBools* bools){
    free(bools->bools);
}

int get_radix_pos(char* file_path){
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

size_t get_file_size(char* file_path){
    FILE* fptr = fopen(file_path, "r");
    if (fptr == NULL){
        perror("Cant open file");
        return 0;
    }
    fseek(fptr, 0L, SEEK_END);
    size_t end = ftell(fptr);
    fclose(fptr);
    return end;
}

ScanResult cover_dist(char* file_path, int num_digits){
    size_t file_size = get_file_size(file_path);
    int radix_pos = get_radix_pos(file_path);
    if (file_size == 0 || radix_pos <= 0) return (ScanResult){0,0};

    FILE* fptr = fopen(file_path, "r"); // assume this to work, otherwise last check wouldve detected issue and returned

    // this uses 2 buffers since we read the file in chunks and need the last chunk (technically only the last "num_digits" digits of the previous chunk)
    // instead of copying the buffers i opted to swap pointers which should be faster
    char buffera[BLOCKSIZE];
    char bufferb[BLOCKSIZE];
    char* buffer = buffera;
    char* buffer_prev = bufferb;
    char* buffer_tmp = NULL;

    fread(buffera, 1, BLOCKSIZE, fptr);
    size_t window = 0;
    size_t pow10mod = 1;

    // move the integer part to the right, overwriting the radix . to give a clean start to the sequence
    memcpy(buffer+1, buffer, radix_pos);

    // disregard the first 0s
    // numbers like ln2 (0.69314) start with a 6, and not with a 0
    // the integer sequence of some number 0.000123 would be (1,2,3)
    size_t number_offset = 1;
    for (; number_offset<BLOCKSIZE; number_offset++){
        if (buffer[number_offset] != 48) break;
    }
    number_offset--; // -- because we used it as an index but have to skip the first char

    // generate the first window
    for (size_t i=num_digits+number_offset; i>number_offset; i--){
        window += (buffer[i]-48) * pow10mod;
        pow10mod *= 10;
    }

    // bitset i think its called by the professionals...
    PackedBools bools = packedbools_make(pow10mod);
    packedbools_set(window, &bools);

    // some more variables
    size_t file_offset, nextn, droppedn;
    size_t block_offset = num_digits + 1 + number_offset;
    file_offset = block_offset - number_offset;

    // main loop
    while (file_size > file_offset+block_offset){ // loop until file ends to not segfault
        // actual hot loop
        for (; block_offset<BLOCKSIZE; block_offset++){
            nextn = buffer[block_offset]-48;
            droppedn = buffer[block_offset-num_digits]-48;
            window = window * 10 + nextn - droppedn * pow10mod;

            packedbools_set(window, &bools);
            if (bools.remaining == 0){
                packedbools_free(&bools);
                size_t dist = file_offset + block_offset - num_digits - 1 - number_offset;
                return (ScanResult){dist, window};
            }
        }

        // buffer ended, swap buffers and manually do the last digits that live in both buffers
        // love to use mmap, but is a pain for cross compat
        file_offset += BLOCKSIZE;
        buffer_tmp = buffer;
        buffer = buffer_prev;
        buffer_prev = buffer_tmp;

        // read the next chunk and just walk "num_digits" steps, nextn from new buffer, droppedn from prev buffer
        fread(buffer, 1, BLOCKSIZE, fptr);
        block_offset = 0;
        for (int i=0; i<num_digits; i++){
            nextn = buffer[block_offset]-48;
            droppedn = buffer_prev[BLOCKSIZE-num_digits+i]-48;
            window = window * 10 + nextn - droppedn * pow10mod;

            if (bools.remaining == 0){
                packedbools_free(&bools);
                size_t dist = file_offset + block_offset - num_digits - 1 - number_offset;
                return (ScanResult){dist, window};
            }
            block_offset++;
        }
    }

    printf("Not enough digits in file");
    return (ScanResult){0, 0};
}

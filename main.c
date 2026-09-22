#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

typedef struct {
    uint8_t* bools;
    size_t num_bools;
} PackedBools;

PackedBools packedbools_make(size_t num_bools){
    size_t bytes_needed = 1;
    if (num_bools > 8) {
        bytes_needed = num_bools / 8 + (num_bools%8>0);
    }
    // printf("bytes_needed: %zu\n", bytes_needed);
    PackedBools bools;
    uint8_t* arr = (uint8_t*)malloc(bytes_needed);
    memset(arr, 0, bytes_needed);
    bools.bools = arr;
    bools.num_bools = num_bools;
    return bools;
}

void packedbools_set(size_t index, PackedBools* bools){
    size_t index_byte = index / 8;
    uint8_t index_bit = index % 8;
    // printf("index: %zu, index_byte: %zu, index_bit: %d\n", index, index_byte, index_bit);
    uint8_t boolpart = bools->bools[index_byte];
    boolpart = boolpart | (1 << (7-index_bit));
    bools->bools[index_byte] = boolpart;
}

bool packedbools_all_set(PackedBools bools){
    size_t num_bytes = bools.num_bools / 8;
    uint8_t last_byte_width = bools.num_bools % 8;
    for (int i=0; i<num_bytes; i++){
        if (bools.bools[i] != 255){
            return false;
        }
    }
    if (last_byte_width && bools.num_bools>8) {
        uint8_t last_expected = 1;
        for (uint8_t i=0; i<last_byte_width; i++){
            last_expected |= last_expected << i;
        }
        last_expected <<= 8 - last_byte_width;
        // printf("last_byte_width: %d, last_expected: %d, last_is: %d\n",last_byte_width, last_expected, bools.bools[num_bytes]);
        return bools.bools[num_bytes] == last_expected;
    }
    return true;
}

void packedbools_print(PackedBools bools){
    uint8_t last_byte_width = bools.num_bools % 8;
    size_t num_bytes = bools.num_bools / 8;
    for (size_t i=0; i<num_bytes; i++){
        uint8_t byte = bools.bools[i];
        for (int j=0; j<8; j++){
            putc(48+((byte>>(7-j))&1), stdout);
        }
    }
    if (last_byte_width){
        uint8_t byte = bools.bools[num_bytes];
        for (int i=0; i<last_byte_width; i++){
            putc(48+((byte>>(7-i))&1), stdout);
        }
    }
    printf("\n");
}

void packedbools_free(PackedBools* bools){
    free(bools->bools);
}

int file_find_radix_point(char* filename){
    FILE* fptr = fopen(filename, "r");
    if (fptr == NULL){
        perror("Cant open file");
        return -1;
    }
    // files made by y-crucher can have an int part of up to ~2**63, so technically 19 chars would be enough
    char window_str[20];
    fgets(window_str, 20, fptr);
    for (int i=0; i<20; i++){
        if (window_str[i] == 46){
            return i;
        }
    }
    return -1;
}

size_t cover_dist(char* filename, uint8_t num_digits){
    // number assume to have a radix . somewhere, skip that
    int radix_pos = file_find_radix_point(filename);
    if (radix_pos == -1){
        perror("Cant find radix pos\n");
        return 0;
    }

    // open file and skip past radix
    FILE* fptr = fopen(filename, "r");
    fseek(fptr, radix_pos+1, SEEK_SET);

    // create bool set
    size_t num_bits = 1;
    for (int i=0; i<num_digits; i++) num_bits *= 10;
    // printf("num_bits: %zu\n", num_bits);
    PackedBools bools = packedbools_make(num_bits);
    char window_str[num_digits];

    // generate 10^k table for k=0 to k=num_digits-1
    size_t pow10[num_digits+1];
    pow10[0] = 1;
    pow10[1] = 10;
    for (int i=2; i<num_digits+1; i++){
        pow10[i] = 10*pow10[i-1];
    }
    size_t pow10mod = pow10[num_digits];

    // init first window
    fgets(window_str, num_digits+1, fptr);
    // printf("window_str=%s\n",window_str);
    size_t window = 0;
    for (int i=0; i<num_digits; i++){
        // printf("window_str[%d]=%d\n", i, window_str[i]);
        window += pow10[i] * (window_str[num_digits-i-1]-48);
    }
    // size_t window = atoi(window_str);
    // printf("first window: %zu\n", window);
    packedbools_set(window, &bools);
    size_t dist = num_digits;

    // main loop
    while (!packedbools_all_set(bools)){
        size_t nextn = fgetc(fptr)-48;
        window = (window * 10 + nextn) % pow10mod;
        // printf("dist: %zu, ", dist);
        // printf("nextn: %zu, ", nextn);
        // printf("window: %zu\n", window);
        packedbools_set(window, &bools);
        // packedbools_print(bools);
        dist++;
        // if (dist > 40) break;
    }

    // cleanup
    printf("last num: %zu\n",window);
    packedbools_free(&bools);
    return dist;
}

int main(){
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    double time_start = (double)t.tv_sec + (double)t.tv_nsec/1e9;
    size_t dist = cover_dist("./nums/pi", 5);
    clock_gettime(CLOCK_MONOTONIC, &t);
    double time_end = (double)t.tv_sec + (double)t.tv_nsec/1e9;
    printf("dist: %zu\n", dist);
    printf("took: %fs\n", time_end - time_start);
    return EXIT_SUCCESS;
}

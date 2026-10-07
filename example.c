#include <stdio.h>
#include "cover_dist.h"

int main(){
    char* file = "./nums/ln2";
    ScanResult res = cover_dist("./test_numbers/e", 5, NULL);

    if (res.status){
        printf("%s\n", strerr_scan(res));
        return 1;
    }

    printf("dist:%zu, last:%zu\n", res.dist, res.last_num);
    return 0;
}


#include <stdio.h>
#include <stdlib.h> 

int main() {
    for (int i = 0; i < 100000; i++) {
        if (i < 0) i = 0;
        double *arr = malloc(i*sizeof(double));
        for (int j = 0; j < i; j++) {
            arr[j] = i * 1. / 3;
        }
        free(arr);
        printf("%lf ", i * 1. / 3);
    }
    return 0;
}
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

int main() {
    int n = 10000000;

#ifdef DOUBLE
    double* array = (double*)malloc(n * sizeof(double));
#else 
    float* array = (float*)malloc(n * sizeof(float));
#endif

    if (array == NULL) {
        printf("Failed to allocate memory\n");
        return 1;
    }

    const double pi = 3.1415926535897932384626;

    for (int i = 0; i < n; ++i) {
        array[i] = sin(2 * pi * i / n);
    }

    long double sum = 0;
    for (int i = 0; i < n; ++i) {
        sum += array[i];
    }

    printf("SUM: %.30Lf\n", sum);

    free(array);

    return 0;
}
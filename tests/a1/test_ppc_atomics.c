#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "a1-bgq-atomics.h"

int main (int argc, char ** argv)
{
    int64_t a=0, b=0, c; 
    int64_t n=(2<<22);

    for (int64_t i=0; i<n; i++)
        Inc64(&a);

    printf("a = %lld \n", (long long int)a);

    for (int64_t i=0; i<n; i++)
        Dec64(&a);

    printf("a = %lld \n", (long long int)a);

    while (b<n)
        b = FetchInc64(&b);

    printf("b = %lld \n", (long long int)b);

    c = 2<<22;

    for (int64_t i=0; i<n; i++)
        Dec64(&c);

    printf("c = %lld \n", (long long int)c);

    for (int64_t i=0; i<n; i++)
        Inc64(&c);

    printf("c = %lld \n", (long long int)c);

    return 0;
}

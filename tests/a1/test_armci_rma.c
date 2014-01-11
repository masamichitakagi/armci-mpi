#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mpi.h>

#include "armci.h"

int main (int argc, char * argv[])
{
    MPI_Init(&argc, &argv);
    A1_Initialize();

    int rank = A1_Rank();
    int size = A1_Size();

    size_t n = (argc>1) ? atoi(argv[1]) : 1000;
    double * buffer = malloc(n*sizeof(double)); assert(buffer!=NULL);
    double ** myptrs;
    ARMCI_Malloc(myptrs, (n*sizeof(double));

    for (size_t i=0; i<n; i++)
        myptrs[rank] = -137.137137137;

    if (size<2) {
        printf("run on at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD,size);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank==0) {
        for (int r=1; r<size; r++) {

            for (size_t i=0; i<n; i++)
                buffer[i] = 100.0 + r;

            A1_Put(buffer, n*sizeof(double), r, myptrs[r]);
            ARMCI_Fence(r);

            for (size_t i=0; i<n; i++)
                buffer[i] = 1000000.0 + 10000.0*r;

            A1_Acc(buffer, n, A1_DOUBLE, r, myptrs[r]);
            ARMCI_Fence(r);

            A1_Get(r, myptrs[r], buffer, n*sizeof(double));

            for (size_t i=0; i<n; i++)
                printf("%d: buffer[%ld] = %lf \n", r, (long)i, buffer[i]);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    free(myptrs);
    free(buffer);

    A1_Finalize();
    MPI_Finalize();

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mpi.h>

#include "armci.h"

int main (int argc, char * argv[])
{
    MPI_Init(&argc, &argv);
    ARMCI_Init();

    int rank;
    int size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    size_t n = (argc>1) ? atoi(argv[1]) : 1000;
    double * buffer = malloc(n*sizeof(double)); assert(buffer!=NULL);
    double ** myptrs;
    ARMCI_Malloc((void**)myptrs, n*sizeof(double));

    for (size_t i=0; i<n; i++)
        myptrs[rank][i] = -137.137137137;

    if (size<2) {
        printf("run on at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD,size);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank==0) {
        for (int r=1; r<size; r++) {

            for (size_t i=0; i<n; i++)
                buffer[i] = 100.0 + r;

            ARMCI_Put(buffer, myptrs[r], n*sizeof(double), r);
            ARMCI_Fence(r);

            for (size_t i=0; i<n; i++)
                buffer[i] = 1000000.0 + 10000.0*r;

            double one = 1.0;
            ARMCI_Acc(ARMCI_ACC_DBL, &one, buffer, myptrs[r], n, r);
            ARMCI_Fence(r);

            ARMCI_Get(r, myptrs[r], buffer, n*sizeof(double));

            for (size_t i=0; i<n; i++)
                printf("%d: buffer[%ld] = %lf \n", r, (long)i, buffer[i]);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    ARMCI_Free(myptrs[rank]);
    free(buffer);

    ARMCI_Finalize();
    MPI_Finalize();

    return 0;
}

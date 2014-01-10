#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <mpi.h>

#include "a1.h"

int main (int argc, char * argv[])
{
    MPI_Init(&argc, &argv);
    A1_Initialize();

    int rank = A1_Rank();
    int size = A1_Size();

    size_t n = (argc>1) ? atoi(argv[1]) : 1000;
    double *  buffer = malloc(n*sizeof(double)); assert(buffer!=NULL);
    double ** myptrs = malloc(size*sizeof(double*)); assert(myptr!=NULL);

    MPI_Allgather(&buffer, sizeof(double*), MPI_BYTE,
                  myptrs,  sizeof(double*), MPI_BYTE,
                  MPI_COMM_WORLD);

    printf("%d: buffer = %p \n", rank, buffer);
    if (rank==0)
        for (int i=0; i<size; i++)
            printf("myptrs[%d] = %p \n", rank, myptrs[i]);
/*
    A1_Put(void * local,
           size_t bytes,
           int    target,
           void * remote);

    A1_Acc(void *        local,
           size_t        count,
           A1_datatype_t type,
           int           target,
           void *        remote);

    A1_Get(int    target,
           void * remote,
           void * local,
           size_t bytes);
*/

    free(myptrs);
    free(buffer);

    A1_Finalize();
    MPI_Finalize();

    return 0;
}

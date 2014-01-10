#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <armci.h>

int main(int argc, char **argv)
{
    int rank, nranks;
    int provided;
    int *buffer;
    char op = '+';

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    ARMCI_Init_args(&argc, &argv);
    ARMCI_Barrier();

    buffer = (int *) malloc(1024*sizeof(int));

    for(int i=0; i<1024; i++) {
       buffer[i] = 10;
    }

    armci_msg_igop(buffer, 1024*sizeof(int), &op); 

    for(int i=0; i<1024; i++) {
       if(buffer[i] != 10*nranks) {
          printf("Validation failed expected: %d actual: %d \n", 10*nranks, buffer[i]);
          fflush(stdout);
          exit(-1);
       }  
    }

    printf("Validation successful \n");
    fflush(stdout);

    free(buffer);
    ARMCI_Finalize();
    MPI_Finalize();
    return 0;
}

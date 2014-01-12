#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <armci.h>

#define MAX_MSG_SIZE 1024*1024

int main(int argc, char **argv)
{
    int rank, nranks;
    int *buffer;
    int provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    ARMCI_Init_args(&argc, &argv);

    ARMCI_Barrier();

    buffer = (int *) malloc(MAX_MSG_SIZE);

    for(int i=0; i<MAX_MSG_SIZE/sizeof(int); i++) {
        buffer[i] = (rank==0) ? ((2<<20) - 1) : 0;
    }

    if(rank == 0) {
      printf("Testing functionality of ARMCI_Bcast \n");
      fflush(stdout);
    } 

    for(int msgsize=(int)sizeof(int); msgsize<=MAX_MSG_SIZE; msgsize*=2) {
       armci_msg_bcast(buffer, msgsize, 0); 

       for(int i=0; i<msgsize/sizeof(int); i++) {
          if(buffer[i] != ((2<<20) - 1))
          {
             printf("[%d] Validation failed for msg size: %d at index: %d expected: %d actual: %d \n",
                     rank, msgsize, i, ((2<<20) - 1), buffer[i]);
             fflush(stdout);
          }  
       }

       for(int i=0; i<MAX_MSG_SIZE/sizeof(int); i++) {
          buffer[i] = (rank==0) ? ((2<<20) - 1) : 0;
       }

       ARMCI_Barrier();

       if(rank == 0) {
         printf("Validation successful for msg size: %d\n", msgsize);
         fflush(stdout);
       }
    }
    free(buffer);
    ARMCI_Finalize();
    MPI_Finalize();
    return 0;
}

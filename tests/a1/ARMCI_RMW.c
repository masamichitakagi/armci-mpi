#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <armci.h>

#define COUNT 1024*1024

int main(int argc, char* argv[])
{
    int provided;
    int rank, nranks, target;
    int **counter;
    int *complete;
    int increment;
    int counter_fetch;
    int counters_received;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    ARMCI_Init_args(&argc, &argv);

    complete = (int *) malloc(sizeof(int) * COUNT);

    counter = (int**) ARMCI_Malloc_local( nranks * sizeof(int*) );
    ARMCI_Malloc((void **) counter[rank], sizeof(int));

    if (rank == 0) {
        printf("ARMCI_RMW Test - in usec \n");
        fflush(stdout);
    }

    target = 0; 

    for (int i=0; i<COUNT; i++) {
       complete[i] = 0;
    } 
    if (rank == target) { 
       *(counter[rank]) = 0;
    }
    increment = 1;
    counter_fetch = 0;
    counters_received = 0;

    MPI_Barrier(MPI_COMM_WORLD);
 
    while(counter_fetch < COUNT) {  
        ARMCI_Rmw(ARMCI_FETCH_AND_ADD,
                  (void *) &counter_fetch,
                  (void *) counter[target],
                  increment,
                  target);

        /* s/1/rank/ means we will know who got the counter */
        if (counter_fetch < COUNT) complete[counter_fetch] = rank;
        counters_received++;
    }

    MPI_Allreduce(MPI_IN_PLACE,complete,COUNT,MPI_INT,MPI_SUM,MPI_COMM_WORLD);

    for (int i=0; i<COUNT; i++) {
       if (complete[i] == 0) {
           printf("[%d] The RMW update failed at index: %d \n", rank, i);
           fflush(stdout);
           exit(-1);
       }   
    }
    printf("[%d] The RMW update completed successfully \n", rank);
    fflush(stdout);
    MPI_Barrier(MPI_COMM_WORLD);

    if (0==rank) {
        printf("Checking for fairness...\n");
        fflush(stdout);
        for(int i=0; i<COUNT; i++) {
           printf("counter value %d was received by process %d\n", i, complete[i]);
        }
        fflush(stdout);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    printf("process %d received %d counters\n", rank, counters_received);
    fflush(stdout);

    ARMCI_Free(counter[rank]);
    ARMCI_Free_local(counter);

    ARMCI_Finalize();

    MPI_Finalize();

    return 0;
}

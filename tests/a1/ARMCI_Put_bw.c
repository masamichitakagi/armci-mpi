#include <stdio.h>
#include <stdlib.h>
#include <armci.h>
#include <mpi.h>

#define MAX_MSGSIZE 2*1024*1024
#define ITERATIONS 20 

int main(int argc, char *argv[])
{
    int provided;
    int rank, nranks, dest;
    int iterations, max_msgsize;
    int bufsize;
    double **buffer;
    double t_start, t_stop, t_total, d_total;
    double bandwidth;
    armci_hdl_t handle;

    max_msgsize = MAX_MSGSIZE;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    ARMCI_Init_args(&argc, &argv);

    bufsize = max_msgsize * ITERATIONS;
    buffer = (double **) malloc(sizeof(double *) * nranks);
    ARMCI_Malloc((void **) buffer, bufsize);

    for (int i = 0; i < bufsize / sizeof(double); i++) {
        buffer[rank][i] = 1.0 + rank;
    }

    ARMCI_INIT_HANDLE(&handle);
    ARMCI_SET_AGGREGATE_HANDLE(&handle);

    ARMCI_Barrier();
    if (rank == 0) { 
        printf("ARMCI_Put Bandwidth in MBPS \n");
        printf("%20s %22s \n", "Message Size", "Bandwidth");
        fflush(stdout);
        dest = 1;

        for (int msgsize = sizeof(double); msgsize <= max_msgsize; msgsize *= 2) {
            iterations = bufsize/msgsize;
            t_start = MPI_Wtime();
            for (int i = 0; i < iterations; i++) { 
                ARMCI_NbPut(&(buffer[dest][i*msgsize]), 
                            &(buffer[rank][i*msgsize]), 
                            msgsize, dest, &handle);
            }
            ARMCI_Wait(&handle);
            t_stop = MPI_Wtime();
            d_total = (iterations * msgsize) / (1024 * 1024);
            t_total = t_stop - t_start;
            bandwidth = d_total / t_total;
            printf("%20d %20.4lf \n", msgsize, bandwidth);
            fflush(stdout);
            ARMCI_Fence(dest);
        }

    }
    ARMCI_Barrier();
    ARMCI_UNSET_AGGREGATE_HANDLE(&handle);
    ARMCI_Free((void *) buffer[rank]);
    ARMCI_Finalize();
    MPI_Finalize(); 
    return 0;
}

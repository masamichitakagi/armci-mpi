#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <armci.h>

#define MAX_MSG_SIZE 1024*1024
#define ITERATIONS 100
#define SKIP 10

int main(int argc, char **argv)
{
    int rank, nranks, msgsize;
    long bufsize;
    double **buffer;
    double t_start, t_stop, t_latency = 0;
    int provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    ARMCI_Init_args(&argc, &argv);

    buffer = (double **) malloc(sizeof(double *) * nranks);

    bufsize = MAX_MSG_SIZE * (ITERATIONS + SKIP);
    ARMCI_Malloc((void **) buffer, bufsize);

    for (int i = 0; i<(int)bufsize/sizeof(double); i++)
        *(buffer[rank] + i) = 1.0 + rank;

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        printf("ARMCI_Put + FlushAll Latency - in usec \n");
        printf("%20s %22s\n", "Message Size", "Latency");
        fflush(stdout);
        for (msgsize = sizeof(double); msgsize < MAX_MSG_SIZE; msgsize *= 2) { 
            for (int i = 0; i < ITERATIONS + SKIP; i++) { 
                for (int j = 0; j < nranks; j++) { 
                    ARMCI_Put(&(buffer[rank][i*msgsize]),&(buffer[j][i*msgsize]), msgsize, j);
                }
                t_start = MPI_Wtime();
                ARMCI_AllFence();
                t_stop = MPI_Wtime();
                if (i >= SKIP) t_latency = t_latency + (t_stop - t_start);
            }
            printf("%20d %20.2f \n", msgsize, ((t_latency) * 1000000)
                    / ITERATIONS);
            fflush(stdout);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    ARMCI_Free(buffer[rank]);
    ARMCI_Finalize();
    MPI_Finalize();
    return 0;
}

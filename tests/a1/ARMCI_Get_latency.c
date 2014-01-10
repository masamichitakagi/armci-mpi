#include <stdio.h>
#include <stdlib.h>
#include <armci.h>
#include <mpi.h>

#define MAX_MSG_SIZE 1024*1024
#define ITERATIONS 100
#define SKIP 10

int main(int argc, char **argv)
{
    int rank, nranks, dest;
    long bufsize;
    double **buffer;
    double t_start, t_stop;
    int provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    ARMCI_Init_args(&argc, &argv);

    bufsize = MAX_MSG_SIZE * (ITERATIONS + SKIP);
    buffer = (double **) malloc(sizeof(double *) * nranks);
    ARMCI_Malloc((void **) buffer, bufsize);

    for (int i = 0; i < bufsize / sizeof(double); i++) {
        buffer[rank][i] = 1.0 + rank;
    }

    ARMCI_Barrier();

    if (rank == 0) {
        printf("ARMCI_Get Latency in usec \n");
        printf("%20s %22s \n", "Message Size", "Latency");
        fflush(stdout);
        dest = 1;
        for (int msgsize = sizeof(double); msgsize <= MAX_MSG_SIZE; msgsize *= 2) { 
            for (int i = 0; i < ITERATIONS + SKIP; i++) { 
                if (i == SKIP) t_start = MPI_Wtime();
                ARMCI_Get(&(buffer[dest][i*msgsize]), &(buffer[rank][i*msgsize]), msgsize, 1);
            }
            t_stop = MPI_Wtime();
            printf("%20d %20.2f \n", msgsize, ((t_stop - t_start) * 1000000)
                    / ITERATIONS);
            fflush(stdout);

            for (int i = 0; i < ((ITERATIONS + SKIP) * msgsize) / sizeof(double); i++) {
                if (*(buffer[rank] + i) != (1.0 + dest)) {
                    printf("Data validation failed At displacement : %d Expected : %f Actual : %f \n",
                           i, (1.0 + dest), buffer[rank][i]);
                    fflush(stdout);
                    return 1;
                }
            }
            for (int i = 0; i < bufsize / sizeof(double); i++) {
                buffer[rank][i] = 1.0 + rank;
            }
        }
    }
    ARMCI_Barrier();
    ARMCI_Free(buffer[rank]);
    MPI_Finalize();
    return 0;
}

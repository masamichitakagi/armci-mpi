#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <armci.h>

#define MAX_MSG_SIZE 1024*1024
#define ITERATIONS 100
#define SKIP 10

int main(int argc, char **argv)
{
    int provided;
    int rank, nranks, msgsize;
    long bufsize;
    double scaling;
    double **buffer;
    double t_start, t_stop, t_latency = 0;
    int count[2], src_stride, trg_stride, stride_level;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    ARMCI_Init_args(&argc, &argv);

    buffer = (double **) malloc(sizeof(double *) * nranks);

    bufsize = MAX_MSG_SIZE * (ITERATIONS + SKIP);
    ARMCI_Malloc((void **) buffer, bufsize);

    scaling = 2.0;
    for(int i = 0; i < bufsize / sizeof(double); i++) {
        *(buffer[rank] + i) = 1.0 + rank;
    }
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        printf("TESTING ALL-FLUSH-ALL\n");
        printf("ARMCI_Put + ARMCI_Barrier Latency - in usec \n");
        printf("%20s %22s\n", "Message Size", "Latency");
        fflush(stdout);
    }

    for (msgsize = sizeof(double); msgsize < MAX_MSG_SIZE; msgsize *= 2) {
        MPI_Barrier(MPI_COMM_WORLD);
        for(int i = 0; i < ITERATIONS + SKIP; i++) {
            for(int j = 0; j < nranks; j++) {
                ARMCI_Put(&(buffer[rank][i*msgsize]),
                          &(buffer[j][i*msgsize]),
                          msgsize,
                          j);
            }
            t_start = MPI_Wtime();
            ARMCI_AllFence();
            MPI_Barrier(MPI_COMM_WORLD);
            t_stop = MPI_Wtime();
            if (i >= SKIP) t_latency = t_latency + (t_stop - t_start);
        }
        printf("%20d %20.2f \n", msgsize, ((t_latency) * 1000000) / ITERATIONS);
        fflush(stdout);
        MPI_Barrier(MPI_COMM_WORLD);
    }

    if (0 == rank) {
        printf("\n");
        printf("ARMCI_Acc + ARMCI_Barrier - in usec \n");
        printf("%20s %22s\n", "Message Size", "Latency");
        fflush(stdout);
    }

    stride_level = 0;
    for (msgsize = sizeof(double); msgsize < MAX_MSG_SIZE; msgsize *= 2) {
        src_stride = msgsize * sizeof(double);
        trg_stride = msgsize * sizeof(double);
        count[0]   = msgsize * sizeof(double);
        count[1]   = msgsize;
        MPI_Barrier(MPI_COMM_WORLD);
        for(int i = 0; i < ITERATIONS + SKIP; i++) {
            for(int j = 0; j < nranks; j++) {
                ARMCI_AccS(ARMCI_ACC_DBL,
                           &scaling,
                           &(buffer[rank][i*msgsize]),
                           &src_stride,
                           &(buffer[j][i*msgsize]),
                           &trg_stride,
                           count,
                           stride_level,
                           j);
            }
            t_start = MPI_Wtime();
            ARMCI_AllFence();
            MPI_Barrier(MPI_COMM_WORLD);
            t_stop = MPI_Wtime();
            if (i >= SKIP) t_latency = t_latency + (t_stop - t_start);
        }
        printf("%20d %20.2f \n", msgsize, ((t_latency) * 1000000) / ITERATIONS);
        fflush(stdout);
        MPI_Barrier(MPI_COMM_WORLD);
    }

    ARMCI_Free(buffer[rank]);
    ARMCI_Finalize();
    MPI_Finalize();

    return 0;
}

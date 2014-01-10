#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <armci.h>

#define MAX_DIM 1024 
#define ITERATIONS 100
#define SKIP 10

int main(int argc, char **argv)
{
    int rank, nranks, dest;
    int dim;
    long bufsize;
    double **buffer;
    double t_start, t_stop;
    int count[2], src_stride, trg_stride, stride_level;
    int provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    ARMCI_Init_args(&argc, &argv);

    bufsize = MAX_DIM * MAX_DIM * sizeof(double);
    buffer = (double **) malloc(sizeof(double *) * nranks);
    ARMCI_Malloc((void **) buffer, bufsize);

    for (int i = 0; i < bufsize / sizeof(double); i++) {
        buffer[rank][i] = 1.0 + rank;
    }

    ARMCI_Barrier();

    if (rank == 0)
    {
        printf("ARMCI_GetS Latency in usec \n");
        printf("%30s %22s \n", "Dimensions(array of doubles)", "Latency");
        fflush(stdout);

        dest = 1;

        src_stride = MAX_DIM * sizeof(double);
        trg_stride = MAX_DIM * sizeof(double);
        stride_level = 1;

        for (dim = 1; dim <= MAX_DIM; dim *= 2) { 
            count[0] = dim*sizeof(double);
            count[1] = 512;
            for (int i = 0; i < ITERATIONS + SKIP; i++) { 
                if (i == SKIP) t_start = MPI_Wtime();
                ARMCI_GetS(buffer[dest],
                           &src_stride,
                           buffer[rank],
                           &trg_stride,
                           count,
                           stride_level,
                           1);
            }
            t_stop = MPI_Wtime();

            char temp[10];
            sprintf(temp, "%dX%d", count[1], count[0]);
            printf("%30s %20.2f \n", temp, ((t_stop - t_start) * 1000000)
                    / ITERATIONS);
            fflush(stdout);

            for (int i = 0; i < count[1]; i++) {
                for (int j = 0; j < count[0]; j++) {
                    if (*(buffer[rank] + i * MAX_DIM + j) != (1.0 + dest)) {
                        printf("Data validation failed at X: %d Y: %d Expected : %f Actual : %f \n",
                               i, j, (1.0 + dest), buffer[rank][i*MAX_DIM+j]);
                        fflush(stdout);
                        return -1;
                    }
                }
            }
        }
    } 
    ARMCI_Barrier(); 
    ARMCI_Free((void *) buffer[rank]); 
    ARMCI_Finalize(); 
    MPI_Finalize(); 
    return 0;
}

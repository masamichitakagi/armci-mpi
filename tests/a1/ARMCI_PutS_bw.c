#include <stdio.h>
#include <stdlib.h>
#include <armci.h>
#include <mpi.h>

#define MAX_DIM 1024 

int main(int argc, char *argv[])
{
    int rank, nranks, dest;
    int iterations;
    long bufsize;
    double **buffer;
    double t_start, t_stop, t_total, d_total, bw;
    int count[2], src_stride, trg_stride, stride_level;
    int provided;
    armci_hdl_t handle;

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

    ARMCI_INIT_HANDLE(&handle);
    ARMCI_SET_AGGREGATE_HANDLE(&handle);

    ARMCI_Barrier();

    if (rank == 0) {
        printf("ARMCI_PutS Bandwidth in MB/s \n");
        printf("%s %s \n", "Dimensions(array of doubles)", "Latency");
        fflush(stdout);

        dest = 1;

        src_stride = MAX_DIM * sizeof(double);
        trg_stride = MAX_DIM * sizeof(double);
        stride_level = 1;

        for (int dim = 1; dim <= MAX_DIM; dim *= 2) { 
            count[0] = dim*sizeof(double);
            count[1] = dim;
            iterations = 10*(MAX_DIM * MAX_DIM)/(dim * dim);
            t_start = MPI_Wtime();
            for (int i = 0; i < iterations; i++) { 
                ARMCI_NbPutS(buffer[rank],
                             &src_stride,
                             buffer[dest],
                             &trg_stride,
                             count,
                             stride_level,
                             dest,
                             &handle);
            }
            ARMCI_Wait(&handle);
            t_stop = MPI_Wtime();
            ARMCI_Fence(1);

            char temp[10];
            sprintf(temp, "%dX%d", dim, dim);
            t_total = t_stop - t_start;
            d_total = (dim*dim*sizeof(double)*iterations)/(1024*1024);
            bw = d_total/t_total;
            printf("%30s %20.2f \n", temp, bw);
            fflush(stdout);
        }
    }
    ARMCI_Barrier();
    ARMCI_UNSET_AGGREGATE_HANDLE(&handle);
    ARMCI_Free((void *) buffer[rank]);
    ARMCI_Finalize();
    MPI_Finalize();
    return 0;
}

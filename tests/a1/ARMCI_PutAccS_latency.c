#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <armci.h>

#define MAX_DIM 1024 
#define ITERATIONS 100
#define SKIP 10

int main(int argc, char **argv)
{

    int rank, nranks, peer;
    int dim;
    unsigned long bufsize;
    double **buffer;
    double t_start, t_stop;
    int count[2], src_stride, trg_stride, stride_level;
    double scaling;
    int provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    ARMCI_Init_args(&argc, &argv);

    buffer = (double **) malloc(sizeof(int32_t *) * nranks);

    bufsize = MAX_DIM * MAX_DIM * sizeof(double);
    ARMCI_Malloc((void **) buffer, bufsize);

    if (rank == 0)
    {
        printf("ARMCI_AccS Latency - local and remote completions - in usec \n");
        printf("%30s %22s %22s\n",
               "Dimensions(array of double)",
               "Local Completion",
               "Remote completion");
        fflush(stdout);
    }

    for (int i = 0; i < bufsize / sizeof(double); i++)
    {
        *(buffer[rank] + i) = 1.0 + rank;
    }
    scaling = 2.0;

    src_stride = MAX_DIM * sizeof(double);
    trg_stride = MAX_DIM * sizeof(double);
    stride_level = 1;

    ARMCI_Barrier();

    for (dim = 1; dim <= MAX_DIM; dim *= 2)
    {

        count[0] = dim*sizeof(double);
        count[1] = dim;

            if (rank == 0)
            {

                peer = 1;

                for (int i = 0; i < ITERATIONS + SKIP; i++)
                {

                    if (i == SKIP) t_start = MPI_Wtime();

                    ARMCI_AccS(ARMCI_ACC_DBL,
                               (void *) &scaling,
                               (void *) buffer[rank],
                               &src_stride,
                               (void *) buffer[peer],
                               &trg_stride,
                               count,
                               stride_level,
                               1);

                }
                t_stop = MPI_Wtime();
                ARMCI_Fence(1);

                char temp[10];
                sprintf(temp, "%dX%d", dim, dim);
                printf("%30s %20.2f ", temp, ((t_stop - t_start) * 1000000)
                        / ITERATIONS);
                fflush(stdout);

                ARMCI_Barrier();

                ARMCI_Barrier();

                for (int i = 0; i < ITERATIONS + SKIP; i++)
                {

                    if (i == SKIP) t_start = MPI_Wtime();

                    ARMCI_AccS(ARMCI_ACC_DBL,
                               (void *) &scaling,
                               (void *) buffer[rank],
                               &src_stride,
                               (void *) buffer[peer],
                               &trg_stride,
                               count,
                               stride_level,
                               1);
                    ARMCI_Fence(1);

                }
                t_stop = MPI_Wtime();
                printf("%20.2f \n", ((t_stop - t_start) * 1000000) / ITERATIONS);
                fflush(stdout);

                ARMCI_Barrier();

                ARMCI_Barrier();

            }
            else
            {

                peer = 0;

                ARMCI_Barrier();

                for (int i = 0; i < dim; i++)
                {
                    for (int j = 0; j < dim; j++)
                    {
                        if (*(buffer[rank] + i * MAX_DIM + j) != ((1.0 + rank)
                                + scaling * (1.0 + peer) * (ITERATIONS + SKIP)))
                        {
                            printf("Data validation failed at X: %d Y: %d Expected : %f Actual : %f \n",
                                   i,
                                   j,
                                   ((1.0 + rank) + scaling * (1.0 + peer)),
                                   *(buffer[rank] + i * MAX_DIM + j));
                            fflush(stdout);
                            return -1;
                        }
                    }
                }

                for (int i = 0; i < bufsize / sizeof(double); i++)
                {
                    *(buffer[rank] + i) = 1.0 + rank;
                }

                ARMCI_Barrier();

                ARMCI_Barrier();

                for (int i = 0; i < dim; i++)
                {
                    for (int j = 0; j < dim; j++)
                    {
                        if (*(buffer[rank] + i * MAX_DIM + j) != ((1.0 + rank)
                                + scaling * (1.0 + peer) * (ITERATIONS + SKIP)))
                        {
                            printf("Data validation failed at X: %d Y: %d Expected : %f Actual : %f \n",
                                   i,
                                   j,
                                   ((1.0 + rank) + scaling * (1.0 + peer)),
                                   *(buffer[rank] + i * MAX_DIM + j));
                            fflush(stdout);
                            return -1;
                        }
                    }
                }

                for (int i = 0; i < bufsize / sizeof(double); i++)
                {
                    *(buffer[rank] + i) = 1.0 + rank;
                }

                ARMCI_Barrier();

            }

    }

    ARMCI_Barrier();

    ARMCI_Free((void *) buffer[rank]);

    ARMCI_Finalize();

    MPI_Finalize();

    return 0;
}

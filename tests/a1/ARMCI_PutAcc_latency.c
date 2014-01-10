#include <stdio.h>
#include <stdlib.h>
#include <armci.h>
#include <mpi.h>

#define MAX_MSG_SIZE 1024*1024
#define ITERATIONS 100
#define SKIP 10

int main(int argc, char **argv)
{

    int rank, nranks;
    int i, msgsize, peer;
    long bufsize;
    double **buffer;
    double scaling;
    double t_start, t_stop, t_latency;
    int provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    
    ARMCI_Init_args(&argc, &argv);

    buffer = (double **) malloc(sizeof(int32_t *) * nranks);

    bufsize = MAX_MSG_SIZE * (ITERATIONS + SKIP);
    ARMCI_Malloc((void **) buffer, bufsize);

    if (rank == 0)
    {
        printf("ARMCI_PutAcc Latency in usec \n");
        printf("%20s %22s %22s\n",
               "Message Size",
               "Local Completion",
               "Remote Completion");
        fflush(stdout);
    }

    for (i = 0; i < (((ITERATIONS + SKIP) * MAX_MSG_SIZE) / sizeof(double)); i++)
    {
        *(buffer[rank] + i) = 1.0 + rank;
    }
    scaling = 2.0;

    ARMCI_Barrier();

    for (msgsize = sizeof(double); msgsize < MAX_MSG_SIZE; msgsize *= 2)
    {

        if (rank == 0)
        {

            peer = 1;

            /** Local Completion **/
            for (i = 0; i < ITERATIONS + SKIP; i++)
            {

                if (i == SKIP) t_start = MPI_Wtime();

                ARMCI_Acc(ARMCI_ACC_DBL,
                          (void *) &scaling,
                          (void *) ((int) buffer[rank] + (int)(i
                                  * msgsize)),
                          (void *) ((int) buffer[peer] + (int)(i
                                  * msgsize)),
                          msgsize,
                          peer);

            }
            t_stop = MPI_Wtime();
            ARMCI_Fence(peer);
            printf("%20d %20.2f ", msgsize, ((t_stop - t_start) * 1000000)
                    / ITERATIONS);
            fflush(stdout);

            ARMCI_Barrier();

            ARMCI_Barrier();

            for (i = 0; i < ITERATIONS + SKIP; i++)
            {

                if (i == SKIP) t_start = MPI_Wtime();

                ARMCI_Acc(ARMCI_ACC_DBL,
                          (void *) &scaling,
                          (void *) ((int) buffer[rank] + (int)(i
                                  * msgsize)),
                          (void *) ((int) buffer[peer] + (int)(i
                                  * msgsize)),
                          msgsize,
                          peer);
                ARMCI_Fence(peer);

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

            /** Data Validation **/
            for (i = 0; i < (((ITERATIONS + SKIP) * msgsize) / sizeof(double)); i++)
            {
                if (*(buffer[rank] + i) != ((1.0 + rank) + scaling * (1.0
                        + peer)))
                {
                    printf("Data validation failed At displacement : %d Expected : %f Actual : %f \n",
                           i,
                           ((1.0 + rank) + scaling * (1.0 + peer)),
                           *(buffer[rank] + i));
                    fflush(stdout);
                    return -1;
                }
            }

            for (i = 0; i < (((ITERATIONS + SKIP) * MAX_MSG_SIZE)
                    / sizeof(double)); i++)
            {
                *(buffer[rank] + i) = 1.0 + rank;
            }

            ARMCI_Barrier();

            ARMCI_Barrier();

            /** Data Validation **/
            for (i = 0; i < (((ITERATIONS + SKIP) * msgsize) / sizeof(double)); i++)
            {
                if (*(buffer[rank] + i) != ((1.0 + rank) + scaling * (1.0
                        + peer)))
                {
                    printf("Data validation failed At displacement : %d Expected : %f Actual : %f \n",
                           i,
                           ((1.0 + rank) + scaling * (1.0 + peer)),
                           *(buffer[rank] + i));
                    fflush(stdout);
                    return -1;
                }
            }

            for (i = 0; i < (((ITERATIONS + SKIP) * MAX_MSG_SIZE)
                    / sizeof(double)); i++)
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

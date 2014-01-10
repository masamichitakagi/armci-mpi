#include <stdio.h>
#include <stdlib.h>
#include <armci.h>
#include <mpi.h>

#define MAX_MSG_SIZE 1024*1024
#define ITERATIONS 100
#define SKIP 10

int main(int argc, char *argv[])
{
    int rank, nranks, peer;
    long bufsize;
    double **buffer;
    double t_start, t_stop;
    int provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    ARMCI_Init_args(&argc, &argv);

    if (nranks != 2) {
        printf("[%d] This test requires only two processes \n", rank);
        fflush(stdout); 
        ARMCI_Finalize(); 
        return -1;
    }

    buffer = (double **) malloc(sizeof(double *) * nranks);

    bufsize = MAX_MSG_SIZE * (ITERATIONS + SKIP);
    buffer = (double **) malloc(sizeof(double *) * nranks);
    ARMCI_Malloc((void **) buffer, bufsize);

    for (int i = 0; i < bufsize/sizeof(double); i++) {
        buffer[rank][i] = 1.0 + rank;
    }

    if (rank == 0) {
        printf("ARMCI_Put Latency - local and remote completions - in usec \n");
        printf("%20s %22s %22s\n",
               "Message Size",
               "Latency-LocalCompelte",
               "Latency-RemoteComplete");
        fflush(stdout);
    }

    ARMCI_Barrier();

    for (int msgsize = sizeof(double); msgsize < MAX_MSG_SIZE; msgsize *= 2) { 
        if (rank == 0) { 
            peer = 1; 
            for (int i = 0; i < ITERATIONS + SKIP; i++) { 
                if (i == SKIP) 
                   t_start = MPI_Wtime();

                ARMCI_Put(&(buffer[rank][i*msgsize]),
                          &(buffer[peer][i*msgsize]),
                          msgsize, peer);
            }
            t_stop = MPI_Wtime();
            ARMCI_Fence(peer);
            printf("%20d %20.2f", msgsize, ((t_stop - t_start) * 1000000)
                    / ITERATIONS);
            fflush(stdout);

            ARMCI_Barrier();

            for (int i = 0; i < ITERATIONS + SKIP; i++) { 
                if (i == SKIP) 
                   t_start = MPI_Wtime();
                ARMCI_Put(&(buffer[rank][i*msgsize]),
                          &(buffer[peer][i*msgsize]),
                          msgsize, peer);
                ARMCI_Fence(peer);
            }
            t_stop = MPI_Wtime();
            printf("%20.2f \n", ((t_stop - t_start) * 1000000) / ITERATIONS);
            fflush(stdout);
            ARMCI_Barrier();
        }
        else {
            peer = 0;
            ARMCI_Barrier();

            /** Data Validation **/
            for (int i = 0; i < ( (ITERATIONS + SKIP) * msgsize / sizeof(double)); i++) {
                if (*(buffer[rank] + i) != (1.0 + peer)) {
                    printf("Data validation failed At displacement : %d Expected : %f Actual : %f \n",
                           i, (1.0 + peer), buffer[rank][i]);
                    fflush(stdout);
                    return -1;
                }
            }

            for (int i = 0; i<(int)(bufsize/sizeof(double)); i++) {
                buffer[rank][i] = 1.0 + rank;
            }
            ARMCI_Barrier();

            /** Data Validation **/
            for (int i = 0; i < ((ITERATIONS + SKIP) * msgsize / sizeof(double)); i++) {
                if (buffer[rank][i] != (1.0 + peer)) {
                    printf("Data validation failed At displacement : %d Expected : %f Actual : %f \n",
                           i, (1.0 + peer), buffer[rank][i]);
                    fflush(stdout);
                    return -1;
                }
            }
            for (int i = 0; i < (int)(bufsize/sizeof(double)); i++) {
                buffer[rank][i] = 1.0 + rank;
            }
            ARMCI_Barrier();
        }        
    }
    ARMCI_Barrier();
    ARMCI_Free(buffer[rank]);
    MPI_Finalize();
    return 0;
}

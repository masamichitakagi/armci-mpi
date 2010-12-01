#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <armci.h>

#define XDIM 1024 
#define YDIM 1024
#define ITERATIONS 10

int main(int argc, char **argv) {
    int i, j, rank, nranks, peer, bufsize, errors;
    double **buffer, *src_buf;
    int count[2], src_stride, trg_stride, stride_level;
    double scaling;

    MPI_Init(&argc, &argv);
    ARMCI_Init();

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    buffer = (double **) malloc(sizeof(double *) * nranks);

    bufsize = XDIM * YDIM * sizeof(double);
    ARMCI_Malloc((void **) buffer, bufsize);
    src_buf = ARMCI_Malloc_local(bufsize);

    if (rank == 0)
        printf("ARMCI Strided Accumulate Test:\n");

    ARMCI_Access_begin(buffer[rank]);

    for (i = 0; i < XDIM*YDIM; i++) {
        *(buffer[rank] + i) = 1.0 + rank;
        *(src_buf + i) = 1.0 + rank;
    }

    ARMCI_Access_end(buffer[rank]);

    scaling = 2.0;

    src_stride = XDIM * sizeof(double);
    trg_stride = XDIM * sizeof(double);
    stride_level = 1;

    count[1] = YDIM;
    count[0] = XDIM * sizeof(double);

    ARMCI_Barrier();

    peer = (rank+1) % nranks;

    for (i = 0; i < ITERATIONS; i++) {

      ARMCI_AccS(ARMCI_ACC_DBL,
          (void *) &scaling,
          src_buf,
          &src_stride,
          (void *) buffer[peer],
          &trg_stride,
          count,
          stride_level,
          peer);
    }

    ARMCI_Barrier();

    ARMCI_Access_begin(buffer[rank]);
    for (i = errors = 0; i < XDIM; i++) {
      for (j = 0; j < YDIM; j++) {
        if (*(buffer[rank] + i + j*XDIM) != ((1.0 + rank) + scaling * (1.0 + ((rank+nranks-1)%nranks)) * (ITERATIONS))) {
          printf("%d: Data validation failed at X: %d Y: %d Expected : %f Actual : %f \n",
              rank, i, j, ((1.0 + rank) + scaling * (1.0 + ((2*rank)%nranks)) * (ITERATIONS)),
              *(buffer[rank] + i + j*XDIM));
          errors++;
          fflush(stdout);
        }
      }
    }
    ARMCI_Access_end(buffer[rank]);

    ARMCI_Free((void *) buffer[rank]);
    ARMCI_Free_local(src_buf);

    ARMCI_Finalize();
    MPI_Finalize();

    if (errors == 0) {
      printf("%d: Success\n", rank);
      return 0;
    } else {
      printf("%d: Fail\n", rank);
      return 1;
    }
}
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <mpi.h>
#include <armci.h>

#define MAX_DATA_SIZE   (1024*1024)
#define NUM_ITERATIONS  ((data_size <= 2048) ? 4096 : ((data_size <= 16384) ? 512 : 128))
#define NUM_WARMUP_ITER 1 

int main(int argc, char ** argv) {
  int    rank, nproc, test_iter, target_rank, data_size;
  int   *my_data, *buf;
  void **base_ptrs;

  MPI_Init(&argc, &argv);
  ARMCI_Init();

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  if (rank == 0) printf("Starting one-sided contiguous performance test with %d processes\n", nproc);

  buf = ARMCI_Malloc_local(MAX_DATA_SIZE);
  base_ptrs = malloc(sizeof(void*)*nproc);
  ARMCI_Malloc(base_ptrs, MAX_DATA_SIZE);

  memset(buf, rank, MAX_DATA_SIZE);

  if (rank == 0)
    printf("%12s %12s %12s %12s %12s %12s %12s %12s\n", "Trg. Rank", "Xfer Size",
        "Get (usec)", "Put (usec)", "Acc (usec)",
        "Get (MiB/s)", "Put (MiB/s)", "Acc (MiB/s)");

  for (target_rank = 1; rank == 0 && target_rank < nproc; target_rank++) {
    for (data_size = sizeof(int); data_size <= MAX_DATA_SIZE; data_size *= 2) {
      double t_get, t_put, t_acc;

      for (test_iter = 0; test_iter < NUM_ITERATIONS + NUM_WARMUP_ITER; test_iter++) {
        if (test_iter == NUM_WARMUP_ITER)
          t_get = MPI_Wtime();

        ARMCI_Get(base_ptrs[target_rank], buf, data_size, target_rank);
      }
      t_get = (MPI_Wtime() - t_get)/NUM_ITERATIONS;

      for (test_iter = 0; test_iter < NUM_ITERATIONS + NUM_WARMUP_ITER; test_iter++) {
        if (test_iter == NUM_WARMUP_ITER)
          t_put = MPI_Wtime();

        ARMCI_Put(buf, base_ptrs[target_rank], data_size, target_rank);
      }
      ARMCI_Fence(target_rank);
      t_put = (MPI_Wtime() - t_put)/NUM_ITERATIONS;

      for (test_iter = 0; test_iter < NUM_ITERATIONS + NUM_WARMUP_ITER; test_iter++) {
        int scale = 1;

        if (test_iter == NUM_WARMUP_ITER)
          t_acc = MPI_Wtime();
#ifdef NO_ACC
        int stride = 0;
        ARMCI_AccS(ARMCI_ACC_INT, &scale, buf, &stride, base_ptrs[target_rank], &stride, &data_size, 0, target_rank);
#else
        ARMCI_Acc(ARMCI_ACC_INT, &scale, buf, base_ptrs[target_rank], data_size, target_rank);
#endif
      }
      ARMCI_Fence(target_rank);
      t_acc = (MPI_Wtime() - t_acc)/NUM_ITERATIONS;

      printf("%12d %12d %12.3f %12.3f %12.3f %12.3f %12.3f %12.3f\n", target_rank, data_size,
          t_get*1.0e6, t_put*1.0e6, t_acc*1.0e6, data_size/(1024.0*1024.0)/t_get, data_size/(1024.0*1024.0)/t_put, data_size/(1024.0*1024.0)/t_acc);
    }
  }

  ARMCI_Barrier();

  ARMCI_Free(base_ptrs[rank]);
  ARMCI_Free_local(buf);
  free(base_ptrs);

  ARMCI_Finalize();
  MPI_Finalize();

  return 0;
}

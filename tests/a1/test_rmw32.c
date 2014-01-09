#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>

#include <mpi.h>
#include "a1.h"

void test_rmw(int mytask, int origin, int ntasks)
{
  int outbuf[2] = { mytask, origin };
  int target = (mytask+1)%ntasks;

  MPI_Barrier(MPI_COMM_WORLD);
  if (mytask == origin) {
    for (int i = 0; i < ntasks-1; i++) {
      A1_Rmw(target, 
	     &outbuf[0],
	     &outbuf[1],
	     &outbuf[0],
	     A1_SWAP,
	     A1_INT32);
      target++;
      outbuf[0] = outbuf[1];
      //printf ("%d: current swap %d\n", i, outbuf[0]);
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);

  printf("%d: My new task id %d\n", mytask, outbuf[0]);
  assert( outbuf[0] == ((mytask+ntasks-1)%ntasks) );

  return;
}

int main (int argc, char ** argv)
{
    int my_task, num_tasks, origin_task = 0;

    MPI_Init(&argc, &argv);
    A1_Initialize();

    MPI_Comm_rank(MPI_COMM_WORLD, &my_task);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);

    test_rmw(my_task, origin_task, num_tasks);

    A1_Finalize();
    MPI_Finalize();

    return 0;
}

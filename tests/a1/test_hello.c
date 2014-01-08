#include <stdio.h>

#include <mpi.h>
#include "a1.h"

int main (int argc, char ** argv)
{
    int mpi_rank, mpi_size, a1_rank, a1_size;

    MPI_Init(&argc, &argv);
    A1_Initialize();

    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    a1_rank = A1_Rank();    
    a1_size = A1_Size();

    printf("MPI: %d of %d A1: %d of %d \n", mpi_rank, mpi_size, a1_rank, a1_size);

    A1_Finalize();
    MPI_Finalize();

    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <armci.h>

int main(int argc, char **argv)
{
    int rank, nranks;
    int provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);

    ARMCI_Init_args(&argc, &argv);

    ARMCI_Barrier();
 
    int me = armci_msg_me();
    int node = armci_domain_my_id(ARMCI_DOMAIN_SMP);

    printf("MPI_Rank: %d, \
            armci_msg_nproc: %d \
            armci_msg_me: %d, \
            armci_domain_id: %d, \
            armci_domain_same_id: %d, \
            armci_domain_my_id: %d, \
            armci_domain_count: %d, \
            armci_domain_nprocs: %d, \
            armci_domain_glob_proc_id: %d \n",
              rank, armci_msg_nproc(), me, armci_domain_id(ARMCI_DOMAIN_SMP, me),
              armci_domain_same_id(ARMCI_DOMAIN_SMP, me), armci_domain_my_id(ARMCI_DOMAIN_SMP),
              armci_domain_count(ARMCI_DOMAIN_SMP), armci_domain_nprocs(ARMCI_DOMAIN_SMP, node),
              armci_domain_glob_proc_id(ARMCI_DOMAIN_SMP, node, 0));
    fflush(stdout);

    ARMCI_Finalize();

    MPI_Finalize();

    return 0;
}

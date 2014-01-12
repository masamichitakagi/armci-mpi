#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <armci.h>

#define MAX_DIM 1024
#define ITERATIONS 100
#define SKIP 10

int main(int argc, char *argv[]) {

   int rank, nranks;
   int dim;
   long bufsize;
   double **buffer;
   double t_start, t_stop;
   int count[2], src_stride, trg_stride, stride_level, peer;
   double expected, actual;
   int provided;

   MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &nranks);

   ARMCI_Init_args(&argc, &argv);
   
   bufsize = MAX_DIM * MAX_DIM * sizeof(double);
   buffer = (double **) malloc(sizeof(double *) * nranks);
   ARMCI_Malloc((void **) buffer, bufsize);

   for(int i=0; i< bufsize/sizeof(double); i++) {
       *(buffer[rank] + i) = 1.0 + rank;
   }

   if(rank == 0) {
     printf("ARMCI_PutS Latency - local and remote completions - in usec \n");
     printf("%s %s %s\n", "Dimensions(array of doubles)", "Latency-LocalCompeltion", "Latency-RemoteCompletion");
     fflush(stdout);
   }

   src_stride = MAX_DIM*sizeof(double);
   trg_stride = MAX_DIM*sizeof(double);
   stride_level = 1;

   ARMCI_Barrier();

   for(dim=1; dim<=MAX_DIM; dim*=2) {

      count[0] = dim*sizeof(double);
      count[1] = dim;

        if(rank == 0) 
        {
          peer = 1;          
 
          for(int i=0; i<ITERATIONS+SKIP; i++) { 

             if(i == SKIP)
                 t_start = MPI_Wtime();

             ARMCI_PutS((void *) buffer[rank], &src_stride, (void *) buffer[peer], &trg_stride, count, stride_level, peer); 
 
          }
          t_stop = MPI_Wtime();
          ARMCI_Fence(peer);
          char temp[10]; 
          sprintf(temp,"%dX%d", dim, dim);
          printf("%30s %20.2f", temp, ((t_stop-t_start)*1000000)/ITERATIONS);
          fflush(stdout);

          ARMCI_Barrier();

          ARMCI_Barrier();

          for(int i=0; i<ITERATIONS+SKIP; i++) {
  
             if(i == SKIP)
                t_start = MPI_Wtime();

             ARMCI_PutS((void *) buffer[rank], &src_stride, (void *) buffer[peer], &trg_stride, count, stride_level, peer); 
             ARMCI_Fence(peer);

          }
          t_stop = MPI_Wtime();
          printf("%20.2f \n", ((t_stop-t_start)*1000000)/ITERATIONS);
          fflush(stdout);

          ARMCI_Barrier();

          ARMCI_Barrier();
        }
        else
        {
            peer = 0;

            expected = (1.0 + (double) peer);

            ARMCI_Barrier();

            for(int i=0; i<dim; i++) {
               for(int j=0; j<dim; j++)
               {
                   actual = *(buffer[rank] + i*MAX_DIM + j);
                   if(actual != expected)
                   {
                      printf("Data validation failed at X: %d Y: %d Expected : %f Actual : %f \n",
                              i, j, expected, actual);
                      fflush(stdout);
                      return -1;
                    }
                }
            }

            for(int i=0; i< bufsize/sizeof(double); i++) {
                *(buffer[rank] + i) = 1.0 + rank;
            }

            ARMCI_Barrier();

            ARMCI_Barrier();

            for(int i=0; i<dim; i++) {
               for(int j=0; j<dim; j++) {
                   actual = *(buffer[rank] + i*MAX_DIM + j);
                   if(actual != expected)
                   {
                      printf("Data validation failed at X: %d Y: %d Expected : %f Actual : %f \n",
                              i, j, expected, actual);
                      fflush(stdout);
                      return -1;
                    }
                }
            }

            for(int i=0; i< bufsize/sizeof(double); i++) {
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

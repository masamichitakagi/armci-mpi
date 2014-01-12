#ifndef A1_TYPES_H
#define A1_TYPES_H

#include <mpi.h>

/* ARMCII error/assert macros */
#include "debug.h"

typedef enum
{
  A1_INT32,  /* int32_t  */
  A1_INT64,  /* int64_t  */
  A1_UINT32, /* uint32_t */
  A1_UINT64, /* uint64_t */
  A1_FLOAT,  /* float    */
  A1_DOUBLE, /* double   */
} A1_datatype_t;

typedef enum
{
  A1_FETCH_AND_ADD,
  A1_SWAP,
} A1_atomic_op_t;

/* This is PAMI-specific.  Fix (generalize) later. */
typedef struct
{
    int32_t active;
} A1_handle_t;

static inline void types_mpi_to_a1(MPI_Datatype mpi, A1_datatype_t * a1)
{
    switch(mpi)
    {
        case MPI_DOUBLE:
            *a1 = A1_DOUBLE;
            break;
        case MPI_FLOAT:
            *a1 = A1_FLOAT;
            break;
        case MPI_INT32_T:
            *a1 = A1_INT32;
            break;
        case MPI_UINT32_T:
            *a1 = A1_UINT32;
            break;
        case MPI_INT64_T:
            *a1 = A1_INT64;
            break;
        case MPI_UINT64_T:
            *a1 = A1_UINT64;
            break;
        default:
            ARMCII_Error("INVALID TYPE (%d)",mpi);
            break;
    }
    return;
}

#endif // A1_TYPES_H

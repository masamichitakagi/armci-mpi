#ifndef A1_API_H
#define A1_API_H

#include "a1-types.h"

/*
 * Initialize the A1 Library
 */
int A1_Initialize(void);

/*
 * Cleanup the A1 Library
 */
int A1_Finalize(void);

/*
 * \brief Blocking RMW on arbitrary data.
 *
 * \param[out] rc                The error code.
 * \param[in]  target            Rank of the target process.
 * \param[in]  source_in         Pointer of modification data at source process.
 * \param[out] source_out        Pointer of original read data at source process.
 * \param[in]  target_ptr        Pointer of modified data at target process.
 * \param[in]  count             Number of data elements to update.
 * \param[in]  op                Operation to be performed.
 * \param[in]  type              Type of data and value.
 *
 * \ingroup DATA_TRANSFER
 */

int A1_Rmw(int                target,
           void *             source_in,
           void *             source_out,
           void *             target_ptr,
           size_t             count,
           A1_atomic_op_t     op,
           A1_datatype_t      type);

#endif // A1_API_H

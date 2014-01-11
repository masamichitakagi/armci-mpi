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
 * \brief Get total process count
 *
 * \param[out] size        Process count.
 *
 * \ingroup UTIL
 */
int A1_Size(void);

/*
 * \brief Get process id
 *
 * \param[out] rank        Process id.
 *
 * \ingroup UTIL
 */
int A1_Rank(void);

/*
 * \brief Cause remote completion of all comm to target
 *
 * \param[out] rc           The error code.
 * \param[in] target        Process id.
 *
 * \ingroup UTIL
 */
int A1_Flush(int target);

/*
 * \brief Blocking RMW on arbitrary data.
 *
 * \param[out] rc                The error code.
 * \param[in]  target            Rank of the target process.
 * \param[in]  source_in         Pointer of modification data at source process.
 * \param[out] source_out        Pointer of original read data at source process.
 * \param[in]  target_ptr        Pointer of modified data at target process.
 * \param[in]  op                Operation to be performed.
 * \param[in]  type              Type of data and value.
 *
 * \ingroup DATA_TRANSFER
 */

int A1_Rmw(int                target,
           void *             source_in,
           void *             source_out,
           void *             target_ptr,
           A1_atomic_op_t     op,
           A1_datatype_t      type);

/*
 * \brief Blocking copy of contiguous data from remote memory to local memory.
 *
 * \param[out] rc       The error code.
 * \param[in]  target   Rank of the remote process.
 * \param[in]  remote   Starting address in the remote memory.
 * \param[in]  local    Starting address in the local memory.
 * \param[in]  bytes    Amount of data to transfer in bytes.
 *
 * \see A1_Iget, A1_Put
 *
 * \ingroup DATA_TRANSFER
 */

int A1_Get(int    target,
           void * remote,
           void * local,
           size_t bytes);

/*
 * \brief Blocking copy of contiguous data from local memory to remote memory.
 *
 * \param[out] rc       The error code.
 * \param[in]  local    Starting address in the local memory.
 * \param[in]  bytes    Amount of data to transfer in bytes.
 * \param[in]  target   Rank of the remote process.
 * \param[in]  remote   Starting address in the remote memory.
 *
 * \see A1_Iput, A1_Get
 *
 * \ingroup DATA_TRANSFER
 */

int A1_Put(void * local,
           size_t bytes,
           int    target,
           void * remote);

/*
 * \brief Nonblocking copy of contiguous data from remote memory to local memory.
 *
 * \param[out] rc       The error code.
 * \param[in]  target   Rank of the remote process.
 * \param[in]  remote   Starting address in the remote memory.
 * \param[in]  local    Starting address in the local memory.
 * \param[in]  bytes    Amount of data to transfer in bytes.
 * \param[out] handle   Opaque handle for the request
 *
 * \see A1_Get, A1_Iput
 *
 * \ingroup DATA_TRANSFER
 */

int A1_Iget(int           target,
            void *        remote,
            void *        local,
            size_t        bytes,
            A1_handle_t * handle);

/*
 * \brief Blocking copy of contiguous data from local memory to remote memory.
 *
 * \param[out] rc       The error code.
 * \param[in]  local    Starting address in the local memory.
 * \param[in]  bytes    Amount of data to transfer in bytes.
 * \param[in]  target   Rank of the remote process.
 * \param[in]  remote   Starting address in the remote memory.
 * \param[out] handle   Opaque handle for the request
 *
 * \see A1_Put, A1_Iget
 *
 * \ingroup DATA_TRANSFER
 */

int A1_Iput(void *        local,
            size_t        bytes,
            int           target,
            void *        remote,
            A1_handle_t * handle);

/*
 * \brief Blocking accumulate (y+=x) of contiguous data from local memory to remote memory.
 *
 * \param[out] rc       The error code.
 * \param[in]  local    Starting address in the local memory.
 * \param[in]  count    Amount of data to transfer in elements.
 * \param[in]  type     Datatype of elements.
 * \param[in]  target   Rank of the remote process.
 * \param[in]  remote   Starting address in the remote memory.
 *
 * \see A1_Iacc, A1_Put
 *
 * \ingroup DATA_TRANSFER
 */

int A1_Acc(void *        local,
           size_t        count,
           A1_datatype_t type,
           int           target,
           void *        remote);

/*
 * \brief Nonblocking accumulate (y+=x) of contiguous data from local memory to remote memory.
 *
 * \param[out] rc       The error code.
 * \param[in]  local    Starting address in the local memory.
 * \param[in]  count    Amount of data to transfer in elements.
 * \param[in]  type     Datatype of elements.
 * \param[in]  target   Rank of the remote process.
 * \param[in]  remote   Starting address in the remote memory.
 * \param[out] handle   Opaque handle for the request
 *
 * \see A1_Acc, A1_Put
 *
 * \ingroup DATA_TRANSFER
 */

int A1_Iacc(void *        local,
            size_t        count,
            A1_datatype_t type,
            int           target,
            void *        remote,
            A1_handle_t * handle);

#endif // A1_API_H

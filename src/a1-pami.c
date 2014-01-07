#include <assert.h>

#include <pami.h>

#include "a1.h"

pami_client_t

/*
 * Initialize the A1 Library
 */
int A1_Initialize(void)
{

    return 0;
}

/*
 * Cleanup the A1 Library
 */
int A1_Finalize(void)
{

    return 0;
}

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
           A1_atomic_op_t     a1op,
           A1_datatype_t      a1type)
{
    pami_result_t rc;
    pami_type_t type;
    pami_atomic_t op;        

    switch (a1type)
    {
        A1_INT32:  type = PAMI_TYPE_SIGNED_INT;
        A1_INT64:  type = PAMI_TYPE_SIGNED_LONG;
        A1_UINT32: type = PAMI_TYPE_UNSIGNED_INT;
        A1_UINT64: type = PAMI_TYPE_UNSIGNED_LONG;
        default: assert(0);
    }

    switch (a1op)
    {
        A1_FETCH_AND_ADD: op = PAMI_ATOMIC_FETCH_ADD;
        A1_SWAP:          op = PAMI_ATOMIC_FETCH_SET;
        default: assert(0);
    }

    pami_endpoint_t ep;
    rc = PAMI_Endpoint_create(a1client, (pami_task_t)target, remote_context_offset, &ep); 
    assert(rc==PAMI_SUCCESS);

    pami_rmw_t m;
    memset(&m, 0, sizeof(pami_rmw_t));

    int active = 1;
    rmw.cookie  = (void*)&active;
    rmw.done_fn = cb_done;

    rmw.value     = source_in;
    rmw.local     = source_out;
    rmw.remote    = target_ptr;
    rmw.type      = type;
    rmw.operation = op;
    rmw.dest      = ep;
  
    pami_context_t lcontext = 0;

    rc = PAMI_Rmw(a1contexts[local_context_offset], &rmw);
    assert(rc==PAMI_SUCCESS);

    while (active)
      PAMI_Context_trylock_advancev(&(a1contexts[local_context_offset]), 1, 100);

    return 0;
}

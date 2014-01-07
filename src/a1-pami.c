#include <mpi.h>

#include <pami.h>

#include "a1-pami.h"

/* for mbar */
#include <hwi/include/bqc/A2_inlines.h>

#define NUM_CONTEXTS 2
int local_context_offset  = 0;
int remote_context_offset = 1;

pami_client_t a1client;
pami_context_t a1contexts[NUM_CONTEXTS];

pthread_t Progress_thread;
volatile int progress_active;

static void * Progress_function(void * input)
{
    pami_result_t result = PAMI_ERROR;

    while (progress_active)
    {
        result = PAMI_Context_trylock_advancev(&(a1contexts[remote_context_offset]), 1, 10000000);
        TEST_ASSERT(result == PAMI_SUCCESS || result == PAMI_EAGAIN,"PAMI_Context_trylock_advancev");
        usleep(1);
    }

    return NULL;
}

/*
 * Initialize the A1 Library
 */
int A1_Initialize(void)
{
    int status = -1;
    pami_result_t result = PAMI_ERROR;

    char * clientname = "ARMCI";
    pami_client_t client;
    result = PAMI_Client_create(clientname, &client, NULL, 0);
    A1_ASSERT(result == PAMI_SUCCESS,"PAMI_Client_create");

    pami_configuration_t config[3];
    config[0].name = PAMI_CLIENT_NUM_TASKS;
    config[1].name = PAMI_CLIENT_TASK_ID;
    config[2].name = PAMI_CLIENT_NUM_CONTEXTS;
    result = PAMI_Client_query(client, config, 3);
    A1_ASSERT(result == PAMI_SUCCESS,"PAMI_Client_query");

    const size_t world_size = config[0].value.intval;
    const size_t world_rank = config[1].value.intval;
    const size_t ncontexts  = config[2].value.intval;
    A1_ASSERT(ncontexts >= NUM_CONTEXTS,"available a1contexts >= NUM_CONTEXTS");

    a1contexts = (pami_context_t *) malloc( NUM_CONTEXTS * sizeof(pami_context_t) );
    A1_ASSERT(a1contexts != NULL,"A1 a1contexts malloc");

    result = PAMI_Context_createv( a1client, NULL, 0, a1contexts, NUM_CONTEXTS );
    A1_ASSERT(result == PAMI_SUCCESS,"PAMI_Context_createv");

    pami_geometry_t world_geometry;
    result = PAMI_Geometry_world(a1client, &world_geometry );
    A1_ASSERT(result == PAMI_SUCCESS,"PAMI_Geometry_world");

    progress_active = 1;
    mbar();

    status = pthread_create(&Progress_thread, NULL, &Progress_function, NULL);
    A1_ASSERT(status==0, "pthread_create");

    MPI_Barrier(MPI_COMM_WORLD);

    return 0;
}

/*
 * Cleanup the A1 Library
 */
int A1_Finalize(void)
{
    int status = -1;
    pami_result_t result = PAMI_ERROR;

    MPI_Barrier(MPI_COMM_WORLD);

    status = pthread_cancel(Progress_thread);
    A1_ASSERT(status==0, "pthread_cancel");

    void * rv;
    status = pthread_join(Progress_thread, &rv);
    A1_ASSERT(status==0, "pthread_join");

    result = PAMI_Context_destroyv( a1contexts, NUM_CONTEXTS );
    A1_ASSERT(result == PAMI_SUCCESS,"PAMI_Context_destroyv");

    free(a1contexts);

    result = PAMI_Client_destroy( &a1client );
    A1_ASSERT(result == PAMI_SUCCESS,"PAMI_Client_destroy");

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
    pami_result_t rc = PAMI_ERROR;

    pami_type_t type;
    pami_atomic_t op;        

    switch (a1type)
    {
        A1_INT32:  type = PAMI_TYPE_SIGNED_INT;
        A1_INT64:  type = PAMI_TYPE_SIGNED_LONG;
        A1_UINT32: type = PAMI_TYPE_UNSIGNED_INT;
        A1_UINT64: type = PAMI_TYPE_UNSIGNED_LONG;
        default:   A1_ASSERT(0,"INVALID TYPE");
    }

    switch (a1op)
    {
        A1_FETCH_AND_ADD: op = PAMI_ATOMIC_FETCH_ADD;
        A1_SWAP:          op = PAMI_ATOMIC_FETCH_SET;
        default:   A1_ASSERT(0,"INVALID OP");
    }

    pami_endpoint_t ep;
    rc = PAMI_Endpoint_create(a1client, (pami_task_t)target, remote_context_offset, &ep); 
    A1_ASSERT(result == PAMI_SUCCESS,"PAMI_Endpoint_create");

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
    A1_ASSERT(result == PAMI_SUCCESS,"PAMI_Rmw");

    while (active)
      PAMI_Context_trylock_advancev(&(a1contexts[local_context_offset]), 1, 100);

    return 0;
}

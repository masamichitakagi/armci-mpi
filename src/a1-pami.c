#include <mpi.h>

#include <pami.h>

#include "a1-pami.h"

/* for mbar */
#include <hwi/include/bqc/A2_inlines.h>

int a1_initialized = 0;

int world_size = 0;
int world_rank = -1;

pami_client_t a1client;
pami_context_t * a1contexts = NULL;

pthread_t Progress_thread;
volatile int progress_active = 0;

static void * Progress_function(void * input)
{
    pami_result_t rc = PAMI_ERROR;

    rc = PAMI_Context_lock(a1contexts[remote_context_offset]);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Context_lock");

    while (progress_active)
    {
        rc = PAMI_Context_advance(a1contexts[remote_context_offset], 1000000);
        A1_ASSERT(rc == PAMI_SUCCESS || rc == PAMI_EAGAIN,"PAMI_Context_advance (remote)");
        usleep(1);
    }

    rc = PAMI_Context_unlock(a1contexts[remote_context_offset]);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Context_unlock");

    pthread_exit((void*)&progress_active);

    return NULL;
}

/*
 * Initialize the A1 Library
 */
int A1_Initialize(void)
{
    if (!a1_initialized) 
    {
        int status = -1;
        pami_result_t result = PAMI_ERROR;

        char * clientname = "ARMCI";
        result = PAMI_Client_create(clientname, &a1client, NULL, 0);
        A1_ASSERT(result == PAMI_SUCCESS,"PAMI_Client_create");

        pami_configuration_t config[3];
        config[0].name = PAMI_CLIENT_NUM_TASKS;
        config[1].name = PAMI_CLIENT_TASK_ID;
        config[2].name = PAMI_CLIENT_NUM_CONTEXTS;
        result = PAMI_Client_query(a1client, config, 3);
        A1_ASSERT(result == PAMI_SUCCESS,"PAMI_Client_query");

        int ncontexts = -1;
        world_size = config[0].value.intval;
        world_rank = config[1].value.intval;
        ncontexts  = config[2].value.intval;
        A1_ASSERT(ncontexts >= NUM_CONTEXTS,"available a1contexts >= NUM_CONTEXTS");

        a1contexts = (pami_context_t *) malloc( NUM_CONTEXTS * sizeof(pami_context_t) );
        A1_ASSERT(a1contexts != NULL,"A1 a1contexts malloc");

        result = PAMI_Context_createv(a1client, NULL, 0, a1contexts, NUM_CONTEXTS );
        A1_ASSERT(result == PAMI_SUCCESS,"PAMI_Context_createv");

        pami_geometry_t world_geometry;
        result = PAMI_Geometry_world(a1client, &world_geometry );
        A1_ASSERT(result == PAMI_SUCCESS,"PAMI_Geometry_world");

        progress_active = 1;
        mbar();

        status = pthread_create(&Progress_thread, NULL, &Progress_function, NULL);
        A1_ASSERT(status==0, "pthread_create");

        MPI_Barrier(MPI_COMM_WORLD);

        a1_initialized = 1;
    }
    else
        A1_ASSERT(0, "A1_Initialize called more than once");

    return 0;
}

/*
 * Cleanup the A1 Library
 */
int A1_Finalize(void)
{
    if (a1_initialized) 
    {
        int status = -1;
        pami_result_t result = PAMI_ERROR;

        MPI_Barrier(MPI_COMM_WORLD);

    #if 0
        status = pthread_cancel(Progress_thread);
        A1_ASSERT(status==0, "pthread_cancel");
    #else
        progress_active = 0;
        mbar();
    #endif

        void * rv;
        status = pthread_join(Progress_thread, &rv);
        A1_ASSERT(status==0, "pthread_join");

        result = PAMI_Context_destroyv( a1contexts, NUM_CONTEXTS );
        A1_ASSERT(result == PAMI_SUCCESS,"PAMI_Context_destroyv");

        free(a1contexts);

        result = PAMI_Client_destroy( &a1client );
        A1_ASSERT(result == PAMI_SUCCESS,"PAMI_Client_destroy");

        a1_initialized = 0;
    }
    else
        A1_ASSERT(0, "A1_Finalize called when A1 not initialized");

    return 0;
}

/*
 * \brief Get total process count
 *
 * \param[out] size        Process count.
 *
 * \ingroup UTIL
 */
int A1_Size(void)
{
    return world_size;
}

/*
 * \brief Get process id
 *
 * \param[out] rank        Process id.
 *
 * \ingroup UTIL
 */
int A1_Rank(void)
{
    return world_rank;
}

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
           A1_atomic_op_t     a1op,
           A1_datatype_t      a1type)
{
    pami_result_t rc = PAMI_ERROR;

    pami_type_t type;
    pami_atomic_t op;

    switch (a1type)
    {
        case A1_INT32:  type = PAMI_TYPE_SIGNED_INT;         break;
        case A1_INT64:  type = PAMI_TYPE_SIGNED_LONG_LONG;   break;
        case A1_UINT32: type = PAMI_TYPE_UNSIGNED_INT;       break;
        case A1_UINT64: type = PAMI_TYPE_UNSIGNED_LONG_LONG; break;
        default:
          A1_ASSERT(0,"A1_Rmw: INVALID TYPE");
          printf("A1_Rmw: INVALID TYPE \n");
          MPI_Abort(MPI_COMM_WORLD, 1);
          break;
    }

    switch (a1op)
    {
        case A1_FETCH_AND_ADD: op = PAMI_ATOMIC_FETCH_ADD; break;
        case A1_SWAP:          op = PAMI_ATOMIC_FETCH_SET; break;
        default:
          A1_ASSERT(0,"A1_Rmw: INVALID TYPE");
          printf("A1_Rmw: INVALID TYPE \n");
          MPI_Abort(MPI_COMM_WORLD, 2);
          break;
    }

    pami_endpoint_t ep;
    rc = PAMI_Endpoint_create(a1client, (pami_task_t)target, remote_context_offset, &ep);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Endpoint_create");

    pami_rmw_t rmw;
    memset(&rmw, 0, sizeof(pami_rmw_t));

    volatile int active = 1;

    rmw.dest      = ep;
    rmw.cookie    = (void*)&active;
    rmw.done_fn   = cb_done;
    rmw.value     = source_in;
    rmw.test      = source_in; /* unused */
    rmw.remote    = target_ptr;
    rmw.local     = source_out;
    rmw.type      = type;
    rmw.operation = op;

    rc = PAMI_Context_lock(a1contexts[local_context_offset]);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Context_lock");

    rc = PAMI_Rmw(a1contexts[local_context_offset], &rmw);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Rmw");

    int attempts = 0;
    while (active) {
      rc = PAMI_Context_advance(a1contexts[local_context_offset], 1000);
      A1_ASSERT(rc == PAMI_SUCCESS || rc == PAMI_EAGAIN,"PAMI_Context_advance (local)");
      attempts++;
      if (attempts > 1000000)
        MPI_Abort(MPI_COMM_WORLD, 3);
    }

    rc = PAMI_Context_unlock(a1contexts[local_context_offset]);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Context_unlock");

    if (attempts>1000)
      fprintf(stderr, "PAMI_Rmw required %d calls to PAMI_Context_advance(..,1000)\n", attempts);

    return 0;
}

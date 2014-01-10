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

/************ ACCUMULATE STUFF ************/

#define ACCUMULATE_DISPATCH_ID 12

typedef struct
{
    /* address of remote data to update */
    void * addr;
#if 0
    /* scaling factor; currently unused */
    union scaling { int32_t i; int64_t l; float f; double d; }
#endif
    A1_datatype_t type;
} 
pami_acc_header_t;

static void accumulate_done_cb(pami_context_t context, void * cookie, pami_result_t result)
{
  return;
}

static void accumulate_recv_cb(pami_context_t context,
                               void * cookie,
                               const void * header_addr, size_t header_size,
                               const void * pipe_addr,   
                               size_t data_size, /* applies to both pipe and stream */
                               pami_endpoint_t origin,
                               pami_recv_t * recv)
{
  pami_acc_header_t * h = (pami_acc_header_t*)header_addr;

  A1_datatype_t  a1type = h->type;

  if (pipe_addr!=NULL) {
    switch(a1type) {
      case A1_DOUBLE:
        A1_ACC_MACRO(double, pipe_addr, h->addr, data_size);
        break;
      case A1_FLOAT:
        A1_ACC_MACRO(float, pipe_addr, h->addr, data_size);
        break;
      case A1_INT32:
        A1_ACC_MACRO(int32_t, pipe_addr, h->addr, data_size);
        break;
      case A1_INT64:
        A1_ACC_MACRO(int64_t, pipe_addr, h->addr, data_size);
        break;
      case A1_UINT32:
        A1_ACC_MACRO(uint32_t, pipe_addr, h->addr, data_size);
        break;
      case A1_UINT64:
        A1_ACC_MACRO(uint64_t, pipe_addr, h->addr, data_size);
        break;
      default:
        A1_ASSERT(0,"accumulate_recv_cb: INVALID TYPE");
        printf("accumulate_recv_cb: INVALID TYPE \n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        break;
    }
#if 0
    /* TODO macro and switch-case this ala A1-DCMF */
    size_t count = data_size/sizeof(double);
    double * target_data = h->addr;
    const double * pipe_data = (const double *) pipe_addr;
    for (size_t i=0; i<count; i++)
      target_data[i] += pipe_data[i];
#endif
  }
  else /* stream */ {
    
    pami_type_t pt;
    int sz;

    types_a1_to_pami(a1type, &pt, &sz);

    recv->cookie      = 0;
    recv->local_fn    = NULL; /* accumulate_done_cb would go here */
    recv->addr        = h->addr;
    recv->type        = pt;
    recv->offset      = 0;
    recv->data_fn     = PAMI_DATA_SUM;
    recv->data_cookie = NULL;
  }

  return;
}

/****************************************/

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

        /* register the dispatch function */

        pami_dispatch_callback_function pami_acc_dispatch_fn = { .p2p = accumulate_recv_cb };
        pami_dispatch_hint_t pami_acc_dispatch_hint          = { .recv_immediate = PAMI_HINT_DISABLE };
        size_t pami_acc_dispatch_id                          = ACCUMULATE_DISPATCH_ID;
        int pami_acc_dispatch_cookie                         = world_rank; /* what is the point of this? */

        /* The dispatch has to be registered with the local context otherwise PAMI_Send fails. */
        result = PAMI_Dispatch_set(a1contexts[local_context_offset], 
                                   pami_acc_dispatch_id, 
                                   pami_acc_dispatch_fn, 
                                   &pami_acc_dispatch_cookie, 
                                   pami_acc_dispatch_hint);
        A1_ASSERT(result == PAMI_SUCCESS,"PAMI_Dispatch_set");

        result = PAMI_Dispatch_set(a1contexts[remote_context_offset], 
                                   pami_acc_dispatch_id, 
                                   pami_acc_dispatch_fn, 
                                   &pami_acc_dispatch_cookie, 
                                   pami_acc_dispatch_hint);
        A1_ASSERT(result == PAMI_SUCCESS,"PAMI_Dispatch_set");
  
        /* startup the (stupid) progress engine */

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

    int active = 1;

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

/*
 * \brief Blocking copy of contiguous data from remote memory to local memory.
 *
 * \param[out] rc       The error code.
 * \param[in]  target   Rank of the remote process.
 * \param[in]  remote   Starting address in the remote memory.
 * \param[in]  local    Starting address in the local memory.
 * \param[in]  bytes    Amount of data to transfer in bytes.
 *
 * \see A1_NbGet, A1_Put
 *
 * \ingroup DATA_TRANSFER
 */

int A1_Get(int    target,
           void * remote,
           void * local,
           size_t bytes)
{
    pami_result_t rc = PAMI_ERROR;

    pami_endpoint_t ep;
    rc = PAMI_Endpoint_create(a1client, (pami_task_t)target, remote_context_offset, &ep);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Endpoint_create");

    pami_get_simple_t get;
    memset(&get, 0, sizeof(pami_get_simple_t));

    int active = 1;

    get.rma.dest      = ep;
    get.rma.bytes     = bytes;
    get.rma.cookie    = (void*)&active;
    get.rma.done_fn   = cb_done;
    get.addr.local    = local;
    get.addr.remote   = remote;

    rc = PAMI_Context_lock(a1contexts[local_context_offset]);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Context_lock");

    rc = PAMI_Get(a1contexts[local_context_offset], &get);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Get");

    int attempts = 0;
    while (active) {
      rc = PAMI_Context_advance(a1contexts[local_context_offset], 1);
      A1_ASSERT(rc == PAMI_SUCCESS || rc == PAMI_EAGAIN,"PAMI_Context_advance (local)");
      attempts++;
    }

    rc = PAMI_Context_unlock(a1contexts[local_context_offset]);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Context_unlock");

    return 0;
}

/*
 * \brief Blocking copy of contiguous data from local memory to remote memory.
 *
 * \param[out] rc       The error code.
 * \param[in]  local    Starting address in the local memory.
 * \param[in]  bytes    Amount of data to transfer in bytes.
 * \param[in]  target   Rank of the remote process.
 * \param[in]  remote   Starting address in the remote memory.
 *
 * \see A1_NbPut, A1_Get
 *
 * \ingroup DATA_TRANSFER
 */

int A1_Put(void * local,
           size_t bytes,
           int    target,
           void * remote)
{
    pami_result_t rc = PAMI_ERROR;

    pami_endpoint_t ep;
    rc = PAMI_Endpoint_create(a1client, (pami_task_t)target, remote_context_offset, &ep);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Endpoint_create");

    pami_put_simple_t put;
    memset(&put, 0, sizeof(pami_put_simple_t));

    int active = 2;

    put.rma.dest      = ep;
    put.rma.bytes     = bytes;
    put.rma.cookie    = (void*)&active;
    put.rma.done_fn   = cb_done;
    put.put.rdone_fn  = cb_done;
    put.addr.local    = local;
    put.addr.remote   = remote;

    rc = PAMI_Context_lock(a1contexts[local_context_offset]);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Context_lock");

    rc = PAMI_Put(a1contexts[local_context_offset], &put);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Put");

    int attempts = 0;
    while (active) {
      rc = PAMI_Context_advance(a1contexts[local_context_offset], 1);
      A1_ASSERT(rc == PAMI_SUCCESS || rc == PAMI_EAGAIN,"PAMI_Context_advance (local)");
      attempts++;
    }

    rc = PAMI_Context_unlock(a1contexts[local_context_offset]);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Context_unlock");

    return 0;
}

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
           A1_datatype_t a1type,
           int           target,
           void *        remote)
{
    pami_result_t rc = PAMI_ERROR;

    pami_endpoint_t ep;
    rc = PAMI_Endpoint_create(a1client, (pami_task_t)target, remote_context_offset, &ep);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Endpoint_create");

    pami_type_t pamitype;
    int typesize;
    types_a1_to_pami(a1type, &pamitype, &typesize);

    pami_acc_header_t acc_header = { .addr = remote , .type = a1type };

    pami_send_t acc;
    memset(&acc, 0, sizeof(pami_send_t));

    int active = 2;

    acc.send.header.iov_base = (void*)&acc_header;
    acc.send.header.iov_len  = sizeof(pami_acc_header_t);
    acc.send.data.iov_base   = local;
    acc.send.data.iov_len    = count*typesize;
    acc.send.dispatch        = ACCUMULATE_DISPATCH_ID;
    acc.send.dest            = ep;
    acc.send.hints.use_shmem = PAMI_HINT_DISABLE;
    acc.events.cookie        = &active;
    acc.events.local_fn      = cb_done;
    acc.events.remote_fn     = cb_done;

    rc = PAMI_Context_lock(a1contexts[local_context_offset]);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Context_lock");

    rc = PAMI_Send(a1contexts[local_context_offset], &acc);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Send");

    int attempts = 0;
    while (active) {
      rc = PAMI_Context_advance(a1contexts[local_context_offset], 1);
      A1_ASSERT(rc == PAMI_SUCCESS || rc == PAMI_EAGAIN,"PAMI_Context_advance (local)");
      attempts++;
    }

    rc = PAMI_Context_unlock(a1contexts[local_context_offset]);
    A1_ASSERT(rc == PAMI_SUCCESS,"PAMI_Context_unlock");

    return 0;
}

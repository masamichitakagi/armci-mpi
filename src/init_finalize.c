/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#define _GNU_SOURCE             /* for async progress */
#include <sched.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include <armci.h>
#include <armci_internals.h>
#include <debug.h>
#include <gmr.h>

#ifdef HAVE_PTHREADS
#include <pthread.h>

#if defined(HAVE_NANOSLEEP)

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#include <time.h> /* nanosleep */

#elif defined(HAVE_USLEEP)

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#include <unistd.h> /* usleep */

#else

#warning No naptime available!

#endif

#include <unistd.h> /* for syscall() */
#ifdef WITH_UTI
#include <uti.h> /* for async progress */
#endif

int progress_active;
pthread_t ARMCI_Progress_thread;

static pthread_t thr;
static pthread_mutex_t mutex;
static pthread_cond_t cond;
static pthread_barrier_t bar;
static volatile int flag;

static MPI_Comm progress_comm;
static int progress_refc;
#define WAKE_TAG 100

static void * progress_function(void * arg)
{
#if 1
	int rc;
	MPI_Request req;
	int completed;

	rc = pthread_barrier_wait(&bar);
	if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
		printf("%s: ERROR: pthread_barrier_wait (%d)\n", __FUNCTION__, rc);
	}

	if ((rc = MPI_Irecv(NULL, 0, MPI_CHAR, 0, WAKE_TAG, progress_comm, &req)) != MPI_SUCCESS) {
		printf("%s: ERROR: MPI_Irecv failed (%d)\n", __FUNCTION__, rc);
	}
#if 0
	completed = 0;
	while (!completed) {
		if ((rc = MPI_Test(&req, &completed, MPI_STATUS_IGNORE)) != MPI_SUCCESS) {
			printf("%s: ERROR: MPI_Test failed (%d)\n", __FUNCTION__, rc);
			break;
		}
		//sched_yield();
	}
#else
	if ((rc = MPI_Wait(&req, MPI_STATUS_IGNORE)) != MPI_SUCCESS) {
		printf("%s: ERROR: MPI_Wait failed (%d)\n", __FUNCTION__, rc);
	}
#endif

#if 1
	pthread_mutex_lock(&mutex);
	flag = 1;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
#endif
	//printf("%s: exiting\n", __FUNCTION__);
#else
    volatile int * active = (volatile int*)arg;
#if defined(HAVE_NANOSLEEP)
    int naptime = 1000 * ARMCII_GLOBAL_STATE.progress_usleep;
    struct timespec napstruct = { .tv_sec  = 0,
                                  .tv_nsec = naptime };
#elif defined(HAVE_USLEEP)
    int naptime = ARMCII_GLOBAL_STATE.progress_usleep;
#endif

    while(*active) {
        ARMCIX_Progress();
#if defined(HAVE_NANOSLEEP)
        if (naptime) nanosleep(&napstruct,NULL);
#elif defined(HAVE_USLEEP)
        if (naptime) usleep(naptime);
#endif
    }

    pthread_exit(NULL);

    return NULL;
#endif
}
#endif

/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_Init = PARMCI_Init
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_Init ARMCI_Init
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_Init as PARMCI_Init
#endif
/* -- end weak symbols block -- */

/** Initialize ARMCI.  MPI must be initialized before this can be called.  It
  * invalid to make ARMCI calls before initialization.  Collective on the world
  * group.
  *
  * @return            Zero on success
  */
int PARMCI_Init(void) {
  char *var;

  /* GA/TCGMSG end up calling ARMCI_Init() multiple times. */
  if (ARMCII_GLOBAL_STATE.init_count > 0) {
    ARMCII_GLOBAL_STATE.init_count++;
    return 0;
  }

  /* Check for MPI initialization */
  {
    int mpi_is_init, mpi_is_fin;
    MPI_Initialized(&mpi_is_init);
    MPI_Finalized(&mpi_is_fin);
    if (!mpi_is_init || mpi_is_fin) 
      ARMCII_Error("MPI must be initialized before calling ARMCI_Init");
  }

#ifdef HAVE_PTHREADS
  /* Check progress thread settings */
  {
    int mpi_thread_level;
    MPI_Query_thread(&mpi_thread_level);

    ARMCII_GLOBAL_STATE.progress_thread    = ARMCII_Getenv_bool("ARMCI_PROGRESS_THREAD", 0);
    ARMCII_GLOBAL_STATE.progress_usleep    = ARMCII_Getenv_int("ARMCI_PROGRESS_USLEEP", 0);

    if (ARMCII_GLOBAL_STATE.progress_thread && (mpi_thread_level!=MPI_THREAD_MULTIPLE)) {
        ARMCII_Warning("ARMCI progress thread requires MPI_THREAD_MULTIPLE (%d); progress thread disabled.\n",
                       mpi_thread_level);
        ARMCII_GLOBAL_STATE.progress_thread = 0;
    }

    if (ARMCII_GLOBAL_STATE.progress_thread && (ARMCII_GLOBAL_STATE.progress_usleep < 0)) {
        ARMCII_Warning("ARMCI progress thread is not a time machine. (%d)\n",
                       ARMCII_GLOBAL_STATE.progress_usleep);
        ARMCII_GLOBAL_STATE.progress_usleep = -ARMCII_GLOBAL_STATE.progress_usleep;
    }
  }
#endif

  /* Set defaults */
#ifdef ARMCI_GROUP
  ARMCII_GLOBAL_STATE.noncollective_groups = 1;
#endif
#ifdef NO_SEATBELTS
  ARMCII_GLOBAL_STATE.iov_checks           = 0;
#endif

  /* Check for debugging flags */

  ARMCII_GLOBAL_STATE.debug_alloc          = ARMCII_Getenv_bool("ARMCI_DEBUG_ALLOC", 0);
  {
	int junk;
	junk = ARMCII_Getenv_bool("ARMCI_FLUSH_BARRIERS", -1);
    if (junk != -1)
      ARMCII_Warning("ARMCI_FLUSH_BARRIERS is deprecated.  Use ARMCI_SYNC_AT_BARRIERS instead. \n");
  }
  ARMCII_GLOBAL_STATE.verbose              = ARMCII_Getenv_bool("ARMCI_VERBOSE", 0);

  /* Group formation options */

  ARMCII_GLOBAL_STATE.cache_rank_translation=ARMCII_Getenv_bool("ARMCI_CACHE_RANK_TRANSLATION", 1);
  if (ARMCII_Getenv("ARMCI_NONCOLLECTIVE_GROUPS"))
    ARMCII_GLOBAL_STATE.noncollective_groups = ARMCII_Getenv_bool("ARMCI_NONCOLLECTIVE_GROUPS", 0);

  /* Check for IOV flags */

  ARMCII_GLOBAL_STATE.iov_checks           = ARMCII_Getenv_bool("ARMCI_IOV_CHECKS", 0);
  ARMCII_GLOBAL_STATE.iov_batched_limit    = ARMCII_Getenv_int("ARMCI_IOV_BATCHED_LIMIT", 0);

  if (ARMCII_GLOBAL_STATE.iov_batched_limit < 0) {
    ARMCII_Warning("Ignoring invalid value for ARMCI_IOV_BATCHED_LIMIT (%d)\n", ARMCII_GLOBAL_STATE.iov_batched_limit);
    ARMCII_GLOBAL_STATE.iov_batched_limit = 0;
  }

#if defined(OPEN_MPI)
  ARMCII_GLOBAL_STATE.iov_method = ARMCII_IOV_BATCHED;
#else
  /* DIRECT leads to addr=NULL errors when ARMCI_{GetV,PutV} are used
   * Jeff: Is this still true? */
  ARMCII_GLOBAL_STATE.iov_method = ARMCII_IOV_DIRECT;
#endif

  var = ARMCII_Getenv("ARMCI_IOV_METHOD");
  if (var != NULL) {
    if (strcmp(var, "AUTO") == 0)
      ARMCII_GLOBAL_STATE.iov_method = ARMCII_IOV_AUTO;
    else if (strcmp(var, "CONSRV") == 0)
      ARMCII_GLOBAL_STATE.iov_method = ARMCII_IOV_CONSRV;
    else if (strcmp(var, "BATCHED") == 0)
      ARMCII_GLOBAL_STATE.iov_method = ARMCII_IOV_BATCHED;
    else if (strcmp(var, "DIRECT") == 0)
      ARMCII_GLOBAL_STATE.iov_method = ARMCII_IOV_DIRECT;
    else if (ARMCI_GROUP_WORLD.rank == 0)
      ARMCII_Warning("Ignoring unknown value for ARMCI_IOV_METHOD (%s)\n", var);
  }

  /* Check for Strided flags */


#if defined(OPEN_MPI)
  ARMCII_GLOBAL_STATE.strided_method = ARMCII_STRIDED_IOV;
#else
  ARMCII_GLOBAL_STATE.strided_method = ARMCII_STRIDED_DIRECT;
#endif

  var = ARMCII_Getenv("ARMCI_STRIDED_METHOD");
  if (var != NULL) {
    if (strcmp(var, "IOV") == 0)
      ARMCII_GLOBAL_STATE.strided_method = ARMCII_STRIDED_IOV;
    else if (strcmp(var, "DIRECT") == 0)
      ARMCII_GLOBAL_STATE.strided_method = ARMCII_STRIDED_DIRECT;
    else if (ARMCI_GROUP_WORLD.rank == 0)
      ARMCII_Warning("Ignoring unknown value for ARMCI_STRIDED_METHOD (%s)\n", var);
  }

#ifdef OPEN_MPI
  if (ARMCII_GLOBAL_STATE.iov_method == ARMCII_IOV_DIRECT ||
      ARMCII_GLOBAL_STATE.strided_method == ARMCII_STRIDED_DIRECT)
      ARMCII_Warning("MPI Datatypes are broken in RMA in OpenMPI!!!!\n");
#endif

  /* Shared buffer handling method */

  /* The default used to be COPY.  NOGUARD requires MPI_WIN_UNIFIED. */
  ARMCII_GLOBAL_STATE.shr_buf_method = ARMCII_SHR_BUF_NOGUARD;

  var = ARMCII_Getenv("ARMCI_SHR_BUF_METHOD");
  if (var != NULL) {
    if (strcmp(var, "COPY") == 0)
      ARMCII_GLOBAL_STATE.shr_buf_method = ARMCII_SHR_BUF_COPY;
    else if (strcmp(var, "NOGUARD") == 0)
      ARMCII_GLOBAL_STATE.shr_buf_method = ARMCII_SHR_BUF_NOGUARD;
    else if (ARMCI_GROUP_WORLD.rank == 0)
      ARMCII_Warning("Ignoring unknown value for ARMCI_SHR_BUF_METHOD (%s)\n", var);
  }

  /* Use win_allocate or not, to work around MPI-3 RMA implementation bugs (now fixed) in MPICH. */

  int win_alloc_default = 1;
  ARMCII_GLOBAL_STATE.use_win_allocate=ARMCII_Getenv_bool("ARMCI_USE_WIN_ALLOCATE", win_alloc_default);

  /* Poke the MPI progress engine at the end of nonblocking (NB) calls */

  ARMCII_GLOBAL_STATE.explicit_nb_progress=ARMCII_Getenv_bool("ARMCI_EXPLICIT_NB_PROGRESS", 1);

  /* Pass alloc_shm to win_allocate / alloc_mem */

  ARMCII_GLOBAL_STATE.use_alloc_shm=ARMCII_Getenv_bool("ARMCI_USE_ALLOC_SHM", 1);

  /* Enable RMA element-wise atomicity */

  ARMCII_GLOBAL_STATE.rma_atomicity=ARMCII_Getenv_bool("ARMCI_RMA_ATOMICITY", 1);

  /* Flush_local becomes flush */

  ARMCII_GLOBAL_STATE.end_to_end_flush=ARMCII_Getenv_bool("ARMCI_NO_FLUSH_LOCAL", 0);

  /* Use MPI_MODE_NOCHECK assertion */

  ARMCII_GLOBAL_STATE.rma_nocheck=ARMCII_Getenv_bool("ARMCI_RMA_NOCHECK", 1);

  /* Setup groups and communicators */

  MPI_Comm_dup(MPI_COMM_WORLD, &ARMCI_GROUP_WORLD.comm);
  ARMCII_Group_init_from_comm(&ARMCI_GROUP_WORLD);
  ARMCI_GROUP_DEFAULT = ARMCI_GROUP_WORLD;

  /* Create GOP operators */

  MPI_Op_create(ARMCII_Absmin_op, 1 /* commute */, &MPI_ABSMIN_OP);
  MPI_Op_create(ARMCII_Absmax_op, 1 /* commute */, &MPI_ABSMAX_OP);

  MPI_Op_create(ARMCII_Msg_sel_min_op, 1 /* commute */, &MPI_SELMIN_OP);
  MPI_Op_create(ARMCII_Msg_sel_max_op, 1 /* commute */, &MPI_SELMAX_OP);

  ARMCII_GLOBAL_STATE.init_count++;

  if (ARMCII_GLOBAL_STATE.verbose) {
    if (ARMCI_GROUP_WORLD.rank == 0) {
      int major, minor;

      MPI_Get_version(&major, &minor);

      printf("ARMCI-MPI initialized with %d process%s, MPI v%d.%d\n", ARMCI_GROUP_WORLD.size, ARMCI_GROUP_WORLD.size > 1 ? "es":"", major, minor);
#ifdef NO_SEATBELTS
      printf("  NO_SEATBELTS           = ENABLED\n");
#endif

#ifdef HAVE_PTHREADS
      printf("  PROGRESS_THREAD        = %s\n", ARMCII_GLOBAL_STATE.progress_thread ? "ENABLED" : "DISABLED");
      if (ARMCII_GLOBAL_STATE.progress_thread) {
          printf("  PROGRESS_USLEEP        = %d\n", ARMCII_GLOBAL_STATE.progress_usleep);
      }
#endif

      printf("  ALLOC_SHM used         = %s\n", ARMCII_GLOBAL_STATE.use_alloc_shm ? "TRUE" : "FALSE");
      printf("  WINDOW type used       = %s\n", ARMCII_GLOBAL_STATE.use_win_allocate ? "ALLOCATE" : "CREATE");
      if (ARMCII_GLOBAL_STATE.use_win_allocate) {
          /* Jeff: Using win_allocate leads to correctness issues with some
           *       MPI implementations since 3c4ad2abc8c387fcdec3a7f3f44fa5fd75653ece. */
          /* This is required on Cray systems with CrayMPI 7.0.0 (at least) */
          /* Update (Feb. 2015): Xin and Min found the bug in Fetch_and_op and 
           *                     it is fixed upstream. */
          ARMCII_Warning("MPI_Win_allocate can lead to correctness issues.\n");
      }

      printf("  STRIDED_METHOD         = %s\n", ARMCII_Strided_methods_str[ARMCII_GLOBAL_STATE.strided_method]);
      printf("  IOV_METHOD             = %s\n", ARMCII_Iov_methods_str[ARMCII_GLOBAL_STATE.iov_method]);

      if (   ARMCII_GLOBAL_STATE.iov_method == ARMCII_IOV_BATCHED
          || ARMCII_GLOBAL_STATE.iov_method == ARMCII_IOV_AUTO)
      {
        if (ARMCII_GLOBAL_STATE.iov_batched_limit > 0)
          printf("  IOV_BATCHED_LIMIT      = %d\n", ARMCII_GLOBAL_STATE.iov_batched_limit);
        else
          printf("  IOV_BATCHED_LIMIT      = UNLIMITED\n");
      }

      printf("  IOV_CHECKS             = %s\n", ARMCII_GLOBAL_STATE.iov_checks             ? "TRUE" : "FALSE");
      printf("  SHR_BUF_METHOD         = %s\n", ARMCII_Shr_buf_methods_str[ARMCII_GLOBAL_STATE.shr_buf_method]);
      printf("  NONCOLLECTIVE_GROUPS   = %s\n", ARMCII_GLOBAL_STATE.noncollective_groups   ? "TRUE" : "FALSE");
      printf("  CACHE_RANK_TRANSLATION = %s\n", ARMCII_GLOBAL_STATE.cache_rank_translation ? "TRUE" : "FALSE");
      printf("  DEBUG_ALLOC            = %s\n", ARMCII_GLOBAL_STATE.debug_alloc            ? "TRUE" : "FALSE");
      printf("\n");
      fflush(NULL);
    }

    MPI_Barrier(ARMCI_GROUP_WORLD.comm);
  }

#ifdef HAVE_PTHREADS
    /* Create the asynchronous progress thread */
    {
        if(ARMCII_GLOBAL_STATE.progress_thread) {
            progress_active = 1;
            int rc = pthread_create(&ARMCI_Progress_thread, NULL, &progress_function, &progress_active);
            if (rc) {
                ARMCII_Warning("ARMCI progress thread creation failed (%d).\n", rc);
            }
        }
    }
#endif

  return 0;
}

void armci_init_async_thread_() {
#if 1
    int mpi_thread_level;
	int rc;
	char *my_async_progress_str;
	pthread_attr_t attr;
	cpu_set_t cpuset;
	char *async_progress_pin_str;
	int progress_cpus[1024];
	int n_progress_cpus = 0;
	char *list, *token;
	char *rank_str;
	int rank;
	char *mck_str;

    MPI_Query_thread(&mpi_thread_level);
    if (mpi_thread_level != MPI_THREAD_MULTIPLE) {
        ARMCII_Warning("ARMCI progress thread requires MPI_THREAD_MULTIPLE (%d); progress thread creation failed.\n",
                       mpi_thread_level);
		return;
	}

	my_async_progress_str = getenv("MY_ASYNC_PROGRESS");
	if (!my_async_progress_str) {
		return;
	}
	if (atoi(my_async_progress_str) == 0) {
		return;
	}

	if (__sync_fetch_and_add(&progress_refc, 1) > 0) {
		return;
	}

	if ((rc = MPI_Comm_dup(MPI_COMM_SELF, &progress_comm))) {
		printf("%s: ERROR: MPI_Comm_dup failed (%d)\n", __FUNCTION__, rc);
		goto sub_out;
	}

	if ((rc = pthread_attr_init(&attr))) {
 		printf("%s: ERROR: pthread_attr_init failed (%d)\n", __FUNCTION__, rc);
		goto sub_out;
	}

	async_progress_pin_str = getenv("MY_ASYNC_PROGRESS_PIN");
	if (!async_progress_pin_str) {
 		printf("%s: ERROR: MY_ASYNC_PROGRESS_PIN not found\n", __FUNCTION__);
		goto sub_out;
	}

	list = async_progress_pin_str;
	while (1) {
		token = strsep(&list, ",");
		if (!token) {
			break;
		}
		progress_cpus[n_progress_cpus++] = atoi(token);
	}

	rank_str = getenv("PMI_RANK");
	if (!rank_str) {
 		printf("%s: ERROR: PMI_RANK not found\n", __FUNCTION__);
		goto sub_out;
	}
	rank = atoi(rank_str);

	CPU_ZERO(&cpuset);
	CPU_SET(progress_cpus[rank % n_progress_cpus], &cpuset);

	//printf("%s: rank=%d,n_progress_cpus=%d,progress_cpu=%d\n", __FUNCTION__, rank, n_progress_cpus, progress_cpus[rank % n_progress_cpus]);

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);

	if ((rc = pthread_barrier_init(&bar, NULL, 2))) {
		printf("%s: ERROR: pthread_barrier_init (%d)\n", __FUNCTION__, rc);
		goto sub_out;
	}

#ifdef WITH_UTI
	uti_attr_t uti_attr;
	rc = uti_attr_init(&uti_attr);
	if (rc) {
		printf("%s: ERROR: uti_attr_init failed (%d)\n", __FUNCTION__, rc);
		goto sub_out;
	}
#if 0
	/* Give a hint that it's beneficial to put the thread
	 * on the same NUMA-node as the creator */
	rc = UTI_ATTR_SAME_NUMA_DOMAIN(&uti_attr);
	if (rc) {
		printf("%s: ERROR: UTI_ATTR_SAME_NUMA_DOMAIN failed (%d)\n", __FUNCTION__, rc);
		goto uti_destroy_and_out;
	}
	
	/* Give a hint that the thread repeatedly monitors a device
	 * using CPU. */
	rc = UTI_ATTR_CPU_INTENSIVE(&uti_attr);
	if (rc) {
		printf("%s: ERROR: UTI_ATTR_CPU_INTENSIVE failed (%d)\n", __FUNCTION__, rc);
		goto uti_destroy_and_out;
	}
	
	/* Give a hint that it's beneficial to prioritize it in scheduling. */
	rc = UTI_ATTR_HIGH_PRIORITY(&uti_attr);
	if (rc) {
		printf("%s: ERROR: UTI_ATTR_HIGH_PRIORITY failed (%d)\n", __FUNCTION__, rc);
		goto uti_destroy_and_out;
	}
#endif
	
	if ((rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))) {
		printf("%s: ERROR: pthread_attr_setdetachstate failed (%d)\n", __FUNCTION__, rc);
		goto uti_destroy_and_out;
	}
	
	rc = uti_pthread_create(&thr, &attr, progress_function, NULL, &uti_attr);
	if (rc) {
		goto uti_destroy_and_out;
	}
	
	rc = uti_attr_destroy(&uti_attr);
	if (rc) {
		printf("%s: ERROR: uti_attr_destroy failed (%d)\n", __FUNCTION__, rc);
		goto sub_out;
	}
#else
	mck_str = getenv("MY_ASYNC_PROGRESS_MCK");
	if (mck_str) {
		if ((rc = syscall(731, 1, NULL))) {
			printf("%s: ERROR: util_indicate_clone failed (%d)\n", __FUNCTION__, rc);
			goto sub_out;
		}
	} else {
		if ((rc = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset))) {
			printf("%s: ERROR: pthread_attr_setaffinity_np failed (%d)\n", __FUNCTION__, rc);
			goto sub_out;
		}

	}

	if ((rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))) {
		printf("%s: ERROR: pthread_attr_setdetachstate failed (%d)\n", __FUNCTION__, rc);
		goto sub_out;
	}
	
	if ((rc = pthread_create(&thr, &attr, progress_function, NULL))) {
		printf("%s: ERROR: pthread_create failed (%d)\n", __FUNCTION__, rc);
		goto sub_out;
	}
#endif

	rc = pthread_barrier_wait(&bar);
	if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
		printf("%s: ERROR: pthread_barrier_wait (%d)\n", __FUNCTION__, rc);
		goto sub_out;
	}

 fn_exit:
	return;

#ifdef WITH_UTI
 uti_destroy_and_out:
	rc = uti_attr_destroy(&uti_attr);
	if (rc) {
		printf("%s: ERROR: uti_attr_destroy failed (%d)\n", __FUNCTION__, rc);
		goto sub_out;
	}
#endif

 sub_out:
	__sync_fetch_and_sub(&progress_refc, 1);
	goto fn_exit;
#else
    int mpi_thread_level;

    MPI_Query_thread(&mpi_thread_level);
    if (mpi_thread_level != MPI_THREAD_MULTIPLE) {
        ARMCII_Warning("ARMCI progress thread requires MPI_THREAD_MULTIPLE (%d); progress thread creation failed.\n",
                       mpi_thread_level);
		return;
	}

	progress_active = 1;
	int rc = pthread_create(&ARMCI_Progress_thread, NULL, &progress_function, &progress_active);
	if (rc) {
		ARMCII_Warning("ARMCI progress thread creation failed (%d).\n", rc);
	}
#endif
}

void armci_finalize_async_thread_() {
#if 1
	int rc;
	char *my_async_progress_str;
	MPI_Request req;

	my_async_progress_str = getenv("MY_ASYNC_PROGRESS");
	if (!my_async_progress_str) {
		return;
	}
	if (atoi(my_async_progress_str) == 0) {
		return;
	}

	if (__sync_sub_and_fetch(&progress_refc, 1) != 0) {
		return;
	}

	if ((rc = MPI_Isend(NULL, 0, MPI_CHAR, 0, WAKE_TAG, progress_comm, &req)) != MPI_SUCCESS) {
		printf("%s: ERROR: MPI_Send failed (%d)\n", __FUNCTION__, rc);
		return;
	}

	if ((rc = MPI_Wait(&req, MPI_STATUS_IGNORE)) != MPI_SUCCESS) {
		printf("%s: ERROR: MPI_Wait failed (%d)\n", __FUNCTION__, rc);
		return;
	}

#if 1
	//printf("%s: before cond_wait\n", __FUNCTION__);

	pthread_mutex_lock(&mutex);
	while(!flag) {
		pthread_cond_wait(&cond, &mutex);
	}
	flag = 0;
	pthread_mutex_unlock(&mutex);
	//printf("%s: after cond_wait\n", __FUNCTION__);
#else
	pthread_join(thr, NULL);
#endif

	if ((rc = MPI_Comm_free(&progress_comm)) != MPI_SUCCESS) {
		printf("%s: ERROR: MPI_Comm_free failed (%d)\n", __FUNCTION__, rc);
		return;
	}
#else
	progress_active = 0;
	int rc = pthread_join(ARMCI_Progress_thread, NULL);
	if (rc) {
		ARMCII_Warning("ARMCI progress thread join failed (%d).\n", rc);
	}
#endif
}

/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_Init_args = PARMCI_Init_args
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_Init_args ARMCI_Init_args
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_Init_args as PARMCI_Init_args
#endif
/* -- end weak symbols block -- */

/** Initialize ARMCI.  MPI must be initialized before this can be called.  It
  * is invalid to make ARMCI calls before initialization.  Collective on the
  * world group.
  *
  * @param[inout] argc Command line argument count
  * @param[inout] argv Command line arguments
  * @return            Zero on success
  */
int PARMCI_Init_args(int *argc, char ***argv) {
  return PARMCI_Init();
}


/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_Initialized = PARMCI_Initialized
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_Initialized ARMCI_Initialized
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_Initialized as PARMCI_Initialized
#endif
/* -- end weak symbols block -- */

/** Check if ARMCI has been initialized.
  *
  * @return Non-zero if ARMCI has been initialized.
  */
int PARMCI_Initialized(void) {
  return ARMCII_GLOBAL_STATE.init_count > 0;
}


/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_Finalize = PARMCI_Finalize
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_Finalize ARMCI_Finalize
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_Finalize as PARMCI_Finalize
#endif
/* -- end weak symbols block -- */

/** Finalize ARMCI.  Must be called before MPI is finalized.  ARMCI calls are
  * not valid after finalization.  Collective on world group.
  *
  * @return            Zero on success
  */
int PARMCI_Finalize(void) {
  int nfreed;

  /* GA/TCGMSG end up calling ARMCI_Finalize() multiple times. */
  if (ARMCII_GLOBAL_STATE.init_count == 0) {
    return 0;
  }

  ARMCII_GLOBAL_STATE.init_count--;

  /* Only finalize on the last matching call */
  if (ARMCII_GLOBAL_STATE.init_count > 0) {
    return 0;
  }

#ifdef HAVE_PTHREADS
    /* Destroy the asynchronous progress thread */
    {
        if(ARMCII_GLOBAL_STATE.progress_thread) {
            progress_active = 0;
            int rc = pthread_join(ARMCI_Progress_thread, NULL);
            if (rc) {
                ARMCII_Warning("ARMCI progress thread join failed (%d).\n", rc);
            }
        }
    }
#endif

  nfreed = gmr_destroy_all();

  if (nfreed > 0 && ARMCI_GROUP_WORLD.rank == 0)
    ARMCII_Warning("Freed %d leaked allocations\n", nfreed);

  /* Free GOP operators */

  MPI_Op_free(&MPI_ABSMIN_OP);
  MPI_Op_free(&MPI_ABSMAX_OP);

  MPI_Op_free(&MPI_SELMIN_OP);
  MPI_Op_free(&MPI_SELMAX_OP);

  ARMCI_Cleanup();

  ARMCI_Group_free(&ARMCI_GROUP_WORLD);

  return 0;
}


/** Cleaup ARMCI resources.  Call finalize instead.
  */
void ARMCI_Cleanup(void) {
  return;
}


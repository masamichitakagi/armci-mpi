/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#ifndef HAVE_GMR_H
#define HAVE_GMR_H

#include <mpi.h>

#include <armci.h>
#include <armcix.h>

typedef armci_size_t gmr_size_t;

enum gmr_lock_states_e { 
  GMR_LOCK_UNLOCKED,    /* Mem region is unlocked */
  GMR_LOCK_EXCLUSIVE,   /* Mem region is locked for exclusive access */
  GMR_LOCK_SHARED,      /* Mem region is locked for shared (non-conflicting) access */
#if defined(RMA_SUPPORTS_LOCK_ALL) || defined(RMA_SUPPORTS_FLUSH)
  GMR_LOCK_ALL,         /* Mem region is locked for shared access for all targets using MPI_WIN_LOCK_ALL */
#endif
  GMR_LOCK_DLA,         /* Mem region is locked for Direct Local Access */
  GMR_LOCK_DLA_SUSP     /* Mem region is unlocked and DLA is suspended */
};

typedef struct {
  void       *base;
  gmr_size_t  size;
} gmr_slice_t;

typedef struct gmr_s {
  MPI_Win                 window;         /* MPI Window for this GMR                                        */
  ARMCI_Group             group;          /* Copy of the ARMCI group on which this GMR was allocated        */

  int                     access_mode;    /* Current access mode                                            */
  enum gmr_lock_states_e  lock_state;     /* State of the lock                                              */
  int                     lock_target;    /* Group (window) rank of the current target (if locked).
                                             If LOCK_ALL is used, this value is undefined.                  */
  int                     dla_lock_count; /* Access count on the DLA lock.  Can unlock when this reaches 0. */
  armcix_mutex_hdl_t      rmw_mutex;      /* Mutex used for Read-Modify-Write operations                    */

  struct gmr_s           *prev;           /* Linked list pointers for GMR list                              */
  struct gmr_s           *next;
  gmr_slice_t            *slices;         /* Array of GMR slices for this allocation                        */
  int                     nslices;
} gmr_t;

extern gmr_t *gmr_list;

gmr_t *gmr_create(gmr_size_t local_size, void **base_ptrs, ARMCI_Group *group);
void   gmr_destroy(gmr_t *mreg, ARMCI_Group *group);
int    gmr_destroy_all(void);
gmr_t *gmr_lookup(void *ptr, int proc);

int gmr_get(gmr_t *mreg, void *src, void *dst, int size, int target);
int gmr_put(gmr_t *mreg, void *src, void *dst, int size, int target);
int gmr_accumulate(gmr_t *mreg, void *src, void *dst, int count, MPI_Datatype type, int proc);
#ifdef RMA_SUPPORTS_RMW
int gmr_get_accumulate(gmr_t *mreg, void *src, void *out, void *dst, int count, MPI_Datatype type, MPI_Op op, int proc);
#endif

int gmr_get_typed(gmr_t *mreg, void *src, int src_count, MPI_Datatype src_type,
    void *dst, int dst_count, MPI_Datatype dst_type, int proc);
int gmr_put_typed(gmr_t *mreg, void *src, int src_count, MPI_Datatype src_type,
    void *dst, int dst_count, MPI_Datatype dst_type, int proc);
int gmr_accumulate_typed(gmr_t *mreg, void *src, int src_count, MPI_Datatype src_type,
    void *dst, int dst_count, MPI_Datatype dst_type, int proc);
#ifdef RMA_SUPPORTS_RMW
int gmr_get_accumulate_typed(gmr_t *mreg, void *src, int src_count, MPI_Datatype src_type, 
    void *out, int out_count, MPI_Datatype out_type, void *dst, int dst_count, MPI_Datatype dst_type, MPI_Op op, int proc);
int gmr_fetch_and_op(gmr_t *mreg, void *src, void *out, void *dst, MPI_Datatype type, MPI_Op op, int proc);
#endif

void gmr_lock(gmr_t *mreg, int proc);
void gmr_unlock(gmr_t *mreg, int proc);
#ifdef RMA_SUPPORTS_LOCK_ALL
int  gmr_lockall(gmr_t *mreg);
int  gmr_unlockall(gmr_t *mreg);
#endif
#ifdef RMA_SUPPORTS_FLUSH
int  gmr_flush(gmr_t *mreg, int proc, int local_only);
int  gmr_flushall(gmr_t *mreg);
#endif
#ifdef RMA_SUPPORTS_SYNC
int gmr_sync(gmr_t *mreg);
#endif

void gmr_dla_lock(gmr_t *mreg);
void gmr_dla_unlock(gmr_t *mreg);

#endif /* HAVE_GMR_H */

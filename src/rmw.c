/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>

#include <armci.h>
#include <armci_internals.h>
#include <gmr.h>
#include <debug.h>


/* -- begin weak symbols block -- */
#if defined(HAVE_PRAGMA_WEAK)
#  pragma weak ARMCI_Rmw = PARMCI_Rmw
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#  pragma _HP_SECONDARY_DEF PARMCI_Rmw ARMCI_Rmw
#elif defined(HAVE_PRAGMA_CRI_DUP)
#  pragma _CRI duplicate ARMCI_Rmw as PARMCI_Rmw
#endif
/* -- end weak symbols block -- */

/** Perform atomic read-modify-write on the given integer or long location and
  * return the location's original value.
  *
  * \note ARMCI RMW operations are atomic with respect to other RMW operations,
  * but not with respect to other one-sided operations (get, put, acc, etc).
  *
  * @param[in]  op    Operation to be performed:
  *                     ARMCI_FETCH_AND_ADD (int)
  *                     ARMCI_FETCH_AND_ADD_LONG
  *                     ARMCI_SWAP (int)
  *                     ARMCI_SWAP_LONG
  * @param[out] ploc  Location to store the original value.
  * @param[in]  prem  Location on which to perform atomic operation.
  * @param[in]  value Value to add to remote location (ignored for swap).
  * @param[in]  proc  Process rank for the target buffer.
  */
int PARMCI_Rmw(int op, void *ploc, void *prem, int value, int proc) {
  int           is_long;
  gmr_t *mreg;

  mreg = gmr_lookup(prem, proc);
  ARMCII_Assert_msg(mreg != NULL, "Invalid remote pointer");

  if (op == ARMCI_SWAP_LONG || op == ARMCI_FETCH_AND_ADD_LONG)
    is_long = 1;
  else
    is_long = 0;

  if (op == ARMCI_SWAP || op == ARMCI_SWAP_LONG) {
    long swap_val_l;
    int  swap_val_i;

    ARMCIX_Lock_hdl(mreg->rmw_mutex, 0, proc);
    PARMCI_Get(prem, is_long ? (void*) &swap_val_l : (void*) &swap_val_i, 
              is_long ? sizeof(long) : sizeof(int), proc);
    PARMCI_Put(ploc, prem, is_long ? sizeof(long) : sizeof(int), proc);
    ARMCIX_Unlock_hdl(mreg->rmw_mutex, 0, proc);

    if (is_long)
      *(long*) ploc = swap_val_l;
    else
      *(int*) ploc = swap_val_i;
  }

  else if (op == ARMCI_FETCH_AND_ADD || op == ARMCI_FETCH_AND_ADD_LONG) {
#if (MPI_VERSION < 3)
    long fetch_val_l, new_val_l;
    int  fetch_val_i, new_val_i;
    
    ARMCIX_Lock_hdl(mreg->rmw_mutex, 0, proc);
    PARMCI_Get(prem, is_long ? (void*) &fetch_val_l : (void*) &fetch_val_i,
              is_long ? sizeof(long) : sizeof(int), proc);
    
    if (is_long)
      new_val_l = fetch_val_l + value;
    else
      new_val_i = fetch_val_i + value;

    PARMCI_Put(is_long ? (void*) &new_val_l : (void*) &new_val_i, prem, 
              is_long ? sizeof(long) : sizeof(int), proc);
    ARMCIX_Unlock_hdl(mreg->rmw_mutex, 0, proc);
#else
    long fetch_val_l, add_val_l = value;
    int  fetch_val_i, add_val_i = value;
    
    gmr_t *src_mreg, *dst_mreg;
 
    dst_mreg = gmr_lookup(prem, proc);

    ARMCII_Assert_msg(dst_mreg != NULL, "Invalid remote pointer");
 
    /* If NOGUARD is set, assume the buffer is not shared */
    if (ARMCII_GLOBAL_STATE.shr_buf_method != ARMCII_SHR_BUF_NOGUARD)
      src_mreg = gmr_lookup(ploc, ARMCI_GROUP_WORLD.rank);
    else
      src_mreg = NULL;

    gmr_lock(dst_mreg, proc);
    gmr_fetch_and_op(dst_mreg, 
                     is_long ? (void*) &add_val_l   : (void*) &add_val_i   /* src */, 
                     is_long ? (void*) &fetch_val_l : (void*) &fetch_val_i /* out */, 
                     prem /* dst */, 
                     is_long ? MPI_LONG : MPI_INT, 
                     MPI_SUM, 
                     proc);
    gmr_unlock(dst_mreg, proc);
#endif

    if (is_long)
      *(long*) ploc = fetch_val_l;
    else
      *(int*) ploc = fetch_val_i;
  }

  else {
    ARMCII_Error("invalid operation (%d)", op);
  }

  return 0;
}

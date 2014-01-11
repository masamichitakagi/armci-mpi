#ifndef A1_PAMI_H
#define A1_PAMI_H

#include <unistd.h>
#include <pthread.h>

/* ARMCII error/assert macros */
#include "debug.h"

#include "a1.h"

#define NUM_CONTEXTS 2
const int local_context_offset  = 0;
const int remote_context_offset = 1;

extern pami_client_t a1client;
extern pami_context_t * a1contexts;

static void cb_done(void * ctxt, void * clientdata, pami_result_t err)
{
  int * active = (int *) clientdata;
  (*active)--;
}

static inline void types_a1_to_pami(A1_datatype_t a1, pami_type_t * pt, int * sz)
{
    switch (a1)
    {
        case A1_INT32:
            *pt = PAMI_TYPE_SIGNED_INT;
            *sz = 4;
            break;
        case A1_INT64:
            *pt = PAMI_TYPE_SIGNED_LONG_LONG;
            *sz = 8;
            break;
        case A1_UINT32:
            *pt = PAMI_TYPE_UNSIGNED_INT;
            *sz = 4;
            break;
        case A1_UINT64:
            *pt = PAMI_TYPE_UNSIGNED_LONG_LONG;
            *sz = 8;
            break;
        case A1_FLOAT:
            *pt = PAMI_TYPE_FLOAT;
            *sz = 4;
            break;
        case A1_DOUBLE:
            *pt = PAMI_TYPE_DOUBLE;
            *sz = 8;
            break;
        default:
          ARMCII_Error("INVALID TYPE (%d)",a1);
          break;
    }
    return;
}

static void print_pami_result_text(pami_result_t r)
{
    switch(r)
    {
        case PAMI_SUCCESS     : printf("PAMI result = %16s \n", "PAMI_SUCCESS    "); break;
        case PAMI_NERROR      : printf("PAMI result = %16s \n", "PAMI_NERROR     "); break;
        case PAMI_ERROR       : printf("PAMI result = %16s \n", "PAMI_ERROR      "); break;
        case PAMI_INVAL       : printf("PAMI result = %16s \n", "PAMI_INVAL      "); break;
        case PAMI_UNIMPL      : printf("PAMI result = %16s \n", "PAMI_UNIMPL     "); break;
        case PAMI_EAGAIN      : printf("PAMI result = %16s \n", "PAMI_EAGAIN     "); break;
        case PAMI_ENOMEM      : printf("PAMI result = %16s \n", "PAMI_ENOMEM     "); break;
        case PAMI_SHUTDOWN    : printf("PAMI result = %16s \n", "PAMI_SHUTDOWN   "); break;
        case PAMI_CHECK_ERRNO : printf("PAMI result = %16s \n", "PAMI_CHECK_ERRNO"); break;
        case PAMI_OTHER       : printf("PAMI result = %16s \n", "PAMI_OTHER      "); break;
        case PAMI_RESULT_EXT  : printf("PAMI result = %16s \n", "PAMI_RESULT_EXT "); break;
        default               : printf("PAMI result = %16s \n", "NOT DEFINED     "); break;
    }
    return;
}

#define A1_ACC_MACRO(datatype, source, target, bytes)                       \
   do {                                                                     \
     size_t count = bytes/sizeof(datatype);                                 \
     datatype * s = (datatype *) source;                                    \
     datatype * t = (datatype *) target;                                    \
     for(size_t i=0; i<count; i++)                                          \
          t[i] += s[i];                                                     \
   } while(0)                                                               \

#endif // A1_PAMI_H

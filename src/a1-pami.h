#ifndef A1_PAMI_H
#define A1_PAMI_H

#include <pthread.h>

#include "a1.h"

#define NUM_CONTEXTS 2
const int local_context_offset  = 0;
const int remote_context_offset = 1;

extern pami_client_t a1client;
extern pami_context_t * a1contexts;

static void cb_done(void * ctxt, void * clientdata, pami_result_t err)
{
  printf("cb_done \n");
  fflush(stdout);
  int * active = (int *) clientdata;
  (*active)--;
}

#endif // A1_PAMI_H

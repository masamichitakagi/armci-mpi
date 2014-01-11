#ifndef A1_BGQ_ATOMICS_H
#define A1_BGQ_ATOMICS_H

static inline int64_t Load64(volatile int64_t *pv)
{
    register int64_t v;
    asm volatile ("ldarx  %[rc],0,%[pv];"
                    : [rc] "=&b" (v)
                    : [pv] "b" (pv));
    return(v);
}

static inline int Store64(volatile int64_t * pv, int64_t v)
{
    register int rc = 1;
    asm volatile ("  stdcx.  %2,0,%1;"
                  "  beq     1f;" /* success */
                  "  li      %0,0;"
                  "1:;"
                    : "=b" (rc)
                    : "b"  (pv),
                      "b"  (v),
                      "0"  (rc)
                    : "cc", "memory" );
    return(rc);
}

static inline void Dec64(int64_t *pv)
{
    register int64_t tmp;
    do {
        tmp = Load64(pv);
        tmp--;
    } while ( !Store64(pv,tmp) );
    return;
}

static inline void Inc64(int64_t *pv)
{
    register int64_t tmp;
    do {
        tmp = Load64(pv);
        tmp++;
    } while ( !Store64(pv,tmp) );
    return;
}

static inline int64_t FetchInc64(int64_t *pv)
{
    register int64_t old, tmp;
    do {
        old = Load64(pv);
        tmp = old + 1;
    } while ( !Store64(pv,tmp) );
    return(old);
}

#endif // A1_BGQ_ATOMICS_H

#ifndef A1_TYPES_H
#define A1_TYPES_H

typedef enum
{
  A1_INT32,  /**< int32            */
  A1_INT64,  /**< int64            */
  A1_UINT32, /**< uint32           */
  A1_UINT64, /**< uint64           */
  A1_FLOAT,  /**< single-precision */
  A1_DOUBLE, /**< double-precision */
} A1_datatype_t;

typedef enum
{
  A1_FETCH_AND_ADD,
  A1_SWAP,
} A1_atomic_op_t;

#endif // A1_TYPES_H

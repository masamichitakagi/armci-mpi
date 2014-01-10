#
# Copyright (C) 2010. See COPYRIGHT in top-level directory.
#

check_PROGRAMS += \
                  tests/a1/test_hello                \
                  tests/a1/test_rmw32                \
                  tests/a1/ARMCI_AllFlushAll_latency \
                  tests/a1/ARMCI_Bcast               \
                  tests/a1/ARMCI_FlushAll_latency    \
                  tests/a1/ARMCI_Get_bw              \
                  tests/a1/ARMCI_Get_latency         \
                  tests/a1/ARMCI_Get_latency_SO      \
                  tests/a1/ARMCI_GetS_bw             \
                  tests/a1/ARMCI_GetS_latency        \
                  tests/a1/ARMCI_IGOP_Scalar         \
                  tests/a1/ARMCI_IGOP_Vector         \
                  tests/a1/ARMCI_PutAcc_latency      \
                  tests/a1/ARMCI_PutAccS_bw          \
                  tests/a1/ARMCI_PutAccS_latency     \
                  tests/a1/ARMCI_Put_bw              \
                  tests/a1/ARMCI_Put_latency         \
                  tests/a1/ARMCI_PutS_bw             \
                  tests/a1/ARMCI_PutS_latency        \
                  tests/a1/ARMCI_RMW                 \
                  tests/a1/ARMCI_Test_nodeid         \
                  # end

TESTS          += \
                  tests/a1/test_hello                \
                  tests/a1/test_rmw32                \
                  tests/a1/ARMCI_AllFlushAll_latency \
                  tests/a1/ARMCI_Bcast               \
                  tests/a1/ARMCI_FlushAll_latency    \
                  tests/a1/ARMCI_Get_bw              \
                  tests/a1/ARMCI_Get_latency         \
                  tests/a1/ARMCI_Get_latency_SO      \
                  tests/a1/ARMCI_GetS_bw             \
                  tests/a1/ARMCI_GetS_latency        \
                  tests/a1/ARMCI_IGOP_Scalar         \
                  tests/a1/ARMCI_IGOP_Vector         \
                  tests/a1/ARMCI_PutAcc_latency      \
                  tests/a1/ARMCI_PutAccS_bw          \
                  tests/a1/ARMCI_PutAccS_latency     \
                  tests/a1/ARMCI_Put_bw              \
                  tests/a1/ARMCI_Put_latency         \
                  tests/a1/ARMCI_PutS_bw             \
                  tests/a1/ARMCI_PutS_latency        \
                  tests/a1/ARMCI_RMW                 \
                  tests/a1/ARMCI_Test_nodeid         \
                  # end

tests_a1_test_hello_LDADD = libarmci.la
tests_a1_test_rmw32_LDADD = libarmci.la
tests_a1_ARMCI_AllFlushAll_latency_LDADD = libarmci.la
tests_a1_ARMCI_Bcast_LDADD = libarmci.la
tests_a1_ARMCI_FlushAll_latency_LDADD = libarmci.la
tests_a1_ARMCI_Get_bw_LDADD = libarmci.la
tests_a1_ARMCI_Get_latency_LDADD = libarmci.la
tests_a1_ARMCI_Get_latency_SO_LDADD = libarmci.la
tests_a1_ARMCI_GetS_bw_LDADD = libarmci.la
tests_a1_ARMCI_GetS_latency_LDADD = libarmci.la
tests_a1_ARMCI_IGOP_Scalar_LDADD = libarmci.la
tests_a1_ARMCI_IGOP_Vector_LDADD = libarmci.la
tests_a1_ARMCI_PutAcc_latency_LDADD = libarmci.la
tests_a1_ARMCI_PutAccS_bw_LDADD = libarmci.la
tests_a1_ARMCI_PutAccS_latency_LDADD = libarmci.la
tests_a1_ARMCI_Put_bw_LDADD = libarmci.la
tests_a1_ARMCI_Put_latency_LDADD = libarmci.la
tests_a1_ARMCI_PutS_bw_LDADD = libarmci.la
tests_a1_ARMCI_PutS_latency_LDADD = libarmci.la
tests_a1_ARMCI_RMW_LDADD = libarmci.la
tests_a1_ARMCI_Test_nodeid_LDADD = libarmci.la

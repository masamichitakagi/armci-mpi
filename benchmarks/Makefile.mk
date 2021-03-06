#
# Copyright (C) 2010. See COPYRIGHT in top-level directory.
#

check_PROGRAMS += benchmarks/ping-pong          \
                  benchmarks/ring-flood         \
                  benchmarks/contiguous-bench   \
                  benchmarks/strided-bench      \
                  benchmarks/bench_groups       \
                  benchmarks/rmw_perf           \
                  # end

TESTS          += benchmarks/ping-pong          \
                  benchmarks/ring-flood         \
                  benchmarks/contiguous-bench   \
                  benchmarks/strided-bench      \
                  benchmarks/rmw_perf           \
                  # end

benchmarks_ping_pong_LDADD = libarmci.la
benchmarks_ring_flood_LDADD = libarmci.la
benchmarks_contiguous_bench_LDADD = libarmci.la -lm
benchmarks_strided_bench_LDADD = libarmci.la -lm
benchmarks_bench_groups_LDADD = libarmci.la -lm
benchmarks_rmw_perf_LDADD = libarmci.la

#
# Copyright (C) 2010. See COPYRIGHT in top-level directory.
#

check_PROGRAMS += \
                  tests/a1/test_hello         \
                  tests/a1/test_rmw32         \
                  # end

TESTS          += \
                  tests/a1/test_hello         \
                  tests/a1/test_rmw32         \
                  # end

tests_a1_test_hello_LDADD = libarmci.la
tests_a1_test_rmw32_LDADD = libarmci.la


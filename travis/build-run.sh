#! /bin/sh

# Exit on error
set -ev

MPI_IMPL="$1"

# Environment variables
export CFLAGS="-std=c99"
export MPICC=mpicc

# Configure and build
./autogen.sh
./configure --disable-static

# Run unit tests
export ARMCI_VERBOSE=1
case "$MPI_IMPL" in
make check

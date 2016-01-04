#!/bin/sh
# This configuration file was taken originally from the mpi4py project
# <http://mpi4py.scipy.org/>, and then modified for Julia

set -e
set -x

os=`uname`
MPI_IMPL="$1"

case "$os" in
    Darwin)
        echo "Mac"
        brew update
        case "$MPI_IMPL" in
            mpich|mpich3)
                brew install mpich
                ;;
            *)
                echo "Unknown MPI implementation: $MPI_IMPL"
                exit 10
                ;;
        esac
    ;;

    Linux)
        echo "Linux"
        sudo apt-get update -q
        case "$MPI_IMPL" in
            mpich)
                wget --no-check-certificate -q http://www.mpich.org/static/downloads/3.2/mpich-3.2.tar.gz
                tar -xzf mpich-3.2.tar.gz
                cd mpich-3.2
                mkdir build && cd build
                ../configure CC=$PRK_CC CXX=$PRK_CXX --disable-fortran --prefix=/usr
                make -j4
                sudo make install
                ;;
            *)
                echo "Unknown MPI implementation: $MPI_IMPL"
                exit 20
                ;;
        esac
        ;;
esac

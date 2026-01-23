#!/bin/bash

make || exit 1

mpirun -n 11  ./bin/prefix_sum_sendrecv

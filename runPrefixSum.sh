#!/bin/bash

make || exit 1

mpirun -n 4  ./bin/prefix_sum_sendrecv

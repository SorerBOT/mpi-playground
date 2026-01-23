#!/bin/bash

MAX_THREADS_MY_MACHINE=11
BIN="./bin"
N=4
seed_A=13
seed_B=7
max_value=100

echo "Cleaning up..."
make clean

echo "Compiling..."
make || exit 1

echo "Running $BIN/matmul_mpi..."
mpirun -np $MAX_THREADS_MY_MACHINE "$BIN/matmul_mpi" $N $seed_A $seed_B $max_value

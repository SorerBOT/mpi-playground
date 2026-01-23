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


for N in {1..20}
do
    for seed_A in 1 49 65 13 7 3
    do
        for seed_B in 2 6 43 7 8 3
        do
            echo "Running $BIN/matmul_mpi, with N = $N, seed_A = $seed_A, seed_B = $seed_B..."
            mpirun -np $MAX_THREADS_MY_MACHINE "$BIN/matmul_mpi" $N $seed_A $seed_B $max_value > /dev/null
            RETURN_CODE=$?
            if [ $RETURN_CODE -eq 0 ]; then
                echo "Success!"
            else
                echo "Failure..."
                exit 1
            fi
        done
    done
done

echo "All tests passed successfuly."

BIN=./bin
CC=mpicc

$(BIN)/matmul_mpi: $(BIN)/matmul_mpi.o $(BIN)/matrix.o
	$(CC) $(BIN)/matmul_mpi.o $(BIN)/matrix.o -o $(BIN)/matmul_mpi


$(BIN)/matmul_mpi.o: pre-build
	$(CC) -c matmul_mpi.c -o $(BIN)/matmul_mpi.o

$(BIN)/matrix.o: pre-build
	$(CC) -c matrix.c -o $(BIN)/matrix.o

pre-build:
	mkdir -p $(BIN)

clean:
	rm -rf $(BIN)

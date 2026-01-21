CC=mpicc

prefix_sum_sendrecv: prebuild ./bin/prefix_sum_sendrecv.o
	$(CC) ./bin/prefix_sum_sendrecv.o -o ./bin/prefix_sum_sendrecv $(LDFLAGS)
	rm ./bin/prefix_sum_sendrecv.o

./bin/prefix_sum_sendrecv.o:
	$(CC) -c prefix_sum_sendrecv.c $(CFLAGS) -o ./bin/prefix_sum_sendrecv.o

prebuild:
	mkdir -p ./bin

clean:
	rm -rf ./bin

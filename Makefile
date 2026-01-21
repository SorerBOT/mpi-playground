CC=gcc

main: prebuild ./bin/main.o
	$(CC) ./bin/main.o -o ./bin/main $(LDFLAGS)
	rm ./bin/main.o

./bin/main.o:
	$(CC) -c main.c $(CFLAGS) -o ./bin/main.o

prebuild:
	mkdir -p ./bin

clean:
	rm -rf ./bin

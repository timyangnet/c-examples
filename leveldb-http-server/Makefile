CC_OPTS=-Wall -ggdb3 -lev -lleveldb -I../../libev-4.15 -I../../leveldb-1.12.0/include -L../../libev-4.15/.libs -L../../leveldb-1.12.0

all: bin/leveldb-httpd

clean:
	rm -rf *.o bin/*

bin/leveldb-httpd: main.c
	mkdir -p bin
	$(CC) $(CC_OPTS) -o $@ $<

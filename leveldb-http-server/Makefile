CC_OPTS=-Wall -lev -lleveldb -ggdb3 -I../lib/include -I../leveldb-1.12.0/include -L../lib/lib -L../leveldb-1.12.0

all: bin/leveldb-httpd

clean:
	rm -f *.o bin/*

bin/leveldb-httpd: main.c
	$(CC) $(CC_OPTS) -o $@ $<

leveldb-http-server
==========

http wrapper for leveldb

Install libev (http://software.schmorp.de/pkg/libev.html) and leveled in the same directory of c-examples

<pre>
cd leveldb-1.12.0
./configure
make

cd leveldb-1.12.0
make
</pre>

- set key

curl http://localhost:8002/set?key

- get key

curl http://localhost:8002/get?key


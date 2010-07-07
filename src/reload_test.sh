make clean
rm data.file*
make -j8
#./rethinkdb -c 1 -p 11211
cgdb rethinkdb

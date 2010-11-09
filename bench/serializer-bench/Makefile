serializer-bench: main.cc Makefile
	cd ../../src && make
	g++ main.cc -I ../../src/ -c -o main.o -Wall
	g++ ../stress-client/utils.cc -c -o utils.o -Wall
	g++ main.o utils.o `find ../../build/debug/obj -name "*.o" | grep -v main.o` -o serializer-bench -Wall -rdynamic -lrt -laio

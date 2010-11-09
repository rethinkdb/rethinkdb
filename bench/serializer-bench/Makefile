serializer-bench: main.cc Makefile
	cd ../../src && make DEBUG=0
	g++ main.cc -I ../../src/ -c -o main.o -Wall
	g++ ../stress-client/utils.cc -c -o utils.o -Wall
	g++ main.o utils.o `find ../../build/release/obj -name "*.o" | grep -v main.o` -o serializer-bench -Wall -rdynamic -lrt -laio

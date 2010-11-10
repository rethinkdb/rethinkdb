DEBUG?=0
CXXFLAGS=-Wall -pg -g
LDFLAGS=-Wall -rdynamic -lrt -laio -pg -g
OBJDIR:=../../build/debug/obj

serializer-bench: main.cc Makefile
	cd ../../src && make DEBUG=1 -j8
	g++ main.cc -I ../../src/ -c -o main.o $(CXXFLAGS)
	g++ ../stress-client/utils.cc -c -o utils.o $(CXXFLAGS)
	g++ main.o utils.o `find $(OBJDIR) -name "*.o" | grep -v main.o` -o serializer-bench $(LDFLAGS)

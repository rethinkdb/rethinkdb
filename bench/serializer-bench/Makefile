DEBUG?=0
CXXFLAGS=-Wall -pg -g -DUSE_UCONTEXT
LDFLAGS=-Wall -rdynamic -lrt -laio -pg -g -lgsl -lgslcblas
OBJDIR:=../../build/release/obj

serializer-bench: main.cc Makefile
	cd ../../src && make DEBUG=0 -j8
	g++ main.cc -I ../../src/ -c -o main.o $(CXXFLAGS)
	g++ main.o `find $(OBJDIR) -name "*.o" | grep -v main.o` -o serializer-bench $(LDFLAGS)

clean:
	rm -f *~
	rm -f *.o
	rm -f serializer-bench

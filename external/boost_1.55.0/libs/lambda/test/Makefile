BOOST 		= ../../..
 
CXX		= g++
EXTRAFLAGS 	= -pedantic -Wno-long-long -Wno-long-double -ftemplate-depth-50 
LIBS		= -lstdc++

#CXX		= KCC 
#EXTRAFLAGS 	= --strict --display_error_number --diag_suppress 450 --max_pending_instantiations 50
#LIBS		=

INCLUDES	= -I$(BOOST)



CXXFLAGS	= $(INCLUDES) $(EXTRAFLAGS)

LIBFLAGS	= $(LIBS)


AR		= ar

.SUFFIXES: .cpp .o

SOURCES = \
is_instance_of_test.cpp \
operator_tests_simple.cpp \
member_pointer_test.cpp \
control_structures.cpp \
switch_construct.cpp \
bind_tests_simple.cpp \
bind_tests_advanced.cpp \
bll_and_function.cpp \
constructor_tests.cpp \
extending_rt_traits.cpp \
bind_tests_simple_f_refs.cpp \
cast_test.cpp \
phoenix_control_structures.cpp \
exception_test.cpp \


# Create lists of object files from the source file lists.

OBJECTS = ${SOURCES:.cpp=.o}     

TARGETS = ${SOURCES:.cpp=.exe}     

all: 	$(TARGETS)

%.exe: %.o
	$(CXX) $(LIBFLAGS) $(CXXFLAGS) -o $@ $<

%.o: %.cpp 
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.dep:  %.cpp
	set -e; $(CXX) -M $(INCLUDES) -c $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	[ -s $@ ] || rm -f $@

DEP_FILES = $(SOURCES:.cpp=.dep)

include $(DEP_FILES)

clean:	
	/bin/rm -rf $(TARGETS) $(OBJECTS) $(DEP_FILES)

run:	
	./is_instance_of_test.exe
	./member_pointer_test.exe
	./operator_tests_simple.exe 
	./control_structures.exe 
	./switch_construct.exe 
	./extending_rt_traits.exe 
	./constructor_tests.exe 
	./cast_test.exe 
	./bind_tests_simple.exe 
	./bind_tests_advanced.exe 
	./bll_and_function.exe
	./bind_tests_simple_f_refs.exe
	./phoenix_control_structures.exe
	./exception_test.exe









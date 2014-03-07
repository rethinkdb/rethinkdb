# copyright John Maddock 2003
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt.

# very basic makefile for regression tests
# tests every library combination, static/dynamic/multimthread/singlethread/narrow/wide
#
# Sun Workshop 6 and greater:
#
CXX= CC $(INCLUDES) -I../../../../ -I./ $(CXXFLAGS) -L../../../../stage/lib -L../../build/sunpro $(LDFLAGS)
#
# sources to compile for each test:
#
SOURCES=*.cpp 

total : r rm r/regress rm/regress rs rms rs/regress rms/regress rw rmw rw/regress rmw/regress rsw rmsw rsw/regress rmsw/regress
	echo testsing narrow character versions:
	./r/regress tests.txt
	./rm/regress tests.txt
	./rs/regress tests.txt
	./rms/regress tests.txt
	echo testsing wide character versions;
	./rw/regress tests.txt
	./rmw/regress tests.txt
	./rsw/regress tests.txt
	./rmsw/regress tests.txt

#
# delete the cache before each build.
# NB this precludes multithread builds:
#
r/regress : $(SOURCES)
	rm -f *.o
	rm -fr SunWS_cache
	$(CXX) -O2 -o r/regress $(SOURCES) -lboost_regex$(LIBSUFFIX) $(LIBS)

rm/regress : $(SOURCES)
	rm -f *.o
	rm -fr SunWS_cache
	$(CXX) -O2 -mt -o rm/regress $(SOURCES) -lboost_regex_mt$(LIBSUFFIX) $(LIBS)

rs/regress : $(SOURCES)
	rm -f *.o
	rm -fr SunWS_cache
	$(CXX) -O2 -o rs/regress $(SOURCES) -Bstatic -lboost_regex$(LIBSUFFIX) -Bdynamic $(LIBS)

rms/regress : $(SOURCES)
	rm -f *.o
	rm -fr SunWS_cache
	$(CXX) -O2 -mt -o rms/regress $(SOURCES) -Bstatic -lboost_regex_mt$(LIBSUFFIX) -Bdynamic $(LIBS)

rw/regress : $(SOURCES)
	rm -f *.o
	rm -fr SunWS_cache
	$(CXX) -O2 -DTEST_UNICODE -o rw/regress $(SOURCES) -lboost_regex$(LIBSUFFIX) $(LIBS)

rmw/regress : $(SOURCES)
	rm -f *.o
	rm -fr SunWS_cache
	$(CXX) -O2 -mt -DTEST_UNICODE -o rmw/regress $(SOURCES) -lboost_regex_mt$(LIBSUFFIX) $(LIBS)

rsw/regress : $(SOURCES)
	rm -f *.o
	rm -fr SunWS_cache
	$(CXX) -O2 -DTEST_UNICODE -o rsw/regress $(SOURCES) -Bstatic -lboost_regex$(LIBSUFFIX) -Bdynamic $(LIBS)

rmsw/regress : $(SOURCES)
	rm -f *.o
	rm -fr SunWS_cache
	$(CXX) -O2 -mt -DTEST_UNICODE -o rmsw/regress $(SOURCES) -Bstatic -lboost_regex_mt$(LIBSUFFIX) -Bdynamic $(LIBS)

r:
	mkdir -p r

rm:
	mkdir -p rm

rs:
	mkdir -p rs

rms:
	mkdir -p rms

rw:
	mkdir -p rw

rmw:
	mkdir -p rmw

rsw:
	mkdir -p rsw

rmsw:
	mkdir -p rmsw

clean:
	rm -f *.o
	rm -fr SunWS_cache
	rm -fr r rm rs rms rw rmw rsw rmsw














































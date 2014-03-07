#!/usr/bin/python

import os
import string


call = "   hook(10);\n";
call = "   hook(10); hook2(10);hook3(0);hook4(0);\n";

def run_test(num_calls, compiler_command):
   f = open("program_options_test.cpp", "w")
   f.write("""#include <boost/program_options.hpp>
using namespace boost::program_options;   

void do_it()
{
   boost::program_options::options_description desc;
   desc.add_options()
""")
   for i in range(0, num_calls):
      f.write("(\"opt%d\", value<int>())\n")
   f.write(";\n}\n")   
   f.close()
   os.system(compiler_command + " -c -save-temps -I /home/ghost/Work/Boost/boost-svn program_options_test.cpp")

   nm = os.popen("nm -S program_options_test.o")
   for l in nm:
      if string.find(l, "Z5do_itv") != -1:
         break
   size = int(string.split(l)[1], 16)
   return size

def run_tests(range, compiler_command):

   last_size = None
   first_size = None
   for num in range:
      size = run_test(num, compiler_command)
      if last_size:
         print "%2d calls: %5d bytes (+ %d)" % (num, size, size-last_size)
      else:
         print "%2d calls: %5d bytes" % (num, size)
         first_size = size
      last_size = size
   print "Avarage: ", (last_size-first_size)/(range[-1]-range[0])

if __name__ == '__main__':
   for compiler in [ "g++ -Os", "g++ -O3"]:
      print "****", compiler, "****"
      run_tests(range(1, 20), compiler)


 

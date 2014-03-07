// Copyright Sascha Ochsenknecht 2009.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/program_options.hpp>
using namespace boost::program_options;

#include <string>
#include <iostream>
#include <fstream>
using namespace std;

#include "minitest.hpp"


void required_throw_test()
{
   options_description          opts;
   opts.add_options()
     ("cfgfile,c", value<string>()->required(), "the configfile")
     ("fritz,f",   value<string>()->required(), "the output file")
   ;

   variables_map vm;
   bool throwed = false;
   {
      // This test must throw exception
      string cmdline = "prg -f file.txt";
      vector< string > tokens =  split_unix(cmdline);
      throwed = false;
      try {
         store(command_line_parser(tokens).options(opts).run(), vm);
         notify(vm);    
      } 
      catch (required_option& e) {
         BOOST_CHECK_EQUAL(e.what(), string("the option '--cfgfile' is required but missing"));
         throwed = true;
      }      
      BOOST_CHECK(throwed);
   }
   
   {
      // This test mustn't throw exception
      string cmdline = "prg -c config.txt";
      vector< string > tokens =  split_unix(cmdline);
      throwed = false;
      try {
         store(command_line_parser(tokens).options(opts).run(), vm);
         notify(vm);    
      } 
      catch (required_option& e) {
         throwed = true;
      }
      BOOST_CHECK(!throwed);
   }
}



void simple_required_test(const char* config_file)
{
   options_description          opts;
   opts.add_options()
     ("cfgfile,c", value<string>()->required(), "the configfile")
     ("fritz,f",   value<string>()->required(), "the output file")
   ;

   variables_map vm;
   bool throwed = false;
   {
      // This test must throw exception
      string cmdline = "prg -f file.txt";
      vector< string > tokens =  split_unix(cmdline);
      throwed = false;
      try {
         // options coming from different sources
         store(command_line_parser(tokens).options(opts).run(), vm);
         store(parse_config_file<char>(config_file, opts), vm);
         notify(vm);    
      } 
      catch (required_option& e) {
         throwed = true;
      }      
      BOOST_CHECK(!throwed);
   }
}



int main(int /*argc*/, char* av[])
{  
   required_throw_test();
   simple_required_test(av[1]);
   
   return 0;
}


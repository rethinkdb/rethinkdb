// Copyright Sascha Ochsenknecht 2009.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/detail/cmdline.hpp>
using namespace boost::program_options;
using boost::program_options::detail::cmdline;

#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>
using namespace std;

#include "minitest.hpp"


// Test free function collect_unrecognized()
//
//  it collects the tokens of all not registered options. It can be used
//  to pass them to an own parser implementation



void test_unrecognize_cmdline() 
{
   options_description desc;
   
   string content = "prg --input input.txt --optimization 4 --opt option";   
   vector< string > tokens =  split_unix(content);

   cmdline cmd(tokens);
   cmd.set_options_description(desc);
   cmd.allow_unregistered();

   vector< option > opts = cmd.run();
   vector< string > result = collect_unrecognized(opts, include_positional);
   
   BOOST_CHECK_EQUAL(result.size(), 7);
   BOOST_CHECK_EQUAL(result[0], "prg");
   BOOST_CHECK_EQUAL(result[1], "--input");
   BOOST_CHECK_EQUAL(result[2], "input.txt");
   BOOST_CHECK_EQUAL(result[3], "--optimization");
   BOOST_CHECK_EQUAL(result[4], "4");
   BOOST_CHECK_EQUAL(result[5], "--opt");
   BOOST_CHECK_EQUAL(result[6], "option");
}



void test_unrecognize_config() 
{

   options_description desc;
   
   string content  =
    " input = input.txt\n"
    " optimization = 4\n"
    " opt = option\n"
    ;

   stringstream ss(content);
   vector< option > opts = parse_config_file(ss, desc, true).options;
   vector< string > result = collect_unrecognized(opts, include_positional);
   
   BOOST_CHECK_EQUAL(result.size(), 6);
   BOOST_CHECK_EQUAL(result[0], "input");
   BOOST_CHECK_EQUAL(result[1], "input.txt");
   BOOST_CHECK_EQUAL(result[2], "optimization");
   BOOST_CHECK_EQUAL(result[3], "4");
   BOOST_CHECK_EQUAL(result[4], "opt");
   BOOST_CHECK_EQUAL(result[5], "option");
}



int main(int /*ac*/, char** /*av*/)
{
   test_unrecognize_cmdline();
   test_unrecognize_config();

   return 0;
}

//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//This example shows how boost::tuple can be used to bundle the
//name of the function that fails together with the reported errno.

#include <boost/exception/info_tuple.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/shared_ptr.hpp>
#include <stdio.h>
#include <string>
#include <errno.h>

typedef boost::tuple<boost::errinfo_api_function,boost::errinfo_errno> clib_failure;

struct file_open_error: virtual boost::exception { };

boost::shared_ptr<FILE>
file_open( char const * name, char const * mode )
    {
    if( FILE * f=fopen(name,mode) )
        return boost::shared_ptr<FILE>(f,fclose);
    else
        throw file_open_error() <<
            boost::errinfo_file_name(name) <<
            clib_failure("fopen",errno);
    }

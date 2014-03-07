//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//This example demonstrates the intended use of various commonly used
//error_info typedefs provided by Boost Exception.

#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_at_line.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_handle.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/exception/errinfo_file_open_mode.hpp>
#include <boost/exception/info.hpp>
#include <boost/throw_exception.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <stdio.h>
#include <errno.h>
#include <exception>

struct error : virtual std::exception, virtual boost::exception { };
struct file_error : virtual error { };
struct file_open_error: virtual file_error { };
struct file_read_error: virtual file_error { };

boost::shared_ptr<FILE>
open_file( char const * file, char const * mode )
    {
    if( FILE * f=fopen(file,mode) )
        return boost::shared_ptr<FILE>(f,fclose);
    else
        BOOST_THROW_EXCEPTION(
            file_open_error() <<
            boost::errinfo_api_function("fopen") <<
            boost::errinfo_errno(errno) <<
            boost::errinfo_file_name(file) <<
            boost::errinfo_file_open_mode(mode) );
    }

size_t
read_file( boost::shared_ptr<FILE> const & f, void * buf, size_t size )
    {
    size_t nr=fread(buf,1,size,f.get());
    if( ferror(f.get()) )
        BOOST_THROW_EXCEPTION(
            file_read_error() <<
            boost::errinfo_api_function("fread") <<
            boost::errinfo_errno(errno) <<
            boost::errinfo_file_handle(f) );
    return nr;
    }

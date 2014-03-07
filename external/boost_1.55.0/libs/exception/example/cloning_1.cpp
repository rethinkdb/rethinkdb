//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//This example shows how to enable cloning when throwing a boost::exception.

#include <boost/exception/info.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <stdio.h>
#include <errno.h>

struct file_read_error: virtual boost::exception { };

void
file_read( FILE * f, void * buffer, size_t size )
    {
    if( size!=fread(buffer,1,size,f) )
        throw boost::enable_current_exception(file_read_error()) <<
            boost::errinfo_errno(errno);
    }

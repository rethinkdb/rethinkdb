//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//This example shows how to add arbitrary data to active exception objects.

#include <boost/exception/all.hpp>
#include <boost/shared_ptr.hpp>
#include <stdio.h>
#include <errno.h>

//

struct file_read_error: virtual boost::exception { };

void
file_read( FILE * f, void * buffer, size_t size )
    {
    if( size!=fread(buffer,1,size,f) )
        throw file_read_error() << boost::errinfo_errno(errno);
    }

//

boost::shared_ptr<FILE> file_open( char const * file_name, char const * mode );
void file_read( FILE * f, void * buffer, size_t size );

void
parse_file( char const * file_name )
    {
    boost::shared_ptr<FILE> f = file_open(file_name,"rb");
    assert(f);
    try
        {
        char buf[1024];
        file_read( f.get(), buf, sizeof(buf) );
        }
    catch(
    boost::exception & e )
        {
        e << boost::errinfo_file_name(file_name);
        throw;
        }
    }

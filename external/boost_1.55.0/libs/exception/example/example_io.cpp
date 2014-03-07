//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//This program simulates errors on copying simple data files. It demonstrates
//typical Boost Exception usage.

//The output from this program can vary depending on the platform.

#include <boost/throw_exception.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception/errinfo_file_open_mode.hpp>
#include <boost/exception/errinfo_file_handle.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <iostream>

typedef boost::error_info<struct tag_file_name_src,std::string> errinfo_src_file_name;
typedef boost::error_info<struct tag_file_name_dst,std::string> errinfo_dst_file_name;

char const data[] = "example";
size_t const data_size = sizeof(data);

class
error: //Base for all exception objects we throw.
    public virtual std::exception,
    public virtual boost::exception
    {
    public:

    char const *
    what() const throw()
        {
        return "example_io error";
        }

    protected:

    ~error() throw()
        {
        }
    };

struct open_error: virtual error { };
struct read_error: virtual error { };
struct write_error: virtual error { };
struct fopen_error: virtual open_error { };
struct fread_error: virtual read_error { };
struct fwrite_error: virtual write_error { };

boost::shared_ptr<FILE>
my_fopen( char const * name, char const * mode )
    {
    if( FILE * f = ::fopen(name,mode) )
        return boost::shared_ptr<FILE>(f,fclose);
    else
        BOOST_THROW_EXCEPTION(fopen_error() <<
            boost::errinfo_errno    (errno) <<
            boost::errinfo_file_name(name) <<
            boost::errinfo_file_open_mode(mode) <<
            boost::errinfo_api_function("fopen"));
    }

void
my_fread( void * buffer, size_t size, size_t count, boost::shared_ptr<FILE> const & stream )
    {
    assert(stream);
    if( count!=fread(buffer,size,count,stream.get()) || ferror(stream.get()) )
        BOOST_THROW_EXCEPTION(fread_error() <<
            boost::errinfo_api_function("fread") <<
            boost::errinfo_errno(errno) <<
            boost::errinfo_file_handle(boost::weak_ptr<FILE>(stream)));
    }

void
my_fwrite( void const * buffer, size_t size, size_t count, boost::shared_ptr<FILE> const & stream )
    {
    assert(stream);
    if( count!=fwrite(buffer,size,count,stream.get()) || ferror(stream.get()) )
        BOOST_THROW_EXCEPTION(fwrite_error() <<
            boost::errinfo_api_function("fwrite") <<
            boost::errinfo_errno(errno) <<
            boost::errinfo_file_handle(boost::weak_ptr<FILE>(stream)));
    }

void
reset_file( char const * file_name )
    {
    (void) my_fopen(file_name,"wb");
    }

void
create_data( char const * file_name )
    {
    boost::shared_ptr<FILE> f = my_fopen(file_name,"wb");
    my_fwrite( data, 1, data_size, f );
    }

void
copy_data( char const * src_file_name, char const * dst_file_name )
    {
    boost::shared_ptr<FILE> src = my_fopen(src_file_name,"rb");
    boost::shared_ptr<FILE> dst = my_fopen(dst_file_name,"wb");
    try
        {
        char buffer[data_size];
        my_fread( buffer, 1, data_size, src );
        my_fwrite( buffer, 1, data_size, dst );
        }
    catch(
    boost::exception & x )
        {
        if( boost::weak_ptr<FILE> const * f=boost::get_error_info<boost::errinfo_file_handle>(x) )
            if( boost::shared_ptr<FILE> fs = f->lock() )
                {
                if( fs==src )
                    x << boost::errinfo_file_name(src_file_name);
                else if( fs==dst )
                    x << boost::errinfo_file_name(dst_file_name);
                }
        x <<
            errinfo_src_file_name(src_file_name) <<
            errinfo_dst_file_name(dst_file_name);
        throw;
        }
    }

void
dump_copy_info( boost::exception const & x )
    {
    if( std::string const * src = boost::get_error_info<errinfo_src_file_name>(x) )
        std::cerr << "Source file name: " << *src << "\n";
    if( std::string const * dst = boost::get_error_info<errinfo_dst_file_name>(x) )
        std::cerr << "Destination file name: " << *dst << "\n";
    }

void
dump_file_info( boost::exception const & x )
    {
    if( std::string const * fn = boost::get_error_info<boost::errinfo_file_name>(x) )
        std::cerr << "File name: " << *fn << "\n";
    }

void
dump_clib_info( boost::exception const & x )
    {
    if( int const * err=boost::get_error_info<boost::errinfo_errno>(x) )
        std::cerr << "OS error: " << *err << "\n";
    if( char const * const * fn=boost::get_error_info<boost::errinfo_api_function>(x) )
        std::cerr << "Failed function: " << *fn << "\n";
    }

void
dump_all_info( boost::exception const & x )
    {
    std::cerr << "-------------------------------------------------\n";
    dump_copy_info(x);
    dump_file_info(x);
    dump_clib_info(x);
    std::cerr << "\nOutput from diagnostic_information():\n";
    std::cerr << diagnostic_information(x);
    }

int
main()
    {
    try
        {
        create_data( "tmp1.txt" );
        copy_data( "tmp1.txt", "tmp2.txt" ); //This should succeed.

        reset_file( "tmp1.txt" ); //Creates empty file.
        try
            {
            copy_data( "tmp1.txt", "tmp2.txt" ); //This should fail, tmp1.txt is empty.
            }
        catch(
        read_error & x )
            {
            std::cerr << "\nCaught 'read_error' exception.\n";
            dump_all_info(x);
            }

        remove( "tmp1.txt" );
        remove( "tmp2.txt" );
        try
            {
            copy_data( "tmp1.txt", "tmp2.txt" ); //This should fail, tmp1.txt does not exist.
            }
        catch(
        open_error & x )
            {
            std::cerr << "\nCaught 'open_error' exception.\n";
            dump_all_info(x);
            }
        }
    catch(
    ... )
        {
        std::cerr << "\nCaught unexpected exception!\n";
        std::cerr << boost::current_exception_diagnostic_information();
        }
    }

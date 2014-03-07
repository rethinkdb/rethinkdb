// (C) Copyright 2010 Daniel James
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// A few methods for getting and manipulating file handles.

#ifndef BOOST_IOSTREAMS_FILE_HANDLE_HPP_INCLUDED
#define BOOST_IOSTREAMS_FILE_HANDLE_HPP_INCLUDED

#include <boost/iostreams/detail/file_handle.hpp>
#include <boost/iostreams/detail/config/rtl.hpp>
#include <boost/test/test_tools.hpp>
#include <string>

#ifdef BOOST_IOSTREAMS_WINDOWS
# include <io.h>         // low-level file i/o.
# define WINDOWS_LEAN_AND_MEAN
# include <windows.h>
#else
# include <sys/stat.h>
# include <fcntl.h>
#endif

namespace boost { namespace iostreams { namespace test {

#ifdef BOOST_IOSTREAMS_WINDOWS

// Windows implementation

boost::iostreams::detail::file_handle open_file_handle(std::string const& name)
{
    HANDLE handle = 
        ::CreateFileA( name.c_str(),
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,                   // lpSecurityAttributes
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL );                 // hTemplateFile

    BOOST_REQUIRE (handle != INVALID_HANDLE_VALUE);
    return handle;
}

void close_file_handle(boost::iostreams::detail::file_handle handle)
{
    BOOST_REQUIRE(::CloseHandle(handle) == 1);
}

#define BOOST_CHECK_HANDLE_CLOSED(handle)
#define BOOST_CHECK_HANDLE_OPEN(handle)

#else // BOOST_IOSTREAMS_WINDOWS

// Non-windows implementation

boost::iostreams::detail::file_handle open_file_handle(std::string const& name)
{
    int oflag = O_RDWR;

    #ifdef _LARGEFILE64_SOURCE
        oflag |= O_LARGEFILE;
    #endif

    // Calculate pmode argument to open.

    mode_t pmode = S_IRUSR | S_IWUSR |
                   S_IRGRP | S_IWGRP |
                   S_IROTH | S_IWOTH;

        // Open file.

    int fd = BOOST_IOSTREAMS_FD_OPEN(name.c_str(), oflag, pmode);
    BOOST_REQUIRE (fd != -1);
    
    return fd;
}

void close_file_handle(boost::iostreams::detail::file_handle handle)
{
    BOOST_REQUIRE(BOOST_IOSTREAMS_FD_CLOSE(handle) != -1);
}

// This is pretty dubious. First you must make sure that no other
// operations that could open a descriptor are called before this
// check, otherwise it's quite likely that a closed descriptor
// could be used. Secondly, I'm not sure if it's guaranteed that
// fcntl will know that the descriptor is closed but this seems
// to work okay, and I can't see any other convenient way to check
// that a descripter has been closed.
bool is_handle_open(boost::iostreams::detail::file_handle handle)
{
    return ::fcntl(handle, F_GETFD) != -1;
}

#define BOOST_CHECK_HANDLE_CLOSED(handle) \
    BOOST_CHECK(!::boost::iostreams::test::is_handle_open(handle))
#define BOOST_CHECK_HANDLE_OPEN(handle) \
    BOOST_CHECK(::boost::iostreams::test::is_handle_open(handle))


#endif // BOOST_IOSTREAMS_WINDOWS

}}}

#endif // BOOST_IOSTREAMS_FILE_HANDLE_HPP_INCLUDED

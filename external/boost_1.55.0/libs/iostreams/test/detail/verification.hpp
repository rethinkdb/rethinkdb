// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.


#ifndef BOOST_IOSTREAMS_TEST_VERIFICATION_HPP_INCLUDED
#define BOOST_IOSTREAMS_TEST_VERIFICATION_HPP_INCLUDED

#include <iostream>
#include <exception>
#include <string>
#include <string.h>
#include <fstream>
#ifndef BOOST_IOSTREAMS_NO_STREAM_TEMPLATES
# include <istream>
# include <ostream>
#else
# include <iostream.h>
#endif

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/iostreams/detail/char_traits.hpp>
#include <boost/iostreams/detail/config/wide_streams.hpp>
#include "./constants.hpp"

// Code generation bugs cause tests to fail with global optimization.
#if BOOST_WORKAROUND(BOOST_MSVC, < 1300)
# pragma optimize("g", off)
#endif

// Included only by tests; no need to #undef.
#ifndef BOOST_IOSTREAMS_NO_STREAM_TEMPLATES
# define BOOST_TEMPLATE_DECL template<typename Ch, typename Tr>
# define BOOST_CHAR Ch
# define BOOST_ISTREAM std::basic_istream<Ch, Tr>
# define BOOST_OSTREAM std::basic_ostream<Ch, Tr>
#else
# define BOOST_TEMPLATE_DECL
# define BOOST_CHAR char
# define BOOST_ISTREAM std::istream
# define BOOST_OSTREAM std::ostream
#endif

namespace boost { namespace iostreams { namespace test {

BOOST_TEMPLATE_DECL
bool compare_streams_in_chars(BOOST_ISTREAM& first, BOOST_ISTREAM& second)
{
    for (int z = 0; z < data_reps; ++z)
        for (int w = 0; w < data_length(); ++w)
            if (first.eof() != second.eof() || first.get() != second.get())
                return false;
    return true;
}

BOOST_TEMPLATE_DECL
bool compare_streams_in_chunks(BOOST_ISTREAM& first, BOOST_ISTREAM& second)
{
    int i = 0;
    do {
        BOOST_CHAR buf_one[chunk_size];
        BOOST_CHAR buf_two[chunk_size];
        first.read(buf_one, chunk_size);
        second.read(buf_two, chunk_size);
        std::streamsize amt = first.gcount();
        if ( amt != static_cast<std::streamsize>(second.gcount()) ||
             BOOST_IOSTREAMS_CHAR_TRAITS(BOOST_CHAR)::
                compare(buf_one, buf_two, amt) != 0 )
            return false;
        ++i;
    } while (!first.eof());
    return true;
}

bool compare_files(const std::string& first, const std::string& second)
{
    using namespace std;
    ifstream one(first.c_str(), BOOST_IOS::in | BOOST_IOS::binary);
    ifstream two(second.c_str(), BOOST_IOS::in | BOOST_IOS::binary);
    return compare_streams_in_chunks(one, two);
}

#ifndef BOOST_IOSTREAMS_NO_STREAM_TEMPLATES
    template<typename Container, typename Ch, typename Tr>
#else
    template<typename Container>
#endif
bool compare_container_and_stream(Container& cnt, BOOST_ISTREAM& is)
{
    typename Container::iterator first = cnt.begin();
    typename Container::iterator last = cnt.end();
    do  {
        if ((first == last) != is.eof()) return false;
        if (first != last && *first++ != is.get()) return false;
    } while (first != last);
    return true;
}

template<typename Container>
bool compare_container_and_file(Container& cnt, const std::string& file)
{
    std::ifstream fstrm(file.c_str(), BOOST_IOS::in | BOOST_IOS::binary);
    return compare_container_and_stream(cnt, fstrm);
}

BOOST_TEMPLATE_DECL
void write_data_in_chars(BOOST_OSTREAM& os)
{
    for (int z = 0; z < data_reps; ++z) 
        for (int w = 0; w < data_length(); ++w) 
            os.put(detail::data((BOOST_CHAR*)0)[w]);
    os.flush();
}

BOOST_TEMPLATE_DECL
void write_data_in_chunks(BOOST_OSTREAM& os)
{
    const BOOST_CHAR* buf = detail::data((BOOST_CHAR*)0);
    for (int z = 0; z < data_reps; ++z)
        os.write(buf, data_length());
    os.flush();
}

bool test_seekable_in_chars(std::iostream& io)
{
    int i;  // old 'for' scope workaround.

    // Test seeking with ios::cur
    for (i = 0; i < data_reps; ++i) {
        int j;
        for (j = 0; j < chunk_size; ++j)
            io.put(narrow_data()[j]);
        io.seekp(-chunk_size, BOOST_IOS::cur);
        for (j = 0; j < chunk_size; ++j)
            if (io.get() != narrow_data()[j])
               return false;
        io.seekp(-chunk_size, BOOST_IOS::cur);
        for (j = 0; j < chunk_size; ++j)
            io.put(narrow_data()[j]);
    }

    // Test seeking with ios::beg
    std::streamoff off = 0;
    io.seekp(0, BOOST_IOS::beg);
    for (i = 0; i < data_reps; ++i, off += chunk_size) {
        int j;
        for (j = 0; j < chunk_size; ++j)
            io.put(narrow_data()[j]);
        io.seekp(off, BOOST_IOS::beg);
        for (j = 0; j < chunk_size; ++j)
            if (io.get() != narrow_data()[j])
               return false;
        io.seekp(off, BOOST_IOS::beg);
        for (j = 0; j < chunk_size; ++j)
            io.put(narrow_data()[j]);
    }

    // Test seeking with ios::end
    io.seekp(0, BOOST_IOS::end);
    off = io.tellp();
    io.seekp(-off, BOOST_IOS::end);
    for (i = 0; i < data_reps; ++i, off -= chunk_size) {
        int j;
        for (j = 0; j < chunk_size; ++j)
            io.put(narrow_data()[j]);
        io.seekp(-off, BOOST_IOS::end);
        for (j = 0; j < chunk_size; ++j)
            if (io.get() != narrow_data()[j])
               return false;
        io.seekp(-off, BOOST_IOS::end);
        for (j = 0; j < chunk_size; ++j)
            io.put(narrow_data()[j]);
    }
    return true;
}

bool test_seekable_in_chunks(std::iostream& io)
{
    int i;  // old 'for' scope workaround.

    // Test seeking with ios::cur
    for (i = 0; i < data_reps; ++i) {
        io.write(narrow_data(), chunk_size);
        io.seekp(-chunk_size, BOOST_IOS::cur);
        char buf[chunk_size];
        io.read(buf, chunk_size);
        if (strncmp(buf, narrow_data(), chunk_size) != 0)
            return false;
        io.seekp(-chunk_size, BOOST_IOS::cur);
        io.write(narrow_data(), chunk_size);
    }

    // Test seeking with ios::beg
    std::streamoff off = 0;
    io.seekp(0, BOOST_IOS::beg);
    for (i = 0; i < data_reps; ++i, off += chunk_size) {
        io.write(narrow_data(), chunk_size);
        io.seekp(off, BOOST_IOS::beg);
        char buf[chunk_size];
        io.read(buf, chunk_size);
        if (strncmp(buf, narrow_data(), chunk_size) != 0)
            return false;
        io.seekp(off, BOOST_IOS::beg);
        io.write(narrow_data(), chunk_size);
    }
    
    // Test seeking with ios::end
    io.seekp(0, BOOST_IOS::end);
    off = io.tellp();
    io.seekp(-off, BOOST_IOS::end);
    for (i = 0; i < data_reps; ++i, off -= chunk_size) {
        io.write(narrow_data(), chunk_size);
        io.seekp(-off, BOOST_IOS::end);
        char buf[chunk_size];
        io.read(buf, chunk_size);
        if (strncmp(buf, narrow_data(), chunk_size) != 0)
            return false;
        io.seekp(-off, BOOST_IOS::end);
        io.write(narrow_data(), chunk_size);
    }
    return true;
}

bool test_input_seekable(std::istream& io)
{
    int i;  // old 'for' scope workaround.

    // Test seeking with ios::cur
    for (i = 0; i < data_reps; ++i) {
        for (int j = 0; j < chunk_size; ++j)
            if (io.get() != narrow_data()[j])
               return false;
        io.seekg(-chunk_size, BOOST_IOS::cur);
        char buf[chunk_size];
        io.read(buf, chunk_size);
        if (strncmp(buf, narrow_data(), chunk_size) != 0)
            return false;
    }

    // Test seeking with ios::beg
    std::streamoff off = 0;
    io.seekg(0, BOOST_IOS::beg);
    for (i = 0; i < data_reps; ++i, off += chunk_size) {
        for (int j = 0; j < chunk_size; ++j)
            if (io.get() != narrow_data()[j])
               return false;
        io.seekg(off, BOOST_IOS::beg);
        char buf[chunk_size];
        io.read(buf, chunk_size);
        if (strncmp(buf, narrow_data(), chunk_size) != 0)
            return false;
    }
    
    // Test seeking with ios::end
    io.seekg(0, BOOST_IOS::end);
    off = io.tellg();
    io.seekg(-off, BOOST_IOS::end);
    for (i = 0; i < data_reps; ++i, off -= chunk_size) {
        for (int j = 0; j < chunk_size; ++j)
            if (io.get() != narrow_data()[j])
               return false;
        io.seekg(-off, BOOST_IOS::end);
        char buf[chunk_size];
        io.read(buf, chunk_size);
        if (strncmp(buf, narrow_data(), chunk_size) != 0)
            return false;
    }
    return true;
}

bool test_output_seekable(std::ostream& io)
{
    int i;  // old 'for' scope workaround.

    // Test seeking with ios::cur
    for (i = 0; i < data_reps; ++i) {
        for (int j = 0; j < chunk_size; ++j)
            io.put(narrow_data()[j]);
        io.seekp(-chunk_size, BOOST_IOS::cur);
        io.write(narrow_data(), chunk_size);
    }

    // Test seeking with ios::beg
    std::streamoff off = 0;
    io.seekp(0, BOOST_IOS::beg);
    for (i = 0; i < data_reps; ++i, off += chunk_size) {
        for (int j = 0; j < chunk_size; ++j)
            io.put(narrow_data()[j]);
        io.seekp(off, BOOST_IOS::beg);
        io.write(narrow_data(), chunk_size);
    }
    
    // Test seeking with ios::end
    io.seekp(0, BOOST_IOS::end);
    off = io.tellp();
    io.seekp(-off, BOOST_IOS::end);
    for (i = 0; i < data_reps; ++i, off -= chunk_size) {
        for (int j = 0; j < chunk_size; ++j)
            io.put(narrow_data()[j]);
        io.seekp(-off, BOOST_IOS::end);
        io.write(narrow_data(), chunk_size);
    }
    return true;
}

} } } // End namespaces test, iostreams, boost.

#if BOOST_WORKAROUND(BOOST_MSVC, < 1300)
# pragma optimize("", on)
#endif

#endif // #ifndef BOOST_IOSTREAMS_TEST_VERIFICATION_HPP_INCLUDED

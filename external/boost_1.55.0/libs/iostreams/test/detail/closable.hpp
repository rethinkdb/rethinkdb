/*
 * Distributed under the Boost Software License, Version 1.0.(See accompanying 
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)
 * 
 * See http://www.boost.org/libs/iostreams for documentation.
 *
 * Defines a large collection of closable filters and devices that
 * execute instances of boost::iostreams::test::operation upon
 * closre(). Used to verify that filters and devices are closed 
 * correctly by the iostreams library
 *
 * File:        libs/iostreams/test/detail/closable.hpp
 * Date:        Sun Dec 09 16:12:23 MST 2007
 * Copyright:   2007-2008 CodeRage, LLC
 * Author:      Jonathan Turkanis
 * Contact:     turkanis at coderage dot com
 */

#ifndef BOOST_IOSTREAMS_TEST_CLOSABLE_HPP_INCLUDED
#define BOOST_IOSTREAMS_TEST_CLOSABLE_HPP_INCLUDED

#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/char_traits.hpp>  // EOF
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/detail/ios.hpp>
#include "./operation_sequence.hpp"

namespace boost { namespace iostreams { namespace test {

template<typename Category>
class closable_device { };

// Source
template<>
class closable_device<input> : public source {
public:
    closable_device(operation close) : close_(close) { }
    std::streamsize read(char*, std::streamsize) { return -1; }
    void close() { close_.execute(); }
private:
    operation close_;
};

// Sink
template<>
class closable_device<output> : public sink {
public:
    closable_device(operation close) : close_(close) { }
    std::streamsize write(const char*, std::streamsize) { return 0; }
    void close() { close_.execute(); }
private:
    operation close_;
};

struct borland_output { };

// Copy of closable_device<output>, for Borland <= 5.8.2
template<>
class closable_device<borland_output> : public sink {
public:
    closable_device(operation close) : close_(close) { }
    std::streamsize write(const char*, std::streamsize) { return 0; }
    void close() { close_.execute(); }
private:
    operation close_;
};

// Bidirectional device
template<>
class closable_device<bidirectional> : public device<bidirectional> {
public:
    closable_device(operation close_input, operation close_output) 
        : close_input_(close_input), close_output_(close_output)
        { }
    std::streamsize read(char*, std::streamsize) { return -1; }
    std::streamsize write(const char*, std::streamsize) { return 0; }
    void close(BOOST_IOS::openmode which) 
    { 
        switch (which) {
        case BOOST_IOS::in:
            close_input_.execute();
            break;
        case BOOST_IOS::out:
            close_output_.execute();
            break;
        default:
            break;
        }
    }
private:
    operation close_input_;
    operation close_output_;
};

// Seekable device
template<>
class closable_device<seekable> : public device<seekable> {
public:
    closable_device(operation close) : close_(close) { }
    std::streamsize read(char*, std::streamsize) { return -1; }
    std::streamsize write(const char*, std::streamsize) { return 0; }
    stream_offset seek(stream_offset, BOOST_IOS::seekdir) { return 0; }
    void close() { close_.execute(); }
private:
    operation close_;
};

struct direct_input 
    : input, device_tag, closable_tag, direct_tag 
    { };
struct direct_output 
    : output, device_tag, closable_tag, direct_tag 
    { };
struct direct_bidirectional 
    : bidirectional, device_tag, closable_tag, direct_tag 
    { };
struct direct_seekable 
    : seekable, device_tag, closable_tag, direct_tag 
    { };

// Direct source
template<>
class closable_device<direct_input> {
public:
    typedef char          char_type;
    typedef direct_input  category;
    closable_device(operation close) : close_(close) { }
    std::pair<char*, char*> input_sequence()
    { return std::pair<char*, char*>(static_cast<char*>(0), static_cast<char*>(0)); }
    void close() { close_.execute(); }
private:
    operation close_;
};

// Direct sink
template<>
class closable_device<direct_output> {
public:
    typedef char           char_type;
    typedef direct_output  category;
    closable_device(operation close) : close_(close) { }
    std::pair<char*, char*> output_sequence()
    { return std::pair<char*, char*>(static_cast<char*>(0), static_cast<char*>(0)); }
    void close() { close_.execute(); }
private:
    operation close_;
};

// Direct bidirectional device
template<>
class closable_device<direct_bidirectional> {
public:
    typedef char                  char_type;
    typedef direct_bidirectional  category;
    closable_device(operation close_input, operation close_output) 
        : close_input_(close_input), close_output_(close_output)
        { }
    std::pair<char*, char*> input_sequence()
    { return std::pair<char*, char*>(static_cast<char*>(0), static_cast<char*>(0)); }
    std::pair<char*, char*> output_sequence()
    { return std::pair<char*, char*>(static_cast<char*>(0), static_cast<char*>(0)); }
    void close(BOOST_IOS::openmode which) 
    { 
        switch (which) {
        case BOOST_IOS::in:
            close_input_.execute();
            break;
        case BOOST_IOS::out:
            close_output_.execute();
            break;
        default:
            break;
        }
    }
private:
    operation close_input_;
    operation close_output_;
};

// Direct seekable device
template<>
class closable_device<direct_seekable> {
public:
    typedef char             char_type;
    typedef direct_seekable  category;
    closable_device(operation close) : close_(close) { }
    std::pair<char*, char*> input_sequence()
    { return std::pair<char*, char*>(static_cast<char*>(0), static_cast<char*>(0)); }
    std::pair<char*, char*> output_sequence()
    { return std::pair<char*, char*>(static_cast<char*>(0), static_cast<char*>(0)); }
    void close() { close_.execute(); }
private:
    operation close_;
};

template<typename Mode>
class closable_filter { };

// Input filter
template<>
class closable_filter<input> : public input_filter {
public:
    closable_filter(operation close) : close_(close) { }

    template<typename Source>
    int get(Source&) { return EOF; }

    template<typename Source>
    void close(Source&) { close_.execute(); }
private:
    operation close_;
};

// Output filter
template<>
class closable_filter<output> : public output_filter {
public:
    closable_filter(operation close) : close_(close) { }

    template<typename Sink>
    bool put(Sink&, char) { return true; }

    template<typename Sink>
    void close(Sink&) { close_.execute(); }
private:
    operation close_;
};

// Bidirectional filter
template<>
class closable_filter<bidirectional> : public filter<bidirectional> {
public:
    closable_filter(operation close_input, operation close_output) 
        : close_input_(close_input), close_output_(close_output)
        { }

    template<typename Source>
    int get(Source&) { return EOF; }

    template<typename Sink>
    bool put(Sink&, char) { return true; }

    template<typename Device>
    void close(Device&, BOOST_IOS::openmode which) 
    { 
        switch (which) {
        case BOOST_IOS::in:
            close_input_.execute();
            break;
        case BOOST_IOS::out:
            close_output_.execute();
            break;
        default:
            break;
        }
    }
private:
    operation close_input_;
    operation close_output_;
};

// Seekable filter
template<>
class closable_filter<seekable> : public filter<seekable> {
public:
    closable_filter(operation close) : close_(close) { }
    std::streamsize read(char*, std::streamsize) { return -1; }

    template<typename Source>
    int get(Source&) { return EOF; }

    template<typename Sink>
    bool put(Sink&, char) { return true; }

    template<typename Device>
    stream_offset seek(Device&, stream_offset, BOOST_IOS::seekdir)
    {
        return 0;
    }

    template<typename Device>
    void close(Device&) { close_.execute(); }
private:
    operation close_;
};

// Dual-use filter
template<>
class closable_filter<dual_use> {
public:
    typedef char char_type;
    struct category 
        : filter_tag,
          dual_use,
          closable_tag
        { };
    closable_filter(operation close_input, operation close_output) 
        : close_input_(close_input), close_output_(close_output)
        { }

    template<typename Source>
    int get(Source&) { return EOF; }

    template<typename Sink>
    bool put(Sink&, char) { return true; }

    template<typename Device>
    void close(Device&, BOOST_IOS::openmode which) 
    { 
        switch (which) {
        case BOOST_IOS::in:
            close_input_.execute();
            break;
        case BOOST_IOS::out:
            close_output_.execute();
            break;
        default:
            break;
        }
    }
private:
    operation close_input_;
    operation close_output_;
};

// Symmetric filter
class closable_symmetric_filter {
public:
    typedef char char_type;
    closable_symmetric_filter(operation close) : close_(close) { }
    bool filter( const char*&, const char*,
                 char*&, char*, bool )
    {
        return false;
    }
    void close() { close_.execute(); }
private:
    operation close_;
};

} } } // End namespaces test, iostreams, boost.

#endif // #ifndef BOOST_IOSTREAMS_TEST_CLOSABLE_HPP_INCLUDED

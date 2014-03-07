// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_EXAMPLE_CONTAINTER_DEVICE_HPP_INCLUDED
#define BOOST_IOSTREAMS_EXAMPLE_CONTAINTER_DEVICE_HPP_INCLUDED

#include <algorithm>         // copy, min.
#include <cassert>
#include <boost/config.hpp>  // BOOST_NO_STDC_NAMESPACE.
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/detail/ios.hpp>  // failure.

namespace boost { namespace iostreams { namespace example {

//
// Model of Source which reads from an STL-compatible sequence
// whose iterators are random-access iterators.
//
template<typename Container>
class container_source {
public:
    typedef typename Container::value_type  char_type;
    typedef source_tag                      category;
    container_source(Container& container)
        : container_(container), pos_(0)
        { }
    std::streamsize read(char_type* s, std::streamsize n)
    {
        using namespace std;
        std::streamsize amt = 
            static_cast<std::streamsize>(container_.size() - pos_);
        std::streamsize result = (min)(n, amt);
        if (result != 0) {
            std::copy( container_.begin() + pos_,
                       container_.begin() + pos_ + result,
                       s );
            pos_ += result;
            return result;
        } else {
            return -1; // EOF
        }
    }
    Container& container() { return container_; }
private:
    container_source operator=(const container_source&);
    typedef typename Container::size_type   size_type;
    Container&  container_;
    size_type   pos_;
};

//
// Model of Sink which appends to an STL-compatible sequence.
//
template<typename Container>
class container_sink {
public:
    typedef typename Container::value_type  char_type;
    typedef sink_tag                        category;
    container_sink(Container& container) : container_(container) { }
    std::streamsize write(const char_type* s, std::streamsize n)
    {
        container_.insert(container_.end(), s, s + n);
        return n;
    }
    Container& container() { return container_; }
private:
    container_sink operator=(const container_sink&);
    Container& container_;
};

//
// Model of SeekableDevice which accessS an TL-compatible sequence
// whose iterators are random-access iterators.
//
template<typename Container>
class container_device {
public:
    typedef typename Container::value_type  char_type;
    typedef seekable_device_tag             category;
    container_device(Container& container)
        : container_(container), pos_(0)
        { }

    std::streamsize read(char_type* s, std::streamsize n)
    {
        using namespace std;
        std::streamsize amt = 
            static_cast<std::streamsize>(container_.size() - pos_);
        std::streamsize result = (min)(n, amt);
        if (result != 0) {
            std::copy( container_.begin() + pos_,
                       container_.begin() + pos_ + result,
                       s );
            pos_ += result;
            return result;
        } else {
            return -1; // EOF
        }
    }
    std::streamsize write(const char_type* s, std::streamsize n)
    {
        using namespace std;
        std::streamsize result = 0;
        if (pos_ != container_.size()) {
            std::streamsize amt =
                static_cast<std::streamsize>(container_.size() - pos_);
            result = (min)(n, amt);
            std::copy(s, s + result, container_.begin() + pos_);
            pos_ += result;
        }
        if (result < n) {
            container_.insert(container_.end(), s, s + n);
            pos_ = container_.size();
        }
        return n;
    }
    stream_offset seek(stream_offset off, BOOST_IOS::seekdir way)
    {
        using namespace std;

        // Determine new value of pos_
        stream_offset next;
        if (way == BOOST_IOS::beg) {
            next = off;
        } else if (way == BOOST_IOS::cur) {
            next = pos_ + off;
        } else if (way == BOOST_IOS::end) {
            next = container_.size() + off - 1;
        } else {
            throw BOOST_IOSTREAMS_FAILURE("bad seek direction");
        }

        // Check for errors
        if (next < 0 || next >= container_.size())
            throw BOOST_IOSTREAMS_FAILURE("bad seek offset");

        pos_ = next;
        return pos_;
    }

    Container& container() { return container_; }
private:
    container_device operator=(const container_device&);
    typedef typename Container::size_type   size_type;
    Container&  container_;
    size_type   pos_;
};

} } } // End namespaces example, iostreams, boost.

#endif // #ifndef BOOST_IOSTREAMS_EXAMPLE_CONTAINTER_DEVICE_HPP_INCLUDED

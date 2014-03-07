// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// Contains the definitions of several constants used by the test program.

#ifndef BOOST_IOSTREAMS_TEST_FILTERS_HPP_INCLUDED
#define BOOST_IOSTREAMS_TEST_FILTERS_HPP_INCLUDED

#include <boost/config.hpp>                 
#include <algorithm>                            // min.
#include <cctype>                               // to_upper, to_lower.
#include <cstdlib>                              // to_upper, to_lower (VC6).
#include <cstddef>                              // ptrdiff_t.
#include <vector>
#include <boost/iostreams/char_traits.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/constants.hpp>
#include <boost/iostreams/detail/buffer.hpp>
#include <boost/iostreams/detail/iostream.hpp>  // seekdir, streamsize.
#include <boost/iostreams/detail/streambuf.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/pipeline.hpp>
#include <boost/type_traits/is_convertible.hpp>
#ifdef BOOST_NO_STDC_NAMESPACE
namespace std { using ::toupper; using ::tolower; }
#endif

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp>

namespace boost { namespace iostreams { namespace test {

struct toupper_filter : public input_filter {
    template<typename Source>
    int get(Source& s) 
    { 
        int c = boost::iostreams::get(s);
        return c != EOF && c != WOULD_BLOCK ?
            std::toupper((unsigned char) c) :
            c;
    }
};
BOOST_IOSTREAMS_PIPABLE(toupper_filter, 0)

struct tolower_filter : public output_filter {
    template<typename Sink>
    bool put(Sink& s, char c)
    { 
        return boost::iostreams::put(
                   s, (char) std::tolower((unsigned char) c)
               ); 
    }
};
BOOST_IOSTREAMS_PIPABLE(tolower_filter, 0)

struct toupper_multichar_filter : public multichar_input_filter {
    template<typename Source>
    std::streamsize read(Source& s, char* buf, std::streamsize n)
    {
        std::streamsize result = boost::iostreams::read(s, buf, n);
        if (result == -1)
            return -1;
        for (int z = 0; z < result; ++z)
            buf[z] = (char) std::toupper((unsigned char) buf[z]);
        return result;
    }
};
BOOST_IOSTREAMS_PIPABLE(toupper_multichar_filter, 0)

struct tolower_multichar_filter : public multichar_output_filter {
    template<typename Sink>
    std::streamsize write(Sink& s, const char* buf, std::streamsize n)
    {
        std::streamsize result;
        for (result = 0; result < n; ++result) {
            char c = (char) std::tolower((unsigned char) buf[result]);
            if (!boost::iostreams::put(s, c))
                break;
        }
        return result;
    }
};
BOOST_IOSTREAMS_PIPABLE(tolower_multichar_filter, 0)

struct padding_filter : dual_use_filter {
    explicit padding_filter(char pad_char) 
        : pad_char_(pad_char), use_pad_char_(false), eof_(false)
        { }

    template<typename Source>
    int get(Source& src) 
    { 
        int result;
        if (use_pad_char_) {
            result = eof_ ? EOF : pad_char_;
            use_pad_char_ = false;
        } else {
            result = boost::iostreams::get(src);
            if (result != EOF && result != WOULD_BLOCK)
                use_pad_char_ = true;
            eof_ = result == EOF;
        }
        return result;
    }

    template<typename Sink>
    bool put(Sink& s, char c) 
    { 
        if (use_pad_char_) {
            if (!boost::iostreams::put(s, pad_char_))
                return false;
            use_pad_char_ = false;
        } 
        if (!boost::iostreams::put(s, c))
            return false;
        if (!boost::iostreams::put(s, pad_char_))
            use_pad_char_ = true;
        return true;
    }

    char  pad_char_;
    bool  use_pad_char_;
    bool  eof_;
};
BOOST_IOSTREAMS_PIPABLE(padding_filter, 0)

struct flushable_output_filter {
    typedef char char_type;
    struct category 
        : output_filter_tag, 
          flushable_tag
        { };
    template<typename Sink>
    bool put(Sink&, char c) 
    { 
        buf_.push_back(c); 
        return true; 
    }
    template<typename Sink>
    bool flush(Sink& s) 
    { 
        if (!buf_.empty()) {
            boost::iostreams::write(s, &buf_[0], (std::streamsize) buf_.size());
            buf_.clear();
        }
        return true;
    }
    std::vector<char> buf_;
};
BOOST_IOSTREAMS_PIPABLE(flushable_output_filter, 0)

struct identity_seekable_filter : filter<seekable> {
    template<typename Source>
    int get(Source& s) { return boost::iostreams::get(s); }

    template<typename Sink>
    bool put(Sink& s, char c) { return boost::iostreams::put(s, c); }

    template<typename Device>
    std::streampos seek(Device& d, stream_offset off, BOOST_IOS::seekdir way)
    { return boost::iostreams::seek(d, off, way); }
};
BOOST_IOSTREAMS_PIPABLE(identity_seekable_filter, 0)

struct identity_seekable_multichar_filter : multichar_filter<seekable> {
    template<typename Source>
    std::streamsize read(Source& s, char* buf, std::streamsize n)
    { return boost::iostreams::read(s, buf, n); }
    template<typename Sink>
    std::streamsize write(Sink& s, const char* buf, std::streamsize n)
    { return boost::iostreams::write(s, buf, n); }
    template<typename Device>
    std::streampos seek(Device& d, stream_offset off, BOOST_IOS::seekdir way)
    { return boost::iostreams::seek(d, off, way); }
};
BOOST_IOSTREAMS_PIPABLE(identity_seekable_multichar_filter, 0)

} } } // End namespaces detail, iostreams, boost.

#include <boost/iostreams/detail/config/enable_warnings.hpp>

#endif // #ifndef BOOST_IOSTREAMS_TEST_FILTERS_HPP_INCLUDED

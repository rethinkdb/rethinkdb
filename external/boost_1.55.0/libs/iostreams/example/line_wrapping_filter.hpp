// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// Adapted from an example of James Kanze. See
// http://www.gabi-soft.fr/codebase-en.html.

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_LINE_WRAPPING_FILTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_LINE_WRAPPING_FILTER_HPP_INCLUDED

#include <cstdio>                            // EOF.
#include <boost/iostreams/concepts.hpp>      // output_filter.
#include <boost/iostreams/filter/stdio.hpp>
#include <boost/iostreams/operations.hpp>    // boost::iostreams::put.

namespace boost { namespace iostreams { namespace example {

class line_wrapping_stdio_filter : public stdio_filter {
public:
    explicit line_wrapping_stdio_filter(int line_length = 80)
        : line_length_(line_length), col_no_(0)
        { }
private:
    void do_filter()
    {
        int c;
        while ((c = std::cin.get()) != EOF) {
            if (c != '\n' && col_no_ >= line_length_)
                put_char('\n');
            put_char(c);
        }
    }
    void do_close() { col_no_ = 0; }
    void put_char(int c)
    {
        std::cout.put(c);
        if (c != '\n')
            ++col_no_;
        else
            col_no_ = 0;
    }
    int  line_length_;
    int  col_no_;
};

class line_wrapping_input_filter : public input_filter {
public:
    explicit line_wrapping_input_filter(int line_length = 80)
        : line_length_(line_length), col_no_(0), has_next_(false)
        { }

    template<typename Source>
    int get(Source& src)
    {
        if (has_next_) {
            has_next_ = false;
            return get_char(next_);
        }

        int c;
        if ((c = iostreams::get(src)) == EOF || c == WOULD_BLOCK)
            return c;

        if (c != '\n' && col_no_ >= line_length_) {
            next_ = c;
            has_next_ = true;
            return get_char('\n');
        }

        return get_char(c);
    }

    template<typename Sink>
    void close(Sink&)
    {
        col_no_ = 0;
        has_next_ = false;
    }
private:
    int get_char(int c)
    {
        if (c != '\n')
            ++col_no_;
        else
            col_no_ = 0;
        return c;
    }
    int  line_length_;
    int  col_no_;
    int  next_;
    int  has_next_;
};

class line_wrapping_output_filter : public output_filter {
public:
    explicit line_wrapping_output_filter(int line_length = 80)
        : line_length_(line_length), col_no_(0)
        { }

    template<typename Sink>
    bool put(Sink& dest, int c)
    {
        if (c != '\n' && col_no_ >= line_length_ && !put_char(dest, '\n'))
            return false;
        return put_char(dest, c);
    }

    template<typename Sink>
    void close(Sink&) { col_no_ = 0; }
private:
    template<typename Sink>
    bool put_char(Sink& dest, int c)
    {
        if (!iostreams::put(dest, c))
            return false;
        if (c != '\n')
            ++col_no_;
        else
            col_no_ = 0;
        return true;
    }
    int  line_length_;
    int  col_no_;
};

} } }       // End namespaces example, iostreams, boost.

#endif // #ifndef BOOST_IOSTREAMS_LINE_WRAPPING_FILTER_HPP_INCLUDED

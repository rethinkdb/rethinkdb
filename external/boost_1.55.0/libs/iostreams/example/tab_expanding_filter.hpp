// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// Adapted from an example of James Kanze, with suggestions from Rob Stewart.
// See http://www.gabi-soft.fr/codebase-en.html.

#ifndef BOOST_IOSTREAMS_TAB_EXPANDING_FILTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_TAB_EXPANDING_FILTER_HPP_INCLUDED

#include <cassert>
#include <cstdio>    // EOF.
#include <iostream>  // cin, cout.
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/filter/stdio.hpp>
#include <boost/iostreams/operations.hpp>

namespace boost { namespace iostreams { namespace example {

class tab_expanding_stdio_filter : public stdio_filter {
public:
    explicit tab_expanding_stdio_filter(int tab_size = 8)
        : tab_size_(tab_size), col_no_(0)
    {
        assert(tab_size > 0);
    }
private:
    void do_filter()
    {
        int c;
        while ((c = std::cin.get()) != EOF) {
            if (c == '\t') {
                int spaces = tab_size_ - (col_no_ % tab_size_);
                for (; spaces > 0; --spaces)
                    put_char(' ');
            } else {
                put_char(c);
            }
        }
    }
    void do_close() { col_no_ = 0; }
    void put_char(int c)
    {
        std::cout.put(c);
        if (c == '\n') {
            col_no_ = 0;
        } else {
            ++col_no_;
        }
    }
    int  tab_size_;
    int  col_no_;
};

class tab_expanding_input_filter : public input_filter {
public:
    explicit tab_expanding_input_filter(int tab_size = 8)
        : tab_size_(tab_size), col_no_(0), spaces_(0)
    {
        assert(tab_size > 0);
    }

    template<typename Source>
    int get(Source& src)
    {
        if (spaces_ > 0) {
            --spaces_;
            return get_char(' ');
        }

        int c;
        if ((c = iostreams::get(src)) == EOF || c == WOULD_BLOCK)
            return c;

        if (c != '\t')
            return get_char(c);

        // Found a tab. Call this filter recursively.
        spaces_ = tab_size_ - (col_no_ % tab_size_);
        return this->get(src);
    }

    template<typename Source>
    void close(Source&)
    {
        col_no_ = 0;
        spaces_ = 0;
    }
private:
    int get_char(int c)
    {
        if (c == '\n') {
            col_no_ = 0;
        } else {
            ++col_no_;
        }
        return c;
    }
    int   tab_size_;
    int   col_no_;
    int   spaces_;
};

class tab_expanding_output_filter : public output_filter {
public:
    explicit tab_expanding_output_filter(int tab_size = 8)
        : tab_size_(tab_size), col_no_(0), spaces_(0)
    {
        assert(tab_size > 0);
    }

    template<typename Sink>
    bool put(Sink& dest, int c)
    {
        for (; spaces_ > 0; --spaces_)
            if (!put_char(dest, ' '))
                return false;

        if (c == '\t') {
            spaces_ = tab_size_ - (col_no_ % tab_size_) - 1;
            return this->put(dest, ' ');
        }

        return put_char(dest, c);
    }

    template<typename Sink>
    void close(Sink&)
    {
        col_no_ = 0;
        spaces_ = 0;
    }
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
    int  tab_size_;
    int  col_no_;
    int  spaces_;
};

} } } // End namespaces example, iostreams, boost.

#endif // #ifndef BOOST_IOSTREAMS_TAB_EXPANDING_FILTER_HPP_INCLUDED

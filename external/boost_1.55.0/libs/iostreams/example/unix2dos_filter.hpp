// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_UNIX2DOS_FILTER_FILTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_UNIX2DOS_FILTER_FILTER_HPP_INCLUDED

#include <cassert>
#include <cstdio>    // EOF.
#include <iostream>  // cin, cout.
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/filter/stdio.hpp>
#include <boost/iostreams/operations.hpp>

namespace boost { namespace iostreams { namespace example {

class unix2dos_stdio_filter : public stdio_filter {
private:
    void do_filter()
    {
        int c;
        while ((c = std::cin.get()) != EOF) {
            if (c == '\n')
                std::cout.put('\r');
            std::cout.put(c);
        }
    }
};

class unix2dos_input_filter : public input_filter {
public:
    unix2dos_input_filter() : has_linefeed_(false) { }

    template<typename Source>
    int get(Source& src)
    {
        if (has_linefeed_) {
            has_linefeed_ = false;
            return '\n';
        }

        int c;
        if ((c = iostreams::get(src)) == '\n') {
            has_linefeed_ = true;
            return '\r';
        }

        return c;
    }

    template<typename Source>
    void close(Source&) { has_linefeed_ = false; }
private:
    bool has_linefeed_;
};

class unix2dos_output_filter : public output_filter {
public:
    unix2dos_output_filter() : has_linefeed_(false) { }

    template<typename Sink>
    bool put(Sink& dest, int c)
    {
        if (c == '\n')
            return has_linefeed_ ?
                put_char(dest, '\n') :
                put_char(dest, '\r') ?
                    this->put(dest, '\n') :
                    false;
        return iostreams::put(dest, c);
    }

    template<typename Sink>
    void close(Sink&) { has_linefeed_ = false; }
private:
    template<typename Sink>
    bool put_char(Sink& dest, int c)
    {
        bool result;
        if ((result = iostreams::put(dest, c)) == true) {
            has_linefeed_ =
                c == '\r' ?
                    true :
                    c == '\n' ?
                        false :
                        has_linefeed_;
        }
        return result;
    }

    bool has_linefeed_;
};

} } } // End namespaces example, iostreams, boost.

#endif // #ifndef BOOST_IOSTREAMS_UNIX2DOS_FILTER_FILTER_HPP_INCLUDED

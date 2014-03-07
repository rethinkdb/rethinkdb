// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// Adapted from an example of James Kanze, with suggestions from Peter Dimov.
// See http://www.gabi-soft.fr/codebase-en.html.

#ifndef BOOST_IOSTREAMS_SHELL_COMMENTS_FILTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_SHELL_COMMENTS_FILTER_HPP_INCLUDED

#include <cassert>
#include <cstdio>    // EOF.
#include <iostream>  // cin, cout.
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/detail/ios.hpp>  // BOOST_IOS.
#include <boost/iostreams/filter/stdio.hpp>
#include <boost/iostreams/operations.hpp>

namespace boost { namespace iostreams { namespace example {

class shell_comments_stdio_filter : public stdio_filter {
public:
    explicit shell_comments_stdio_filter(char comment_char = '#')
        : comment_char_(comment_char)
        { }
private:
    void do_filter()
    {
        bool  skip = false;
        int   c;
        while ((c = std::cin.get()) != EOF) {
            skip = c == comment_char_ ?
                true :
                c == '\n' ?
                    false :
                    skip;
            if (!skip)
                std::cout.put(c);
        }
    }
    char comment_char_;
};

class shell_comments_input_filter : public input_filter {
public:
    explicit shell_comments_input_filter(char comment_char = '#')
        : comment_char_(comment_char), skip_(false)
        { }

    template<typename Source>
    int get(Source& src)
    {
        int c;
        while (true) {
            if ((c = boost::iostreams::get(src)) == EOF || c == WOULD_BLOCK)
                break;
            skip_ = c == comment_char_ ?
                true :
                c == '\n' ?
                    false :
                    skip_;
            if (!skip_)
                break;
        }
        return c;
    }

    template<typename Source>
    void close(Source&) { skip_ = false; }
private:
    char comment_char_;
    bool skip_;
};

class shell_comments_output_filter : public output_filter {
public:
    explicit shell_comments_output_filter(char comment_char = '#')
        : comment_char_(comment_char), skip_(false)
        { }

    template<typename Sink>
    bool put(Sink& dest, int c)
    {
        skip_ = c == comment_char_ ?
            true :
            c == '\n' ?
                false :
                skip_;

        if (skip_)
            return true;

        return iostreams::put(dest, c);
    }

    template<typename Source>
    void close(Source&) { skip_ = false; }
private:
    char comment_char_;
    bool skip_;
};

class shell_comments_dual_use_filter : public dual_use_filter {
public:
    explicit shell_comments_dual_use_filter(char comment_char = '#')
        : comment_char_(comment_char), skip_(false)
        { }

    template<typename Source>
    int get(Source& src)
    {
        int c;
        while (true) {
            if ((c = boost::iostreams::get(src)) == EOF || c == WOULD_BLOCK)
                break;
            skip_ = c == comment_char_ ?
                true :
                c == '\n' ?
                    false :
                    skip_;
            if (!skip_)
                break;
        }
        return c;
    }

    template<typename Sink>
    bool put(Sink& dest, int c)
    {
        skip_ = c == comment_char_ ?
            true :
            c == '\n' ?
                false :
                skip_;

        if (skip_)
            return true;

        return iostreams::put(dest, c);
    }

    template<typename Device>
    void close(Device&, BOOST_IOS::openmode) { skip_ = false; }
private:
    char comment_char_;
    bool skip_;
};

class shell_comments_multichar_input_filter : public multichar_input_filter {
public:
    explicit shell_comments_multichar_input_filter(char comment_char = '#')
        : comment_char_(comment_char), skip_(false)
        { }

    template<typename Source>
    std::streamsize read(Source& src, char* s, std::streamsize n)
    {
        for (std::streamsize z = 0; z < n; ++z) {
            int c;
            while (true) {
                if ((c = boost::iostreams::get(src)) == EOF)
                    return z != 0 ? z : -1;
                else if (c == WOULD_BLOCK)
                    return z;
                skip_ = c == comment_char_ ?
                    true :
                    c == '\n' ?
                        false :
                        skip_;
                if (!skip_)
                    break;
            }
            s[z] = c;
        }
        return n;
    }

    template<typename Source>
    void close(Source&) { skip_ = false; }
private:
    char comment_char_;
    bool skip_;
};

class shell_comments_multichar_output_filter : public multichar_output_filter {
public:
    explicit shell_comments_multichar_output_filter(char comment_char = '#')
        : comment_char_(comment_char), skip_(false)
        { }

    template<typename Sink>
    std::streamsize write(Sink& dest, const char* s, std::streamsize n)
    {
        std::streamsize z;
        for (z = 0; z < n; ++z) {
            int c = s[z];
            skip_ = c == comment_char_ ?
                true :
                c == '\n' ?
                    false :
                    skip_;
            if (skip_)
                continue;
            if (!iostreams::put(dest, c))
                break;
        }
        return z;
    }

    template<typename Source>
    void close(Source&) { skip_ = false; }
private:
    char comment_char_;
    bool skip_;
};

} } }       // End namespaces example, iostreams, boost.

#endif // #ifndef BOOST_IOSTREAMS_SHELL_COMMENTS_FILTER_HPP_INCLUDED

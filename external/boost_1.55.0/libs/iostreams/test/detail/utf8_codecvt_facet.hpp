#ifndef BOOST_UTF8_CODECVT_FACET_HPP
#define BOOST_UTF8_CODECVT_FACET_HPP

#include <boost/iostreams/detail/config/wide_streams.hpp>
#ifdef BOOST_IOSTREAMS_NO_WIDE_STREAMS 
# error wide streams not supported on this platform
#endif

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// utf8_codecvt_facet.hpp

// Copyright (c) 2001 Ronald Garcia, Indiana University (garcia@osl.iu.edu)
// Andrew Lumsdaine, Indiana University (lums@osl.iu.edu).
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Note:(Robert Ramey).  I have made the following alterations in the original
// code.
// a) Rendered utf8_codecvt<wchar_t, char>  with using templates
// b) Move longer functions outside class definition to prevent inlining
// and make code smaller
// c) added on a derived class to permit translation to/from current
// locale to utf8

//  See http://www.boost.org for updates, documentation, and revision history.

// archives stored as text - note these ar templated on the basic
// stream templates to accommodate wide (and other?) kind of characters
//
// note the fact that on libraries without wide characters, ostream is
// is not a specialization of basic_ostream which in fact is not defined
// in such cases.   So we can't use basic_ostream<OStream::char_type> but rather
// use two template parameters
//
// utf8_codecvt_facet
//   This is an implementation of a std::codecvt facet for translating 
//   from UTF-8 externally to UCS-4.  Note that this is not tied to
//   any specific types in order to allow customization on platforms
//   where wchar_t is not big enough.
//
// NOTES:  The current implementation jumps through some unpleasant hoops in
// order to deal with signed character types.  As a std::codecvt_base::result,
// it is necessary  for the ExternType to be convertible to unsigned  char.
// I chose not to tie the extern_type explicitly to char. But if any combination
// of types other than <wchar_t,char_t> is used, then std::codecvt must be
// specialized on those types for this to work.

#include <locale>
#include <cstddef> // size_t
#include <cwchar>  // mbstate_t
#include <boost/integer_traits.hpp>
#include <boost/iostreams/detail/config/wide_streams.hpp>
#include <boost/iostreams/detail/codecvt_helper.hpp>

// maximum lenght of a multibyte string
#define MB_LENGTH_MAX 8

struct utf8_codecvt_facet_wchar_t 
    : public boost::iostreams::detail::codecvt_helper<wchar_t, char, std::mbstate_t>  
{
public:
    explicit utf8_codecvt_facet_wchar_t(std::size_t no_locale_manage = 0)
        : boost::iostreams::detail::codecvt_helper<wchar_t, char, std::mbstate_t>
              (no_locale_manage) 
        { }
protected:
    virtual std::codecvt_base::result do_in(
        std::mbstate_t& state, 
        const char * from,
        const char * from_end, 
        const char * & from_next,
        wchar_t * to, 
        wchar_t * to_end, 
        wchar_t*& to_next
    ) const;

    virtual std::codecvt_base::result do_out(
        std::mbstate_t & state, const wchar_t * from,
        const wchar_t * from_end, const wchar_t*  & from_next,
        char * to, char * to_end, char * & to_next
    ) const;

    bool invalid_continuing_octet(unsigned char octet_1) const {
        return (octet_1 < 0x80|| 0xbf< octet_1);
    }

    bool invalid_leading_octet(unsigned char octet_1)   const {
        return (0x7f < octet_1 && octet_1 < 0xc0) ||
            (octet_1 > 0xfd);
    }

    // continuing octets = octets except for the leading octet
    static unsigned int get_cont_octet_count(unsigned   char lead_octet) {
        return get_octet_count(lead_octet) - 1;
    }

    static unsigned int get_octet_count(unsigned char   lead_octet);

    // How many "continuing octets" will be needed for this word
    // ==   total octets - 1.
    int get_cont_octet_out_count(wchar_t word) const ;

    virtual bool do_always_noconv() const throw() { return false; }

    // UTF-8 isn't really stateful since we rewind on partial conversions
    virtual std::codecvt_base::result do_unshift(
        std::mbstate_t&,
        char * from,
        char * /* to */,
        char * & next
    ) const{
        next = from;
        return ok;
    }

    virtual int do_encoding() const throw() {
        const int variable_byte_external_encoding=0;
        return variable_byte_external_encoding;
    }

    // How many char objects can I process to get <= max_limit
    // wchar_t objects?
    virtual int do_length(
        BOOST_IOSTREAMS_CODECVT_CV_QUALIFIER std::mbstate_t &,
        const char * from,
        const char * from_end, 
        std::size_t max_limit
    ) const throw();

    // Largest possible value do_length(state,from,from_end,1) could return.
    virtual int do_max_length() const throw () {
        return 6; // largest UTF-8 encoding of a UCS-4 character
    }
};

#if 0 // not used - incorrect in any case
// Robert Ramey - use the above to make a code converter from multi-byte
// char strings to utf8 encoding
struct utf8_codecvt_facet_char : public utf8_codecvt_facet_wchar_t
{
    typedef utf8_codecvt_facet_wchar_t base_class;
public:
    explicit utf8_codecvt_facet_char(std::size_t no_locale_manage=0)
        : base_class(no_locale_manage)
    {}
protected:
    virtual std::codecvt_base::result do_in(
        std::mbstate_t & state, 
        const char * from, 
        const char * from_end, 
        const char * & from_next,
        char * to, 
        char * to_end, 
        char * & to_next
    ) const;

    virtual std::codecvt_base::result do_out(
        std::mbstate_t & state, 
        const char * from,
        const char * from_end, 
        const char*  & from_next,
        char * to, 
        char * to_end, 
        char * & to_next
    ) const;

    // How many char objects can I process to get <= max_limit
    // char objects?
    virtual int do_length(
        const std::mbstate_t&, 
        const char * from,
        const char * from_end, 
        std::size_t max_limit
    ) const;
};
#endif

template<class Internal, class External>
struct utf8_codecvt_facet
{};

template<>
struct utf8_codecvt_facet<wchar_t, char>
    : public utf8_codecvt_facet_wchar_t
{};

#if 0
template<>
struct utf8_codecvt_facet<char, char>
    : public utf8_codecvt_facet_char
{};
#endif

#endif // BOOST_UTF8_CODECVT_FACET_HPP


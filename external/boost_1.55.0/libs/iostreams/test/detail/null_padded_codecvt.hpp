// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// Contains the definitions of two codecvt facets useful for testing code 
// conversion. Both represent the "null padded" character encoding described as 
// follows. A wide character can be represented by the encoding if its value V
// is within the range of an unsigned char. The first char of the sequence
// representing V is V % 3 + 1. This is followed by V % 3 null characters, and
// finally by V itself.

// The first codecvt facet, null_padded_codecvt, is statefull, with state_type
// equal to int.

// The second codecvt facet, stateless_null_padded_codecvt, is stateless. At 
// each point in a conversion, no characters are consumed unless there is room 
// in the output sequence to write an entire multibyte sequence.

#ifndef BOOST_IOSTREAMS_TEST_NULL_PADDED_CODECVT_HPP_INCLUDED
#define BOOST_IOSTREAMS_TEST_NULL_PADDED_CODECVT_HPP_INCLUDED

#include <boost/config.hpp>                          // NO_STDC_NAMESPACE
#include <boost/iostreams/detail/codecvt_helper.hpp>
#include <boost/iostreams/detail/config/wide_streams.hpp>
#include <cstddef>                                   // mbstate_t.
#include <locale>                                    // codecvt.
#include <boost/integer_traits.hpp>                  // const_max.

#ifdef BOOST_NO_STDC_NAMESPACE
    namespace std { using ::mbstate_t; }
#endif

namespace boost { namespace iostreams { namespace test {

//------------------Definition of null_padded_codecvt_state-------------------//   

class null_padded_codecvt_state {
public:
    null_padded_codecvt_state(int val = 0) : val_(val) { }
    operator int() const { return val_; }
    int& val() { return val_; }
    const int& val() const { return val_; }
private:
    int val_;
};

} } }

BOOST_IOSTREAMS_CODECVT_SPEC(boost::iostreams::test::null_padded_codecvt_state)

namespace boost { namespace iostreams { namespace test {

//------------------Definition of null_padded_codevt--------------------------//            

//
// state is initially 0. After a single character is consumed, state is set to 
// the number of characters in the current multibyte sequence and decremented
// as each character is consumed until its value reaches 0 again.
//
class null_padded_codecvt
    : public iostreams::detail::codecvt_helper<
                 wchar_t, char, null_padded_codecvt_state
             >
{
public:
    typedef null_padded_codecvt_state state_type;
private:
    std::codecvt_base::result 
    do_in( state_type& state, const char* first1, const char* last1, 
           const char*& next1, wchar_t* first2, wchar_t* last2, 
           wchar_t*& next2 ) const
    {
        using namespace std;
        if (state < 0 || state > 3)
            return codecvt_base::error;
        next1 = first1;
        next2 = first2;
        while (next2 != last2 && next1 != last1) {
            while (next1 != last1) {
                if (state == 0) {
                    if (*next1 < 1 || *next1 > 3)
                        return codecvt_base::error;
                    state = *next1++;
                } else if (state == 1) {
                    *next2++ = (unsigned char) *next1++;
                    state = 0;
                    break;
                } else {
                    if (*next1++ != 0)
                        return codecvt_base::error;
                    --state.val();
                }
            }
        }
        return next2 == last2 ? 
            codecvt_base::ok : 
            codecvt_base::partial;
    }

    std::codecvt_base::result 
    do_out( state_type& state, const wchar_t* first1, const wchar_t* last1,
            const wchar_t*& next1, char* first2, char* last2, 
            char*& next2 ) const
    {
        using namespace std;
        if (state < 0 || state > 3)
            return codecvt_base::error;
        next1 = first1;
        next2 = first2;
        while (next1 != last1 && next2 != last2) {
            while (next2 != last2) {
                if (state == 0) {
                    if (*next1 > integer_traits<unsigned char>::const_max)
                        return codecvt_base::noconv;
                    state = *next1 % 3 + 1;
                    *next2++ = static_cast<char>(state);
                } else if (state == 1) {
                    state = 0;
                    *next2++ = static_cast<unsigned char>(*next1++);
                    break;
                } else {
                    --state.val();
                    *next2++ = 0;
                }
            }
        }
        return next1 == last1 ? 
            codecvt_base::ok : 
            codecvt_base::partial;
    }

    std::codecvt_base::result 
    do_unshift( state_type& state, 
                char* /* first2 */, 
                char* last2, 
                char*& next2 ) const
    {
        using namespace std;
        next2 = last2;
        while (state.val()-- > 0)
            if (next2 != last2)
                *next2++ = 0;
            else
                return codecvt_base::partial;
        return codecvt_base::ok;
    }

    bool do_always_noconv() const throw() { return false; }

    int do_max_length() const throw() { return 4; }

    int do_encoding() const throw() { return -1; }

    int do_length( BOOST_IOSTREAMS_CODECVT_CV_QUALIFIER state_type& state, 
                   const char* first1, const char* last1, 
                   std::size_t len2 ) const throw()
    {   // Implementation should follow that of do_in().
        int st = state;
        std::size_t result = 0;
        const char* next1 = first1;
        while (result < len2 && next1 != last1) {
            while (next1 != last1) {
                if (st == 0) {
                    if (*next1 < 1 || *next1 > 3)
                        return static_cast<int>(result); // error.
                    st = *next1++;
                } else if (st == 1) {
                    ++result;
                    st = 0;
                    break;
                } else {
                    if (*next1++ != 0)
                        return static_cast<int>(result); // error.
                    --st;
                }
            }
        }
        return static_cast<int>(result);
    }
};

//------------------Definition of stateless_null_padded_codevt----------------//

class stateless_null_padded_codecvt
    : public std::codecvt<wchar_t, char, std::mbstate_t> 
{
    std::codecvt_base::result 
    do_in( state_type&, const char* first1, const char* last1, 
           const char*& next1, wchar_t* first2, wchar_t* last2, 
           wchar_t*& next2 ) const
    {
        using namespace std;
        for ( next1 = first1, next2 = first2;
              next1 != last1 && next2 != last2; )
        {
            int len = (unsigned char) *next1;
            if (len < 1 || len > 3)
                return codecvt_base::error;
            if (last1 - next1 < len + 1)
                return codecvt_base::partial;
            ++next1;
            while (len-- > 1)
                if (*next1++ != 0)
                    return codecvt_base::error;
            *next2++ = (unsigned char) *next1++;
        }
        return next1 == last1 && next2 == last2 ? 
            codecvt_base::ok : 
            codecvt_base::partial;
    }

    std::codecvt_base::result 
    do_out( state_type&, const wchar_t* first1, const wchar_t* last1, 
            const wchar_t*& next1, char* first2, char* last2, 
            char*& next2 ) const
    {
        using namespace std;
        for ( next1 = first1, next2 = first2; 
              next1 != last1 && next2 != last2; ) 
        {
            if (*next1 > integer_traits<unsigned char>::const_max)
                return codecvt_base::noconv;
            int skip = *next1 % 3 + 2;
            if (last2 - next2 < skip)
                return codecvt_base::partial;
            *next2++ = static_cast<char>(--skip);
            while (skip-- > 1)
                *next2++ = 0;
            *next2++ = (unsigned char) *next1++;
        }
        return codecvt_base::ok;
    }

    std::codecvt_base::result 
    do_unshift( state_type&, 
                char* /* first2 */, 
                char* /* last2 */, 
                char*& /* next2 */ ) const
    {  
        return std::codecvt_base::ok;
    }

    bool do_always_noconv() const throw() { return false; }

    int do_max_length() const throw() { return 4; }

    int do_encoding() const throw() { return -1; }

    int do_length( BOOST_IOSTREAMS_CODECVT_CV_QUALIFIER state_type&, 
                   const char* first1, const char* last1,
                   std::size_t len2 ) const throw()
    {   // Implementation should follow that of do_in().
        std::size_t result = 0;
        for ( const char* next1 = first1;
              next1 != last1 && result < len2; ++result)
        {
            int len = (unsigned char) *next1;
            if (len < 1 || len > 3 || last1 - next1 < len + 1)
                return static_cast<int>(result); // error.
            ++next1;
            while (len-- > 1)
                if (*next1++ != 0)
                    return static_cast<int>(result); // error.
            ++next1;
        }
        return static_cast<int>(result);
    } 
};

} } } // End namespaces detail, iostreams, boost.

#endif // #ifndef BOOST_IOSTREAMS_TEST_NULL_PADDED_CODECVT_HPP_INCLUDED

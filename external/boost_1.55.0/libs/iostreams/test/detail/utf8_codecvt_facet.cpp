/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// utf8_codecvt_facet.cpp

// Copyright (c) 2001 Ronald Garcia, Indiana University (garcia@osl.iu.edu)
// Andrew Lumsdaine, Indiana University (lums@osl.iu.edu). 
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/iostreams for documentation.

//#include <cstdlib> // for multi-byte converson routines

// Jonathan Turkanis: 
//   - Replaced test for BOOST_NO_STD_WSTREAMBUF with test for 
//     BOOST_IOSTREAMS_NO_WIDE_STREAMS;
//   - Derived from codecvt_helper instead of codecvt.

#include <boost/config.hpp>
#include <boost/iostreams/detail/config/wide_streams.hpp>
#ifdef BOOST_IOSTREAMS_NO_LOCALES
# error "C++ locales not supported on this platform"
#else

#include <cassert>
#include <cstddef>

#include <boost/detail/workaround.hpp>
#include "./utf8_codecvt_facet.hpp"

#if BOOST_WORKAROUND(__BORLANDC__, <= 0x600)
# pragma warn -sig // Conversion may lose significant digits
# pragma warn -rng // Constant is out of range in comparison
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// implementation for wchar_t

// Translate incoming UTF-8 into UCS-4
std::codecvt_base::result utf8_codecvt_facet_wchar_t::do_in(
    std::mbstate_t&, 
    const char * from,
    const char * from_end, 
    const char * & from_next,
    wchar_t * to, 
    wchar_t * to_end, 
    wchar_t * & to_next
) const {
    // Basic algorithm:  The first octet determines how many
    // octets total make up the UCS-4 character.  The remaining
    // "continuing octets" all begin with "10". To convert, subtract
    // the amount that specifies the number of octets from the first
    // octet.  Subtract 0x80 (1000 0000) from each continuing octet,
    // then mash the whole lot together.  Note that each continuing
    // octet only uses 6 bits as unique values, so only shift by
    // multiples of 6 to combine.
    while (from != from_end && to != to_end) {

        // Error checking   on the first octet
        if (invalid_leading_octet(*from)){
            from_next = from;
            to_next = to;
            return std::codecvt_base::error;
        }

        // The first octet is   adjusted by a value dependent upon 
        // the number   of "continuing octets" encoding the character
        const   int cont_octet_count = get_cont_octet_count(*from);
        const   wchar_t octet1_modifier_table[] =   {
            0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc
        };

        // The unsigned char conversion is necessary in case char is
        // signed   (I learned this the hard way)
        wchar_t ucs_result = 
            (unsigned char)(*from++) - octet1_modifier_table[cont_octet_count];

        // Invariants   : 
        //   1) At the start of the loop,   'i' continuing characters have been
        //    processed 
        //   2) *from   points to the next continuing character to be processed.
        int i   = 0;
        while(i != cont_octet_count && from != from_end) {

            // Error checking on continuing characters
            if (invalid_continuing_octet(*from)) {
                from_next   = from;
                to_next =   to;
                return std::codecvt_base::error;
            }

            ucs_result *= (1 << 6); 

            // each continuing character has an extra (10xxxxxx)b attached to 
            // it that must be removed.
            ucs_result += (unsigned char)(*from++) - 0x80;
            ++i;
        }

        // If   the buffer ends with an incomplete unicode character...
        if (from == from_end && i   != cont_octet_count) {
            // rewind "from" to before the current character translation
            from_next = from - (i+1); 
            to_next = to;
            return std::codecvt_base::partial;
        }
        *to++   = ucs_result;
    }
    from_next = from;
    to_next = to;

    // Were we done converting or did we run out of destination space?
    if(from == from_end) return std::codecvt_base::ok;
    else return std::codecvt_base::partial;
}

std::codecvt_base::result utf8_codecvt_facet_wchar_t::do_out(
    std::mbstate_t &, 
    const wchar_t *   from,
    const wchar_t * from_end, 
    const wchar_t * & from_next,
    char * to, 
    char * to_end, 
    char * & to_next
) const
{
    // RG - consider merging this table with the other one
    const wchar_t octet1_modifier_table[] = {
        0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc
    };

    while (from != from_end && to != to_end) {

#define BOOST_NULL // Prevent macro expansion
        // Check for invalid UCS-4 character
        if (*from  > std::numeric_limits<wchar_t>::max BOOST_NULL ()) {
            from_next = from;
            to_next = to;
            return std::codecvt_base::error;
        }
#undef BOOST_NULL

        int cont_octet_count = get_cont_octet_out_count(*from);

        // RG  - comment this formula better
        int shift_exponent = (cont_octet_count) *   6;

        // Process the first character
        *to++ = octet1_modifier_table[cont_octet_count] +
            (unsigned char)(*from / (1 << shift_exponent));

        // Process the continuation characters 
        // Invariants: At   the start of the loop:
        //   1) 'i' continuing octets   have been generated
        //   2) '*to'   points to the next location to place an octet
        //   3) shift_exponent is   6 more than needed for the next octet
        int i   = 0;
        while   (i != cont_octet_count && to != to_end) {
            shift_exponent -= 6;
            *to++ = 0x80 + ((*from / (1 << shift_exponent)) % (1 << 6));
            ++i;
        }
        // If   we filled up the out buffer before encoding the character
        if(to   == to_end && i != cont_octet_count) {
            from_next = from;
            to_next = to - (i+1);
            return std::codecvt_base::partial;
        }
        *from++;
    }
    from_next = from;
    to_next = to;
    // Were we done or did we run out of destination space
    if(from == from_end) return std::codecvt_base::ok;
    else return std::codecvt_base::partial;
}

// How many char objects can I process to get <= max_limit
// wchar_t objects?
int utf8_codecvt_facet_wchar_t::do_length(
    BOOST_IOSTREAMS_CODECVT_CV_QUALIFIER std::mbstate_t &,
    const char * from,
    const char * from_end, 
    std::size_t max_limit
) const throw()
{ 
    // RG - this code is confusing!  I need a better way to express it.
    // and test cases.

    // Invariants:
    // 1) last_octet_count has the size of the last measured character
    // 2) char_count holds the number of characters shown to fit
    // within the bounds so far (no greater than max_limit)
    // 3) from_next points to the octet 'last_octet_count' before the
    // last measured character.  
    int last_octet_count=0;
    std::size_t char_count = 0;
    const char* from_next = from;
    // Use "<" because the buffer may represent incomplete characters
    while (from_next+last_octet_count <= from_end && char_count <= max_limit) {
        from_next += last_octet_count;
        last_octet_count = (get_octet_count(*from_next));
        ++char_count;
    }
    return from_next-from_end;
}

unsigned int utf8_codecvt_facet_wchar_t::get_octet_count(
    unsigned char   lead_octet
){
    // if the 0-bit (MSB) is 0, then 1 character
    if (lead_octet <= 0x7f) return 1;

    // Otherwise the count number of consecutive 1 bits starting at MSB
    assert(0xc0 <= lead_octet && lead_octet <= 0xfd);

    if (0xc0 <= lead_octet && lead_octet <= 0xdf) return 2;
    else if (0xe0 <= lead_octet && lead_octet <= 0xef) return 3;
    else if (0xf0 <= lead_octet && lead_octet <= 0xf7) return 4;
    else if (0xf8 <= lead_octet && lead_octet <= 0xfb) return 5;
    else return 6;
}

namespace {
template<std::size_t s>
int get_cont_octet_out_count_impl(wchar_t word){
    if (word < 0x80) {
        return 0;
    }
    if (word < 0x800) {
        return 1;
    }
    return 2;
}

// note the following code will generate on some platforms where
// wchar_t is defined as UCS2.  The warnings are superfluous as
// the specialization is never instantitiated with such compilers.
template<>
int get_cont_octet_out_count_impl<4>(wchar_t word)
{
    if (word < 0x80) {
        return 0;
    }
    if (word < 0x800) {
        return 1;
    }
    if (word < 0x10000) {
        return 2;
    }
    if (word < 0x200000) {
        return 3;
    }
    if (word < 0x4000000) {
        return 4;
    }
    return 5;
}

} // namespace anonymous

// How many "continuing octets" will be needed for this word
// ==   total octets - 1.
int utf8_codecvt_facet_wchar_t::get_cont_octet_out_count(
    wchar_t word
) const {
    return get_cont_octet_out_count_impl<sizeof(wchar_t)>(word);
}

#if 0 // not used?
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// implementation for char

std::codecvt_base::result utf8_codecvt_facet_char::do_in(
    std::mbstate_t & state, 
    const char * from, 
    const char * from_end, 
    const char * & from_next,
    char * to, 
    char * to_end, 
    char * & to_next
) const
{
    while(from_next < from_end){
        wchar_t w;
        wchar_t *wnext = & w;
        utf8_codecvt_facet_wchar_t::result ucs4_result;
        ucs4_result = base_class::do_in(
            state,
            from, from_end, from_next,
            wnext, wnext + 1, wnext
        );
        if(codecvt_base::ok != ucs4_result)
            return ucs4_result;
        // if the conversion succeeds. 
        int length = std::wctomb(to_next, w);
        assert(-1 != length);
        to_next += length;
    }
    return codecvt_base::ok;
}

std::codecvt_base::result utf8_codecvt_facet_char::do_out(
    mbstate_t & state, 
    const char * from,
    const char * from_end, 
    const char * & from_next,
    char * to, 
    char * to_end, 
    char * & to_next
) const
{
    while(from_next < from_end){
        wchar_t w;
        int result = std::mbtowc(&w, from_next,  MB_LENGTH_MAX);
        assert(-1 != result);
        from_next += result;
        utf8_codecvt_facet_wchar_t::result ucs4_result;

        const wchar_t *wptr = & w;
        ucs4_result = base_class::do_out(
            state,
            wptr, wptr+1, wptr,
            to_next, to_end, to_next
        );
        if(codecvt_base::ok != ucs4_result)
            return ucs4_result;     
    }
    return codecvt_base::ok;
}

// How many bytes objects can I process to get <= max_limit
// char objects?
int utf8_codecvt_facet_char::do_length(
    // it seems that the standard doesn't use const so these librarires
    // would be in error
    BOOST_IOSTREAMS_CODECVT_CV_QUALIFIER
    utf8_codecvt_facet_wchar_t::mbstate_t & initial_state,
    const char * from_next,
    const char * from_end, 
    std::size_t max_limit
) const
{
    int total_length = 0;
    const char *from = from_next;
    mbstate_t state = initial_state;
    while(from_next < from_end){
        wchar_t w;
        wchar_t *wnext = & w;
        utf8_codecvt_facet_wchar_t::result ucs4_result;
        ucs4_result = base_class::do_in(
            state,
            from_next, from_end, from_next,
            wnext, wnext + 1, wnext
        );

        if(codecvt_base::ok != ucs4_result)
            break;

        char carray[MB_LENGTH_MAX];
        std::size_t count = wctomb(carray, w);
        if(count > max_limit)
            break;

        max_limit -= count;
        total_length = from_next - from;
    }
    return total_length;
}
#endif

#endif //BOOST_IOSTREAMS_NO_WIDE_STREAMS

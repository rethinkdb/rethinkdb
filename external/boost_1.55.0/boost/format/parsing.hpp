// ----------------------------------------------------------------------------
// parsing.hpp :  implementation of the parsing member functions
//                      ( parse, parse_printf_directive)
// ----------------------------------------------------------------------------

//  Copyright Samuel Krempp 2003. Use, modification, and distribution are
//  subject to the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// see http://www.boost.org/libs/format for library home page

// ----------------------------------------------------------------------------

#ifndef BOOST_FORMAT_PARSING_HPP
#define BOOST_FORMAT_PARSING_HPP


#include <boost/format/format_class.hpp>
#include <boost/format/exceptions.hpp>
#include <boost/throw_exception.hpp>
#include <boost/assert.hpp>


namespace boost {
namespace io {
namespace detail {

#if defined(BOOST_NO_STD_LOCALE)
    // streams will be used for narrow / widen. but these methods are not const
    template<class T>
    T& const_or_not(const T& x) { 
        return const_cast<T&> (x);
    }
#else
    template<class T>
    const T& const_or_not(const T& x) { 
        return x;
    }
#endif

    template<class Ch, class Facet> inline
    char wrap_narrow(const Facet& fac, Ch c, char deflt) {
        return const_or_not(fac).narrow(c, deflt);
    }

    template<class Ch, class Facet> inline
    bool wrap_isdigit(const Facet& fac, Ch c) {
#if ! defined( BOOST_NO_LOCALE_ISDIGIT )
        return fac.is(std::ctype<Ch>::digit, c);
# else
        (void) fac;     // remove "unused parameter" warning
        using namespace std;
        return isdigit(c); 
#endif 
    }
 
    template<class Iter, class Facet> 
    Iter wrap_scan_notdigit(const Facet & fac, Iter beg, Iter end) {
        using namespace std;
        for( ; beg!=end && wrap_isdigit(fac, *beg); ++beg) ;
        return beg;
    }


    // Input : [start, last) iterators range and a
    //          a Facet to use its widen/narrow member function
    // Effects : read sequence and convert digits into integral n, of type Res
    // Returns : n
    template<class Res, class Iter, class Facet>
    Iter str2int (const Iter & start, const Iter & last, Res & res, 
                 const Facet& fac) 
    {
        using namespace std;
        Iter it;
        res=0;
        for(it=start; it != last && wrap_isdigit(fac, *it); ++it ) {
            char cur_ch = wrap_narrow(fac, *it, 0); // cant fail.
            res *= 10;
            res += cur_ch - '0'; // 22.2.1.1.2.13 of the C++ standard
        }
        return it;
    }

    // skip printf's "asterisk-fields" directives in the format-string buf
    // Input : char string, with starting index *pos_p
    //         a Facet merely to use its widen/narrow member function
    // Effects : advance *pos_p by skipping printf's asterisk fields.
    // Returns : nothing
    template<class Iter, class Facet>
    Iter skip_asterisk(Iter start, Iter last, const Facet& fac) 
    {
        using namespace std;
        ++ start;
        start = wrap_scan_notdigit(fac, start, last);
        if(start!=last && *start== const_or_not(fac).widen( '$') )
            ++start;
        return start;
    }


    // auxiliary func called by parse_printf_directive
    // for centralising error handling
    // it either throws if user sets the corresponding flag, or does nothing.
    inline void maybe_throw_exception(unsigned char exceptions, 
                                      std::size_t pos, std::size_t size)
    {
        if(exceptions & io::bad_format_string_bit)
            boost::throw_exception(io::bad_format_string(pos, size) );
    }
    

    // Input: the position of a printf-directive in the format-string
    //    a basic_ios& merely to use its widen/narrow member function
    //    a bitset'exceptions' telling whether to throw exceptions on errors.
    // Returns:
    //  true if parse succeeded (ignore some errors if exceptions disabled)
    //  false if it failed so bad that the directive should be printed verbatim
    // Effects:
    //  start is incremented so that *start is the first char after
    //     this directive
    //  *fpar is set with the parameters read in the directive
    template<class Ch, class Tr, class Alloc, class Iter, class Facet>
    bool parse_printf_directive(Iter & start, const Iter& last, 
                                detail::format_item<Ch, Tr, Alloc> * fpar,
                                const Facet& fac,
                                std::size_t offset, unsigned char exceptions)
    {
        typedef typename basic_format<Ch, Tr, Alloc>::format_item_t format_item_t;

        fpar->argN_ = format_item_t::argN_no_posit;  // if no positional-directive
        bool precision_set = false;
        bool in_brackets=false;
        Iter start0 = start;
        std::size_t fstring_size = last-start0+offset;

        if(start>= last) { // empty directive : this is a trailing %
                maybe_throw_exception(exceptions, start-start0 + offset, fstring_size);
                return false;
        }          
          
        if(*start== const_or_not(fac).widen( '|')) {
            in_brackets=true;
            if( ++start >= last ) {
                maybe_throw_exception(exceptions, start-start0 + offset, fstring_size);
                return false;
            }
        }

        // the flag '0' would be picked as a digit for argument order, but here it's a flag :
        if(*start== const_or_not(fac).widen( '0')) 
            goto parse_flags;

        // handle argument order (%2$d)  or possibly width specification: %2d
        if(wrap_isdigit(fac, *start)) {
            int n;
            start = str2int(start, last, n, fac);
            if( start >= last ) {
                maybe_throw_exception(exceptions, start-start0+offset, fstring_size);
                return false;
            }
            
            // %N% case : this is already the end of the directive
            if( *start ==  const_or_not(fac).widen( '%') ) {
                fpar->argN_ = n-1;
                ++start;
                if( in_brackets) 
                    maybe_throw_exception(exceptions, start-start0+offset, fstring_size); 
                // but don't return.  maybe "%" was used in lieu of '$', so we go on.
                else
                    return true;
            }

            if ( *start== const_or_not(fac).widen( '$') ) {
                fpar->argN_ = n-1;
                ++start;
            } 
            else {
                // non-positionnal directive
                fpar->fmtstate_.width_ = n;
                fpar->argN_  = format_item_t::argN_no_posit;
                goto parse_precision;
            }
        }
    
      parse_flags: 
        // handle flags
        while ( start != last) { // as long as char is one of + - = _ # 0 l h   or ' '
            // misc switches
            switch ( wrap_narrow(fac, *start, 0)) {
            case '\'' : break; // no effect yet. (painful to implement)
            case 'l':
            case 'h':  // short/long modifier : for printf-comaptibility (no action needed)
                break;
            case '-':
                fpar->fmtstate_.flags_ |= std::ios_base::left;
                break;
            case '=':
                fpar->pad_scheme_ |= format_item_t::centered;
                break;
            case '_':
                fpar->fmtstate_.flags_ |= std::ios_base::internal;
                break;
            case ' ':
                fpar->pad_scheme_ |= format_item_t::spacepad;
                break;
            case '+':
                fpar->fmtstate_.flags_ |= std::ios_base::showpos;
                break;
            case '0':
                fpar->pad_scheme_ |= format_item_t::zeropad;
                // need to know alignment before really setting flags,
                // so just add 'zeropad' flag for now, it will be processed later.
                break;
            case '#':
                fpar->fmtstate_.flags_ |= std::ios_base::showpoint | std::ios_base::showbase;
                break;
            default:
                goto parse_width;
            }
            ++start;
        } // loop on flag.

        if( start>=last) {
            maybe_throw_exception(exceptions, start-start0+offset, fstring_size);
            return true; 
        }
      parse_width:
        // handle width spec
        // first skip 'asterisk fields' :  *, or *N$
        if(*start == const_or_not(fac).widen( '*') )
            start = skip_asterisk(start, last, fac); 
        if(start!=last && wrap_isdigit(fac, *start))
            start = str2int(start, last, fpar->fmtstate_.width_, fac);

      parse_precision:
        if( start>= last) { 
            maybe_throw_exception(exceptions, start-start0+offset, fstring_size);
            return true;
        }
        // handle precision spec
        if (*start== const_or_not(fac).widen( '.')) {
            ++start;
            if(start != last && *start == const_or_not(fac).widen( '*') )
                start = skip_asterisk(start, last, fac); 
            if(start != last && wrap_isdigit(fac, *start)) {
                start = str2int(start, last, fpar->fmtstate_.precision_, fac);
                precision_set = true;
            }
            else
                fpar->fmtstate_.precision_ =0;
        }
    
        // handle  formatting-type flags :
        while( start != last && ( *start== const_or_not(fac).widen( 'l') 
                                  || *start== const_or_not(fac).widen( 'L') 
                                  || *start== const_or_not(fac).widen( 'h')) )
            ++start;
        if( start>=last) {
            maybe_throw_exception(exceptions, start-start0+offset, fstring_size);
            return true;
        }

        if( in_brackets && *start== const_or_not(fac).widen( '|') ) {
            ++start;
            return true;
        }
        switch ( wrap_narrow(fac, *start, 0) ) {
        case 'X':
            fpar->fmtstate_.flags_ |= std::ios_base::uppercase;
        case 'p': // pointer => set hex.
        case 'x':
            fpar->fmtstate_.flags_ &= ~std::ios_base::basefield;
            fpar->fmtstate_.flags_ |= std::ios_base::hex;
            break;

        case 'o':
            fpar->fmtstate_.flags_ &= ~std::ios_base::basefield;
            fpar->fmtstate_.flags_ |=  std::ios_base::oct;
            break;

        case 'E':
            fpar->fmtstate_.flags_ |=  std::ios_base::uppercase;
        case 'e':
            fpar->fmtstate_.flags_ &= ~std::ios_base::floatfield;
            fpar->fmtstate_.flags_ |=  std::ios_base::scientific;

            fpar->fmtstate_.flags_ &= ~std::ios_base::basefield;
            fpar->fmtstate_.flags_ |=  std::ios_base::dec;
            break;
      
        case 'f':
            fpar->fmtstate_.flags_ &= ~std::ios_base::floatfield;
            fpar->fmtstate_.flags_ |=  std::ios_base::fixed;
        case 'u':
        case 'd':
        case 'i':
            fpar->fmtstate_.flags_ &= ~std::ios_base::basefield;
            fpar->fmtstate_.flags_ |=  std::ios_base::dec;
            break;

        case 'T':
            ++start;
            if( start >= last)
                maybe_throw_exception(exceptions, start-start0+offset, fstring_size);
            else
                fpar->fmtstate_.fill_ = *start;
            fpar->pad_scheme_ |= format_item_t::tabulation;
            fpar->argN_ = format_item_t::argN_tabulation; 
            break;
        case 't': 
            fpar->fmtstate_.fill_ = const_or_not(fac).widen( ' ');
            fpar->pad_scheme_ |= format_item_t::tabulation;
            fpar->argN_ = format_item_t::argN_tabulation; 
            break;

        case 'G':
            fpar->fmtstate_.flags_ |= std::ios_base::uppercase;
            break;
        case 'g': // 'g' conversion is default for floats.
            fpar->fmtstate_.flags_ &= ~std::ios_base::basefield;
            fpar->fmtstate_.flags_ |=  std::ios_base::dec;

            // CLEAR all floatield flags, so stream will CHOOSE
            fpar->fmtstate_.flags_ &= ~std::ios_base::floatfield; 
            break;

        case 'C':
        case 'c': 
            fpar->truncate_ = 1;
            break;
        case 'S':
        case 's': 
            if(precision_set) // handle truncation manually, with own parameter.
                fpar->truncate_ = fpar->fmtstate_.precision_;
            fpar->fmtstate_.precision_ = 6; // default stream precision.
            break;
        case 'n' :  
            fpar->argN_ = format_item_t::argN_ignored;
            break;
        default: 
            maybe_throw_exception(exceptions, start-start0+offset, fstring_size);
        }
        ++start;

        if( in_brackets ) {
            if( start != last && *start== const_or_not(fac).widen( '|') ) {
                ++start;
                return true;
            }
            else  maybe_throw_exception(exceptions, start-start0+offset, fstring_size);
        }
        return true;
    }
    // -end parse_printf_directive()

    template<class String, class Facet>
    int upper_bound_from_fstring(const String& buf, 
                                 const typename String::value_type arg_mark,
                                 const Facet& fac, 
                                 unsigned char exceptions) 
    {
        // quick-parsing of the format-string to count arguments mark (arg_mark, '%')
        // returns : upper bound on the number of format items in the format strings
        using namespace boost::io;
        typename String::size_type i1=0;
        int num_items=0;
        while( (i1=buf.find(arg_mark,i1)) != String::npos ) {
            if( i1+1 >= buf.size() ) {
                if(exceptions & bad_format_string_bit)
                    boost::throw_exception(bad_format_string(i1, buf.size() )); // must not end in ".. %"
                else {
                  ++num_items;
                  break;
                }
            }
            if(buf[i1+1] == buf[i1] ) {// escaped "%%"
                i1+=2; continue; 
            }

            ++i1;
            // in case of %N% directives, dont count it double (wastes allocations..) :
            i1 = detail::wrap_scan_notdigit(fac, buf.begin()+i1, buf.end()) - buf.begin();
            if( i1 < buf.size() && buf[i1] == arg_mark )
                ++i1;
            ++num_items;
        }
        return num_items;
    }
    template<class String> inline
    void append_string(String& dst, const String& src, 
                       const typename String::size_type beg, 
                       const typename String::size_type end) {
#if !defined(BOOST_NO_STRING_APPEND)
        dst.append(src.begin()+beg, src.begin()+end);
#else
        dst += src.substr(beg, end-beg);
#endif
    }

} // detail namespace
} // io namespace



// -----------------------------------------------
//  format :: parse(..)

    template<class Ch, class Tr, class Alloc>
    basic_format<Ch, Tr, Alloc>& basic_format<Ch, Tr, Alloc>:: 
    parse (const string_type& buf) {
        // parse the format-string 
        using namespace std;
#if !defined(BOOST_NO_STD_LOCALE)
        const std::ctype<Ch> & fac = BOOST_USE_FACET( std::ctype<Ch>, getloc());
#else
        io::basic_oaltstringstream<Ch, Tr, Alloc> fac; 
        //has widen and narrow even on compilers without locale
#endif

        const Ch arg_mark = io::detail::const_or_not(fac).widen( '%');
        bool ordered_args=true; 
        int max_argN=-1;

        // A: find upper_bound on num_items and allocates arrays
        int num_items = io::detail::upper_bound_from_fstring(buf, arg_mark, fac, exceptions());
        make_or_reuse_data(num_items);

        // B: Now the real parsing of the format string :
        num_items=0;
        typename string_type::size_type i0=0, i1=0;
        typename string_type::const_iterator it;
        bool special_things=false;
        int cur_item=0;
        while( (i1=buf.find(arg_mark,i1)) != string_type::npos ) {
            string_type & piece = (cur_item==0) ? prefix_ : items_[cur_item-1].appendix_;
            if( buf[i1+1] == buf[i1] ) { // escaped mark, '%%' 
                io::detail::append_string(piece, buf, i0, i1+1);
                i1+=2; i0=i1;
                continue; 
            }
            BOOST_ASSERT(  static_cast<unsigned int>(cur_item) < items_.size() || cur_item==0);

            if(i1!=i0) {
                io::detail::append_string(piece, buf, i0, i1);
                i0=i1;
            }
            ++i1;
            it = buf.begin()+i1;
            bool parse_ok = io::detail::parse_printf_directive(
                it, buf.end(), &items_[cur_item], fac, i1, exceptions());
            i1 = it - buf.begin();
            if( ! parse_ok ) // the directive will be printed verbatim
                continue; 
            i0=i1;
            items_[cur_item].compute_states(); // process complex options, like zeropad, into params

            int argN=items_[cur_item].argN_;
            if(argN == format_item_t::argN_ignored)
                continue;
            if(argN ==format_item_t::argN_no_posit)
                ordered_args=false;
            else if(argN == format_item_t::argN_tabulation) special_things=true;
            else if(argN > max_argN) max_argN = argN;
            ++num_items;
            ++cur_item;
        } // loop on %'s
        BOOST_ASSERT(cur_item == num_items);
    
        // store the final piece of string
        {
            string_type & piece = (cur_item==0) ? prefix_ : items_[cur_item-1].appendix_;
            io::detail::append_string(piece, buf, i0, buf.size());
        }
    
        if( !ordered_args) {
            if(max_argN >= 0 ) {  // dont mix positional with non-positionnal directives
                if(exceptions() & io::bad_format_string_bit)
                    boost::throw_exception(io::bad_format_string(max_argN, 0));
                // else do nothing. => positionnal arguments are processed as non-positionnal
            }
            // set things like it would have been with positional directives :
            int non_ordered_items = 0;
            for(int i=0; i< num_items; ++i)
                if(items_[i].argN_ == format_item_t::argN_no_posit) {
                    items_[i].argN_ = non_ordered_items;
                    ++non_ordered_items;
                }
            max_argN = non_ordered_items-1;
        }
    
        // C: set some member data :
        items_.resize(num_items, format_item_t(io::detail::const_or_not(fac).widen( ' ')) );

        if(special_things) style_ |= special_needs;
        num_args_ = max_argN + 1;
        if(ordered_args) style_ |=  ordered;
        else style_ &= ~ordered;
        return *this;
    }

} // namespace boost


#endif //  BOOST_FORMAT_PARSING_HPP

//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#define BOOST_LOCALE_SOURCE
#include <boost/locale/generator.hpp>
#include <boost/locale/encoding.hpp>

#include "../encoding/conv.hpp"

#include <boost/locale/util.hpp>

#ifdef BOOST_MSVC
#  pragma warning(disable : 4244 4996) // loose data 
#endif

#include <cstddef>
#include <string.h>
#include <vector>
#include <algorithm>

//#define DEBUG_CODECVT

#ifdef DEBUG_CODECVT            
#include <iostream>
#endif

namespace boost {
namespace locale {
namespace util {
    
    class utf8_converter  : public base_converter {
    public:
        virtual int max_len() const
        {
            return 4;
        }

        virtual utf8_converter *clone() const
        {
            return new utf8_converter();
        }

        bool is_thread_safe() const
        {
            return true;
        }

        virtual uint32_t to_unicode(char const *&begin,char const *end)
        {
            char const *p=begin;
                        
            utf::code_point c = utf::utf_traits<char>::decode(p,end);

            if(c==utf::illegal)
                return illegal;

            if(c==utf::incomplete)
                return incomplete;

            begin = p;
            return c;
        }

        virtual uint32_t from_unicode(uint32_t u,char *begin,char const *end) 
        {
            if(!utf::is_valid_codepoint(u))
                return illegal;
            int width = utf::utf_traits<char>::width(u);
            std::ptrdiff_t d=end-begin;
            if(d < width)
                return incomplete;
            utf::utf_traits<char>::encode(u,begin);
            return width;
        }
    }; // utf8_converter

    class simple_converter : public base_converter {
    public:

        virtual ~simple_converter() 
        {
        }

        simple_converter(std::string const &encoding)
        {
            for(unsigned i=0;i<128;i++)
                to_unicode_tbl_[i]=i;
            for(unsigned i=128;i<256;i++) {
                char buf[2] = { char(i) , 0 };
                try {
                    std::wstring const tmp = conv::to_utf<wchar_t>(buf,buf+1,encoding,conv::stop);
                    if(tmp.size() == 1) {
                        to_unicode_tbl_[i] = tmp[0];
                    }
                    else {
                        to_unicode_tbl_[i] = illegal;
                    }
                }
                catch(conv::conversion_error const &/*e*/) {
                    to_unicode_tbl_[i] = illegal;
                }
            }
            from_unicode_tbl_.resize(256);
            for(unsigned i=0;i<256;i++) {
                from_unicode_tbl_[to_unicode_tbl_[i] & 0xFF].push_back(i);
            }
        }

        virtual int max_len() const 
        {
            return 1;
        }

        virtual bool is_thread_safe() const 
        {
            return true;
        }
        virtual base_converter *clone() const 
        {
           return new simple_converter(*this); 
        }
        virtual uint32_t to_unicode(char const *&begin,char const *end)
        {
            if(begin==end)
                return incomplete;
            unsigned char c = *begin++;
            return to_unicode_tbl_[c];
        }
        virtual uint32_t from_unicode(uint32_t u,char *begin,char const *end)
        {
            if(begin==end)
                return incomplete;
            std::vector<unsigned char> const &tbl = from_unicode_tbl_[u & 0xFF];
            for(std::vector<unsigned char>::const_iterator p=tbl.begin();p!=tbl.end();++p) {
                if(to_unicode_tbl_[*p]==u) {
                    *begin++ = *p;
                    return 1;
                }
            }
            return illegal;
        }
    private:
        uint32_t to_unicode_tbl_[256];
        std::vector<std::vector<unsigned char> > from_unicode_tbl_;
    };

    namespace {
        char const *simple_encoding_table[] = {
            "cp1250",
            "cp1251",
            "cp1252",
            "cp1253",
            "cp1254",
            "cp1255",
            "cp1256",
            "cp1257",
            "iso88591",
            "iso885913",
            "iso885915",
            "iso88592",
            "iso88593",
            "iso88594",
            "iso88595",
            "iso88596",
            "iso88597",
            "iso88598",
            "iso88599",
            "koi8r",
            "koi8u",
            "usascii",
            "windows1250",
            "windows1251",
            "windows1252",
            "windows1253",
            "windows1254",
            "windows1255",
            "windows1256",
            "windows1257"
        };

        bool compare_strings(char const *l,char const *r)
        {
            return strcmp(l,r) < 0;
        }
    }

    
    std::auto_ptr<base_converter> create_simple_converter(std::string const &encoding)
    {
        std::auto_ptr<base_converter> res;
        std::string norm = conv::impl::normalize_encoding(encoding.c_str());
        if(std::binary_search<char const **>( simple_encoding_table,
                        simple_encoding_table + sizeof(simple_encoding_table)/sizeof(char const *),
                        norm.c_str(),
                        compare_strings))
        {
            res.reset(new simple_converter(encoding));
        }
        return res;
    }



    std::auto_ptr<base_converter> create_utf8_converter()
    {
        std::auto_ptr<base_converter> res(new utf8_converter());
        return res;
    }
    
    //
    // Traits for sizeof char
    //
    template<typename CharType,int n=sizeof(CharType)>
    struct uchar_traits;

    template<typename CharType>
    struct uchar_traits<CharType,2> {
        typedef uint16_t uint_type;
    };
    template<typename CharType>
    struct uchar_traits<CharType,4> {
        typedef uint32_t uint_type;
    };

    // Real codecvt

    template<typename CharType>
    class code_converter : public std::codecvt<CharType,char,std::mbstate_t> 
    {
    public:
        code_converter(std::auto_ptr<base_converter> cvt,size_t refs = 0) :
          std::codecvt<CharType,char,std::mbstate_t>(refs),
            cvt_(cvt)
        {
            max_len_ = cvt_->max_len(); 
        }
    protected:

        typedef CharType uchar;

        virtual std::codecvt_base::result do_unshift(std::mbstate_t &s,char *from,char * /*to*/,char *&next) const
        {
            uint16_t &state = *reinterpret_cast<uint16_t *>(&s);
#ifdef DEBUG_CODECVT            
            std::cout << "Entering unshift " << std::hex << state << std::dec << std::endl;
#endif            
            if(state != 0)
                return std::codecvt_base::error;
            next=from;
            return std::codecvt_base::ok;
        }
        virtual int do_encoding() const throw()
        {
            return 0;
        }
        virtual int do_max_length() const throw()
        {
            return max_len_;
        }
        virtual bool do_always_noconv() const throw()
        {
            return false;
        }
        
        virtual std::codecvt_base::result 
        do_in(  std::mbstate_t &state,
                char const *from,
                char const *from_end,
                char const *&from_next,
                uchar *uto,
                uchar *uto_end,
                uchar *&uto_next) const
        {
            typedef typename uchar_traits<uchar>::uint_type uint_type;
            uint_type *to=reinterpret_cast<uint_type *>(uto);
            uint_type *to_end=reinterpret_cast<uint_type *>(uto_end);
            uint_type *&to_next=reinterpret_cast<uint_type *&>(uto_next);
            return do_real_in(state,from,from_end,from_next,to,to_end,to_next);
        }
        
        virtual int
        do_length(  std::mbstate_t &state,
                char const *from,
                char const *from_end,
                size_t max) const
        {
            char const *from_next=from;
            std::vector<uchar> chrs(max+1);
            uchar *to=&chrs.front();
            uchar *to_end=to+max;
            uchar *to_next=to;
            do_in(state,from,from_end,from_next,to,to_end,to_next);
            return from_next-from;
        }

        virtual std::codecvt_base::result 
        do_out( std::mbstate_t &state,
                uchar const *ufrom,
                uchar const *ufrom_end,
                uchar const *&ufrom_next,
                char *to,
                char *to_end,
                char *&to_next) const
        {
            typedef typename uchar_traits<uchar>::uint_type uint_type;
            uint_type const *from=reinterpret_cast<uint_type const *>(ufrom);
            uint_type const *from_end=reinterpret_cast<uint_type const *>(ufrom_end);
            uint_type const *&from_next=reinterpret_cast<uint_type const *&>(ufrom_next);
            return do_real_out(state,from,from_end,from_next,to,to_end,to_next);
        }
       

    private:

        //
        // Implementation for UTF-32
        //
        std::codecvt_base::result
        do_real_in( std::mbstate_t &/*state*/,
                    char const *from,
                    char const *from_end,
                    char const *&from_next,
                    uint32_t *to,
                    uint32_t *to_end,
                    uint32_t *&to_next) const
        {
            std::auto_ptr<base_converter> cvtp;
            base_converter *cvt = 0;
            if(cvt_->is_thread_safe()) {
                cvt = cvt_.get();
            }
            else {
                cvtp.reset(cvt_->clone());
                cvt = cvtp.get();
            }
            std::codecvt_base::result r=std::codecvt_base::ok;
            while(to < to_end && from < from_end)
            {
                uint32_t ch=cvt->to_unicode(from,from_end);
                if(ch==base_converter::illegal) {
                    r=std::codecvt_base::error;
                    break;
                }
                if(ch==base_converter::incomplete) {
                    r=std::codecvt_base::partial;
                    break;
                }
                *to++=ch;
            }
            from_next=from;
            to_next=to;
            if(r!=std::codecvt_base::ok)
                return r;
            if(from!=from_end)
                return std::codecvt_base::partial;
            return r;
        }

        //
        // Implementation for UTF-32
        //
        std::codecvt_base::result
        do_real_out(std::mbstate_t &/*state*/, // state is not used there
                    uint32_t const *from,
                    uint32_t const *from_end,
                    uint32_t const *&from_next,
                    char *to,
                    char *to_end,
                    char *&to_next) const
        {
            std::auto_ptr<base_converter> cvtp;
            base_converter *cvt = 0;
            if(cvt_->is_thread_safe()) {
                cvt = cvt_.get();
            }
            else {
                cvtp.reset(cvt_->clone());
                cvt = cvtp.get();
            }
            
            std::codecvt_base::result r=std::codecvt_base::ok;
            while(to < to_end && from < from_end)
            {
                uint32_t len=cvt->from_unicode(*from,to,to_end);
                if(len==base_converter::illegal) {
                    r=std::codecvt_base::error;
                    break;
                }
                if(len==base_converter::incomplete) {
                    r=std::codecvt_base::partial;
                    break;
                }
                from++;
                to+=len;
            }
            from_next=from;
            to_next=to;
            if(r!=std::codecvt_base::ok)
                return r;
            if(from!=from_end)
                return std::codecvt_base::partial;
            return r;
        }

        //
        // Implementation for UTF-16
        //
        std::codecvt_base::result
        do_real_in( std::mbstate_t &std_state,
                    char const *from,
                    char const *from_end,
                    char const *&from_next,
                    uint16_t *to,
                    uint16_t *to_end,
                    uint16_t *&to_next) const
        {
            std::auto_ptr<base_converter> cvtp;
            base_converter *cvt = 0;
            if(cvt_->is_thread_safe()) {
                cvt = cvt_.get();
            }
            else {
                cvtp.reset(cvt_->clone());
                cvt = cvtp.get();
            }
            std::codecvt_base::result r=std::codecvt_base::ok;
            // mbstate_t is POD type and should be initialized to 0 (i.a. state = stateT())
            // according to standard. We use it to keed a flag 0/1 for surrogate pair writing
            //
            // if 0 no code above >0xFFFF observed, of 1 a code above 0xFFFF observerd
            // and first pair is written, but no input consumed
            uint16_t &state = *reinterpret_cast<uint16_t *>(&std_state);
            while(to < to_end && from < from_end)
            {
#ifdef DEBUG_CODECVT            
                std::cout << "Entering IN--------------" << std::endl;
                std::cout << "State " << std::hex << state <<std::endl;
                std::cout << "Left in " << std::dec << from_end - from << " out " << to_end -to << std::endl;
#endif           
                char const *from_saved = from;
                uint32_t ch=cvt->to_unicode(from,from_end);
                if(ch==base_converter::illegal) {
                    r=std::codecvt_base::error;
                    break;
                }
                if(ch==base_converter::incomplete) {
                    r=std::codecvt_base::partial;
                    break;
                }
                // Normal codepoints go direcly to stream
                if(ch <= 0xFFFF) {
                    *to++=ch;
                }
                else {
                    // for  other codepoints we do following
                    //
                    // 1. We can't consume our input as we may find ourselfs
                    //    in state where all input consumed but not all output written,i.e. only
                    //    1st pair is written
                    // 2. We only write first pair and mark this in the state, we also revert back
                    //    the from pointer in order to make sure this codepoint would be read
                    //    once again and then we would consume our input together with writing
                    //    second surrogate pair
                    ch-=0x10000;
                    uint16_t vh = ch >> 10;
                    uint16_t vl = ch & 0x3FF;
                    uint16_t w1 = vh + 0xD800;
                    uint16_t w2 = vl + 0xDC00;
                    if(state == 0) {
                        from = from_saved;
                        *to++ = w1;
                        state = 1;
                    }
                    else {
                        *to++ = w2;
                        state = 0;
                    }
                }
            }
            from_next=from;
            to_next=to;
            if(r == std::codecvt_base::ok && (from!=from_end || state!=0))
                r = std::codecvt_base::partial;
#ifdef DEBUG_CODECVT            
            std::cout << "Returning ";
            switch(r) {
            case std::codecvt_base::ok:
                std::cout << "ok" << std::endl;
                break;
            case std::codecvt_base::partial:
                std::cout << "partial" << std::endl;
                break;
            case std::codecvt_base::error:
                std::cout << "error" << std::endl;
                break;
            default:
                std::cout << "other" << std::endl;
                break;
            }
            std::cout << "State " << std::hex << state <<std::endl;
            std::cout << "Left in " << std::dec << from_end - from << " out " << to_end -to << std::endl;
#endif            
            return r;
        }

        //encoding// Implementation for UTF-16
        //
        std::codecvt_base::result
        do_real_out(std::mbstate_t &std_state,
                    uint16_t const *from,
                    uint16_t const *from_end,
                    uint16_t const *&from_next,
                    char *to,
                    char *to_end,
                    char *&to_next) const
        {
            std::auto_ptr<base_converter> cvtp;
            base_converter *cvt = 0;
            if(cvt_->is_thread_safe()) {
                cvt = cvt_.get();
            }
            else {
                cvtp.reset(cvt_->clone());
                cvt = cvtp.get();
            }
            std::codecvt_base::result r=std::codecvt_base::ok;
            // mbstate_t is POD type and should be initialized to 0 (i.a. state = stateT())
            // according to standard. We assume that sizeof(mbstate_t) >=2 in order
            // to be able to store first observerd surrogate pair
            //
            // State: state!=0 - a first surrogate pair was observerd (state = first pair),
            // we expect the second one to come and then zero the state
            ///
            uint16_t &state = *reinterpret_cast<uint16_t *>(&std_state);
            while(to < to_end && from < from_end)
            {
#ifdef DEBUG_CODECVT            
            std::cout << "Entering OUT --------------" << std::endl;
            std::cout << "State " << std::hex << state <<std::endl;
            std::cout << "Left in " << std::dec << from_end - from << " out " << to_end -to << std::endl;
#endif            
                uint32_t ch=0;
                if(state != 0) {
                    // if the state idecates that 1st surrogate pair was written
                    // we should make sure that the second one that comes is actually
                    // second surrogate
                    uint16_t w1 = state;
                    uint16_t w2 = *from; 
                    // we don't forward from as writing may fail to incomplete or
                    // partial conversion
                    if(0xDC00 <= w2 && w2<=0xDFFF) {
                        uint16_t vh = w1 - 0xD800;
                        uint16_t vl = w2 - 0xDC00;
                        ch=((uint32_t(vh) << 10)  | vl) + 0x10000;
                    }
                    else {
                        // Invalid surrogate
                        r=std::codecvt_base::error;
                        break;
                    }
                }
                else {
                    ch = *from;
                    if(0xD800 <= ch && ch<=0xDBFF) {
                        // if this is a first surrogate pair we put
                        // it into the state and consume it, note we don't
                        // go forward as it should be illegal so we increase
                        // the from pointer manually
                        state = ch;
                        from++;
                        continue;
                    }
                    else if(0xDC00 <= ch && ch<=0xDFFF) {
                        // if we observe second surrogate pair and 
                        // first only may be expected we should break from the loop with error
                        // as it is illegal input
                        r=std::codecvt_base::error;
                        break;
                    }
                }
                
                uint32_t len=cvt->from_unicode(ch,to,to_end);
                if(len==base_converter::illegal) {
                    r=std::codecvt_base::error;
                    break;
                }
                if(len==base_converter::incomplete) {
                    r=std::codecvt_base::partial;
                    break;
                }
                state = 0;
                to+=len;
                from++;
            }
            from_next=from;
            to_next=to;
            if(r==std::codecvt_base::ok && from!=from_end)
                r = std::codecvt_base::partial;
#ifdef DEBUG_CODECVT            
            std::cout << "Returning ";
            switch(r) {
            case std::codecvt_base::ok:
                std::cout << "ok" << std::endl;
                break;
            case std::codecvt_base::partial:
                std::cout << "partial" << std::endl;
                break;
            case std::codecvt_base::error:
                std::cout << "error" << std::endl;
                break;
            default:
                std::cout << "other" << std::endl;
                break;
            }
            std::cout << "State " << std::hex << state <<std::endl;
            std::cout << "Left in " << std::dec << from_end - from << " out " << to_end -to << std::endl;
#endif            
            return r;
        }
        
        int max_len_;
        std::auto_ptr<base_converter> cvt_;

    };

    static const char ensure_mbstate_size_is_at_least_2[sizeof(std::mbstate_t) >= 2 ? 1 : -1] = {0};
    
    template<>
    class code_converter<char> : public std::codecvt<char,char,std::mbstate_t>
    {
    public:
        code_converter(std::auto_ptr<base_converter> /*cvt*/,size_t refs = 0) : 
          std::codecvt<char,char,std::mbstate_t>(refs)  
        {
        }
    };


    std::locale create_codecvt(std::locale const &in,std::auto_ptr<base_converter> cvt,character_facet_type type)
    {
        if(!cvt.get())
            cvt.reset(new base_converter());
        switch(type) {
        case char_facet:
            return std::locale(in,new code_converter<char>(cvt));
        case wchar_t_facet:
            return std::locale(in,new code_converter<wchar_t>(cvt));
        #if defined(BOOST_HAS_CHAR16_T) && !defined(BOOST_NO_CHAR16_T_CODECVT)
        case char16_t_facet:
            return std::locale(in,new code_converter<char16_t>(cvt));
        #endif
        #if defined(BOOST_HAS_CHAR32_T) && !defined(BOOST_NO_CHAR32_T_CODECVT)
        case char32_t_facet:
            return std::locale(in,new code_converter<char32_t>(cvt));
        #endif
        default:
            return in;
        }
    }


} // util
} // locale 
} // boost

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_ICONV_FIXER_HPP
#define BOOST_LOCALE_ICONV_FIXER_HPP

#include <iconv.h>

namespace boost {
    namespace locale {
        extern "C" {
            typedef size_t (*gnu_iconv_ptr_type)(iconv_t d,char const **in,size_t *insize,char **out,size_t *outsize);
            typedef size_t (*posix_iconv_ptr_type)(iconv_t d,char **in,size_t *insize,char **out,size_t *outsize);
        }
        inline size_t do_iconv(gnu_iconv_ptr_type ptr,iconv_t d,char **in,size_t *insize,char **out,size_t *outsize)
        {
            char const **rin = const_cast<char const **>(in);
            return ptr(d,rin,insize,out,outsize);
        }
        inline size_t do_iconv(posix_iconv_ptr_type ptr,iconv_t d,char **in,size_t *insize,char **out,size_t *outsize)
        {
            return ptr(d,in,insize,out,outsize);
        }
        inline size_t call_iconv(iconv_t d,char **in,size_t *insize,char **out,size_t *outsize)
        {
            return do_iconv( iconv, d, in,insize,out,outsize);
        }

    } // locale 
} // boost

#endif
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

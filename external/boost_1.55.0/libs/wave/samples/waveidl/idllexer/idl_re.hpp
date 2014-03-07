/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Sample: Re2C based IDL lexer
    
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(IDL_RE_HPP_BD62775D_1659_4684_872C_03C02543C9A5_INCLUDED)
#define IDL_RE_HPP_BD62775D_1659_4684_872C_03C02543C9A5_INCLUDED

#include <boost/wave/token_ids.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace idllexer {
namespace re2clex {

///////////////////////////////////////////////////////////////////////////////
//  The scanner function to call whenever a new token is requested
boost::wave::token_id scan(
    boost::wave::cpplexer::re2clex::Scanner *s);

///////////////////////////////////////////////////////////////////////////////
}   // namespace re2clex
}   // namespace idllexer
}   // namespace wave
}   // namespace boost

#endif // !defined(IDL_RE_HPP_BD62775D_1659_4684_872C_03C02543C9A5_INCLUDED)

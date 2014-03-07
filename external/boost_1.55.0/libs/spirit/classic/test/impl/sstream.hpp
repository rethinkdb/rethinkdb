/*=============================================================================
    Copyright (c) 2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/config.hpp>

///////////////////////////////////////////////////////////////////////////
// workaround for prestandard support of stringstreams
//
// * defines sstream_t for the string stream type
// * defines std::string getstring(sstream_t &);
//

#ifdef BOOST_NO_STRINGSTREAM
#   include <strstream>
    typedef strstream sstream_t;
    std::string
    getstring(std::strstream& ss)
    {
        ss << ends;
        std::string rval = ss.str();
        ss.freeze(false);
        return rval;
    }
#else
#   include <sstream>
    typedef std::stringstream sstream_t;
    std::string
    getstring(std::stringstream &ss)
    {
        return ss.str();
    }
#endif

void use_getstring_to_avoid_compiler_warnings_about_unused_functions()
{
    sstream_t ss;
    getstring(ss);
    if(!ss) { // to be not recursive on all control paths
        use_getstring_to_avoid_compiler_warnings_about_unused_functions();
    }
}

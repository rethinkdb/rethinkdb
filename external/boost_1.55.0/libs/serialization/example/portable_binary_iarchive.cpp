/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// portable_binary_iarchive.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <istream>
#include <string>

#include <boost/detail/endian.hpp>
#include <boost/serialization/throw_exception.hpp>
#include <boost/archive/archive_exception.hpp>

#include "portable_binary_iarchive.hpp"

void 
portable_binary_iarchive::load_impl(boost::intmax_t & l, char maxsize){
    char size;
    l = 0;
    this->primitive_base_t::load(size);

    if(0 == size){
        return;
    }

    bool negative = (size < 0);
    if(negative)
        size = -size;

    if(size > maxsize)
        boost::serialization::throw_exception(
            portable_binary_iarchive_exception()
        );

    char * cptr = reinterpret_cast<char *>(& l);
    #ifdef BOOST_BIG_ENDIAN
        cptr += (sizeof(boost::intmax_t) - size);
    #endif
    this->primitive_base_t::load_binary(cptr, size);

    #ifdef BOOST_BIG_ENDIAN
        if(m_flags & endian_little)
    #else
        if(m_flags & endian_big)
    #endif
            reverse_bytes(size, cptr);
    
    if(negative)
        l = -l;
}

void
portable_binary_iarchive::load_override(
    boost::archive::class_name_type & t, int
){
    std::string cn;
    cn.reserve(BOOST_SERIALIZATION_MAX_KEY_SIZE);
    load_override(cn, 0);
    if(cn.size() > (BOOST_SERIALIZATION_MAX_KEY_SIZE - 1))
        boost::serialization::throw_exception(
            boost::archive::archive_exception(
                boost::archive::archive_exception::invalid_class_name)
            );
    std::memcpy(t, cn.data(), cn.size());
    // borland tweak
    t.t[cn.size()] = '\0';
}

void 
portable_binary_iarchive::init(unsigned int flags){
    if(0 == (flags & boost::archive::no_header)){
        // read signature in an archive version independent manner
        std::string file_signature;
        * this >> file_signature;
        if(file_signature != boost::archive::BOOST_ARCHIVE_SIGNATURE())
            boost::serialization::throw_exception(
                boost::archive::archive_exception(
                    boost::archive::archive_exception::invalid_signature
                )
            );
        // make sure the version of the reading archive library can
        // support the format of the archive being read
        boost::archive::library_version_type input_library_version;
        * this >> input_library_version;

        // extra little .t is to get around borland quirk
        if(boost::archive::BOOST_ARCHIVE_VERSION() < input_library_version)
            boost::serialization::throw_exception(
                boost::archive::archive_exception(
                    boost::archive::archive_exception::unsupported_version
                )
            );
        
        #if BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3205))
        this->set_library_version(input_library_version);
        //#else
        //#if ! BOOST_WORKAROUND(BOOST_MSVC, <= 1200)
        //detail::
        //#endif
        boost::archive::detail::basic_iarchive::set_library_version(
            input_library_version
        );
        #endif
    }
    unsigned char x;
    load(x);
    m_flags = x << CHAR_BIT;
}

#include <boost/archive/impl/archive_serializer_map.ipp>
#include <boost/archive/impl/basic_binary_iprimitive.ipp>

namespace boost {
namespace archive {

namespace detail {
    template class archive_serializer_map<portable_binary_iarchive>;
}

template class basic_binary_iprimitive<
    portable_binary_iarchive,
    std::istream::char_type, 
    std::istream::traits_type
> ;

} // namespace archive
} // namespace boost

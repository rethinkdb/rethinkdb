/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// portable_binary_oarchive.cpp

// (C) Copyright 2002-7 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <ostream>
#include <boost/detail/endian.hpp>
#include "portable_binary_oarchive.hpp"

void 
portable_binary_oarchive::save_impl(
    const boost::intmax_t l,
    const char maxsize
){
    char size = 0;

    if(l == 0){
        this->primitive_base_t::save(size);
        return;
    }

    boost::intmax_t ll;
    bool negative = (l < 0);
    if(negative)
        ll = -l;
    else
        ll = l;

    do{
        ll >>= CHAR_BIT;
        ++size;
    }while(ll != 0);

    this->primitive_base_t::save(
        static_cast<char>(negative ? -size : size)
    );

    if(negative)
        ll = -l;
    else
        ll = l;
    char * cptr = reinterpret_cast<char *>(& ll);
    #ifdef BOOST_BIG_ENDIAN
        cptr += (sizeof(boost::intmax_t) - size);
        if(m_flags & endian_little)
            reverse_bytes(size, cptr);
    #else
        if(m_flags & endian_big)
            reverse_bytes(size, cptr);
    #endif
    this->primitive_base_t::save_binary(cptr, size);
}

void 
portable_binary_oarchive::init(unsigned int flags) {
    if(m_flags == (endian_big | endian_little)){
        boost::serialization::throw_exception(
            portable_binary_oarchive_exception()
        );
    }
    if(0 == (flags & boost::archive::no_header)){
        // write signature in an archive version independent manner
        const std::string file_signature(
            boost::archive::BOOST_ARCHIVE_SIGNATURE()
        );
        * this << file_signature;
        // write library version
        const boost::archive::library_version_type v(
            boost::archive::BOOST_ARCHIVE_VERSION()
        );
        * this << v;
    }
    save(static_cast<unsigned char>(m_flags >> CHAR_BIT));
}

#include <boost/archive/impl/archive_serializer_map.ipp>
#include <boost/archive/impl/basic_binary_oprimitive.ipp>

namespace boost {
namespace archive {

namespace detail {
    template class archive_serializer_map<portable_binary_oarchive>;
}

template class basic_binary_oprimitive<
    portable_binary_oarchive,
    std::ostream::char_type, 
    std::ostream::traits_type
> ;

} // namespace archive
} // namespace boost

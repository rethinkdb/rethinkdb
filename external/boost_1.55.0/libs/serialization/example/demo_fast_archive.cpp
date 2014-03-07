/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// demo_fast_binary_archive.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// should pass compilation and execution
#include <sstream>

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/serialization/pfto.hpp>

#define BOOST_ARCHIVE_SOURCE
#include <boost/archive/binary_oarchive_impl.hpp>
#include <boost/archive/binary_iarchive_impl.hpp>
#include <boost/archive/detail/register_archive.hpp>

// include template definitions for base classes used.  Otherwise
// you'll get link failure with undefined symbols
#include <boost/archive/impl/basic_binary_oprimitive.ipp>
#include <boost/archive/impl/basic_binary_iprimitive.ipp>
#include <boost/archive/impl/basic_binary_oarchive.ipp>
#include <boost/archive/impl/basic_binary_iarchive.ipp>

using namespace boost::archive;

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// "Fast" output binary archive.  This is a variation of the native binary 
class fast_binary_oarchive :
    // don't derive from binary_oarchive !!!
    public binary_oarchive_impl<
        fast_binary_oarchive, 
        std::ostream::char_type, 
        std::ostream::traits_type
    >
{
    typedef fast_binary_oarchive derived_t;
    typedef binary_oarchive_impl<
        fast_binary_oarchive, 
        std::ostream::char_type, 
        std::ostream::traits_type
    > base_t;
#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
public:
#else
    friend class boost::archive::detail::interface_oarchive<derived_t>;
    friend class basic_binary_oarchive<derived_t>;
    friend class basic_binary_oprimitive<
        derived_t, 
        std::ostream::char_type, 
        std::ostream::traits_type
    >;
    friend class boost::archive::save_access;
#endif
    // add base class to the places considered when matching
    // save function to a specific set of arguments.  Note, this didn't
    // work on my MSVC 7.0 system using 
    // binary_oarchive_impl<derived_t>::load_override;
    // so we use the sure-fire method below.  This failed to work as well
    template<class T>
    void save_override(T & t, BOOST_PFTO int){
        base_t::save_override(t, 0);
        // verify that this program is in fact working by making sure
        // that arrays are getting passed here
        BOOST_STATIC_ASSERT(! (boost::is_array<T>::value) );
    }
    template<int N>
    void save_override(const int (& t)[N] , int){
        save_binary(t, sizeof(t));
    }
    template<int N>
    void save_override(const unsigned int (& t)[N], int){
        save_binary(t, sizeof(t));
    }
    template<int N>
    void save_override(const long (& t)[N], int){
        save_binary(t, sizeof(t));
    }
    template<int N>
    void save_override(const unsigned long (& t)[N], int){
        save_binary(t, sizeof(t));
    }
public:
    fast_binary_oarchive(std::ostream & os, unsigned flags = 0) :
       base_t(os, flags)
    {}
    fast_binary_oarchive(std::streambuf & bsb, unsigned int flags = 0) :
        base_t(bsb, flags)
    {}
};

// required by export
BOOST_SERIALIZATION_REGISTER_ARCHIVE(fast_binary_oarchive)

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// "Fast" input binary archive.  This is a variation of the native binary 
class fast_binary_iarchive :
    // don't derive from binary_oarchive !!!
    public binary_iarchive_impl<
        fast_binary_iarchive, 
        std::istream::char_type, 
        std::istream::traits_type
    >
{
    typedef fast_binary_iarchive derived_t;
    typedef binary_iarchive_impl<
        fast_binary_iarchive, 
        std::istream::char_type, 
        std::istream::traits_type
    > base_t;
#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
public:
#else
    friend class boost::archive::detail::interface_iarchive<derived_t>;
    friend class basic_binary_iarchive<derived_t>;
    friend class basic_binary_iprimitive<
        derived_t, 
        std::ostream::char_type, 
        std::ostream::traits_type
    >;
    friend class boost::archive::load_access;
#endif
    // add base class to the places considered when matching
    // save function to a specific set of arguments.  Note, this didn't
    // work on my MSVC 7.0 system using 
    // binary_oarchive_impl<derived_t>::load_override;
    // so we use the sure-fire method below.  This failed to work as well
    template<class T>
    void load_override(T & t, BOOST_PFTO int){
        base_t::load_override(t, 0);
        BOOST_STATIC_ASSERT(! (boost::is_array<T>::value) );
    }
    template<int N>
    void load_override(int (& t)[N], int){
        load_binary(t, sizeof(t));
    }
    template<int N>
    void load_override(unsigned int (& t)[N], int){
        load_binary(t, sizeof(t));
    }
    template<int N>
    void load_override(long (& t)[N], int){
        load_binary(t, sizeof(t));
    }
    template<int N>
    void load_override(unsigned long (& t)[N], int){
        load_binary(t, sizeof(t));
    }
public:
    fast_binary_iarchive(std::istream & is, unsigned int flags = 0) :
        base_t(is, flags)
    {}
    fast_binary_iarchive(std::streambuf & bsb, unsigned int flags = 0) :
        base_t(bsb, flags)
    {}
};

// required by export
BOOST_SERIALIZATION_REGISTER_ARCHIVE(fast_binary_iarchive)

int main( int argc, char* argv[] )
{
    const int a[3] = {1, 2, 3};
    int a1[3] = {4, 5, 6};

    std::stringstream ss;
    {   
        fast_binary_oarchive pboa(ss);
        pboa << a;
    }
    {
        fast_binary_iarchive pbia(ss);
        pbia >> a1;
    }
    return (a[0] != a1[0]) || (a[1] != a1[1]) || (a[2] != a1[2]);
}



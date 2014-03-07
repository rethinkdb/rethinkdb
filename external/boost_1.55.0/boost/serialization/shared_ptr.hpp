#ifndef BOOST_SERIALIZATION_SHARED_PTR_HPP
#define BOOST_SERIALIZATION_SHARED_PTR_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// shared_ptr.hpp: serialization for boost shared pointer

// (C) Copyright 2004 Robert Ramey and Martin Ecker
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <cstddef> // NULL

#include <boost/config.hpp>
#include <boost/mpl/integral_c.hpp>
#include <boost/mpl/integral_c_tag.hpp>

#include <boost/detail/workaround.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/serialization/split_free.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/tracking.hpp>

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// shared_ptr serialization traits
// version 1 to distinguish from boost 1.32 version. Note: we can only do this
// for a template when the compiler supports partial template specialization

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
    namespace boost {
    namespace serialization{
        template<class T>
        struct version< ::boost::shared_ptr< T > > {
            typedef mpl::integral_c_tag tag;
            #if BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3206))
            typedef BOOST_DEDUCED_TYPENAME mpl::int_<1> type;
            #else
            typedef mpl::int_<1> type;
            #endif
            #if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x570))
            BOOST_STATIC_CONSTANT(int, value = 1);
            #else
            BOOST_STATIC_CONSTANT(int, value = type::value);
            #endif
        };
        // don't track shared pointers
        template<class T>
        struct tracking_level< ::boost::shared_ptr< T > > { 
            typedef mpl::integral_c_tag tag;
            #if BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3206))
            typedef BOOST_DEDUCED_TYPENAME mpl::int_< ::boost::serialization::track_never> type;
            #else
            typedef mpl::int_< ::boost::serialization::track_never> type;
            #endif
            #if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x570))
            BOOST_STATIC_CONSTANT(int, value = ::boost::serialization::track_never);
            #else
            BOOST_STATIC_CONSTANT(int, value = type::value);
            #endif
        };
    }}
    #define BOOST_SERIALIZATION_SHARED_PTR(T)
#else
    // define macro to let users of these compilers do this
    #define BOOST_SERIALIZATION_SHARED_PTR(T)                         \
    BOOST_CLASS_VERSION(                                              \
        ::boost::shared_ptr< T >,                                     \
        1                                                             \
    )                                                                 \
    BOOST_CLASS_TRACKING(                                             \
        ::boost::shared_ptr< T >,                                     \
        ::boost::serialization::track_never                           \
    )                                                                 \
    /**/
#endif

namespace boost {
namespace serialization{

struct null_deleter {
    void operator()(void const *) const {}
};

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// serialization for shared_ptr

template<class Archive, class T>
inline void save(
    Archive & ar,
    const boost::shared_ptr< T > &t,
    const unsigned int /* file_version */
){
    // The most common cause of trapping here would be serializing
    // something like shared_ptr<int>.  This occurs because int
    // is never tracked by default.  Wrap int in a trackable type
    BOOST_STATIC_ASSERT((tracking_level< T >::value != track_never));
    const T * t_ptr = t.get();
    ar << boost::serialization::make_nvp("px", t_ptr);
}

#ifdef BOOST_SERIALIZATION_SHARED_PTR_132_HPP
template<class Archive, class T>
inline void load(
    Archive & ar,
    boost::shared_ptr< T > &t,
    const unsigned int file_version
){
    // The most common cause of trapping here would be serializing
    // something like shared_ptr<int>.  This occurs because int
    // is never tracked by default.  Wrap int in a trackable type
    BOOST_STATIC_ASSERT((tracking_level< T >::value != track_never));
    T* r;
    if(file_version < 1){
        //ar.register_type(static_cast<
        //    boost_132::detail::sp_counted_base_impl<T *, boost::checked_deleter< T > > *
        //>(NULL));
        ar.register_type(static_cast<
            boost_132::detail::sp_counted_base_impl<T *, null_deleter > *
        >(NULL));
        boost_132::shared_ptr< T > sp;
        ar >> boost::serialization::make_nvp("px", sp.px);
        ar >> boost::serialization::make_nvp("pn", sp.pn);
        // got to keep the sps around so the sp.pns don't disappear
        ar.append(sp);
        r = sp.get();
    }
    else{
        ar >> boost::serialization::make_nvp("px", r);
    }
    ar.reset(t,r);
}

#else
template<class Archive, class T>
inline void load(
    Archive & ar,
    boost::shared_ptr< T > &t,
    const unsigned int /*file_version*/
){
    // The most common cause of trapping here would be serializing
    // something like shared_ptr<int>.  This occurs because int
    // is never tracked by default.  Wrap int in a trackable type
    BOOST_STATIC_ASSERT((tracking_level< T >::value != track_never));
    T* r;
    ar >> boost::serialization::make_nvp("px", r);
    ar.reset(t,r);
}
#endif

template<class Archive, class T>
inline void serialize(
    Archive & ar,
    boost::shared_ptr< T > &t,
    const unsigned int file_version
){
    // correct shared_ptr serialization depends upon object tracking
    // being used.
    BOOST_STATIC_ASSERT(
        boost::serialization::tracking_level< T >::value
        != boost::serialization::track_never
    );
    boost::serialization::split_free(ar, t, file_version);
}

} // namespace serialization
} // namespace boost

#endif // BOOST_SERIALIZATION_SHARED_PTR_HPP

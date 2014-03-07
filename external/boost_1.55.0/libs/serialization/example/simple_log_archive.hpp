#ifndef BOOST_SIMPLE_LOG_ARCHIVE_HPP
#define BOOST_SIMPLE_LOG_ARCHIVE_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// simple_log_archive.hpp

// (C) Copyright 2010 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <ostream>
#include <cstddef> // std::size_t

#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::size_t; 
} // namespace std
#endif

#include <boost/type_traits/is_enum.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/access.hpp>

/////////////////////////////////////////////////////////////////////////
// log data to an output stream.  This illustrates a simpler implemenation
// of text output which is useful for getting a formatted display of
// any serializable class.  Intended to be useful as a debugging aid.
class simple_log_archive {
    std::ostream & m_os;
    unsigned int m_depth;

    template<class Archive>
    struct save_enum_type {
        template<class T>
        static void invoke(Archive &ar, const T &t){
            ar.m_os << static_cast<int>(t);
        }
    };
    template<class Archive>
    struct save_primitive {
        template<class T>
        static void invoke(Archive & ar, const T & t){
            ar.m_os << t;
        }
    };
    template<class Archive>
    struct save_only {
        template<class T>
        static void invoke(Archive & ar, const T & t){
            // make sure call is routed through the highest interface that might
            // be specialized by the user.
            boost::serialization::serialize_adl(
                ar, 
                const_cast<T &>(t), 
                ::boost::serialization::version< T >::value
            );
        }
    };
    template<class T>
    void save(const T &t){
        typedef 
            BOOST_DEDUCED_TYPENAME boost::mpl::eval_if<boost::is_enum< T >,
                boost::mpl::identity<save_enum_type<simple_log_archive> >,
            //else
            BOOST_DEDUCED_TYPENAME boost::mpl::eval_if<
                // if its primitive
                    boost::mpl::equal_to<
                        boost::serialization::implementation_level< T >,
                        boost::mpl::int_<boost::serialization::primitive_type>
                    >,
                    boost::mpl::identity<save_primitive<simple_log_archive> >,
            // else
                boost::mpl::identity<save_only<simple_log_archive> >
            > >::type typex;
        typex::invoke(*this, t);
    }    
    #ifndef BOOST_NO_STD_WSTRING
    void save(const std::wstring &ws){
        m_os << "wide string types not suported in log archive";
    }
    #endif

public:
    ///////////////////////////////////////////////////
    // Implement requirements for archive concept

    typedef boost::mpl::bool_<false> is_loading;
    typedef boost::mpl::bool_<true> is_saving;

    // this can be a no-op since we ignore pointer polymorphism
    template<class T>
    void register_type(const T * = NULL){}

    unsigned int get_library_version(){
        return 0;
    }

    void 
    save_binary(const void *address, std::size_t count){
        m_os << "save_binary not implemented";
    }

    // the << operators 
    template<class T>
    simple_log_archive & operator<<(T const & t){
        m_os << ' ';
        save(t);
        return * this;
    }
    template<class T>
    simple_log_archive & operator<<(T * const t){
        m_os << " ->";
        if(NULL == t)
            m_os << " null";
        else
            *this << * t;
        return * this;
    }
    template<class T, int N>
    simple_log_archive & operator<<(const T (&t)[N]){
        return *this << boost::serialization::make_array(
            static_cast<const T *>(&t[0]),
            N
        );
    }
    template<class T>
    simple_log_archive & operator<<(const boost::serialization::nvp< T > & t){
        m_os << '\n'; // start line with each named object
        // indent according to object depth
        for(unsigned int i = 0; i < m_depth; ++i)
            m_os << ' ';
        ++m_depth;
        m_os << t.name(); // output the name of the object
        * this << t.const_value();
        --m_depth;
        return * this;
    }

    // the & operator 
    template<class T>
    simple_log_archive & operator&(const T & t){
            return * this << t;
    }
    ///////////////////////////////////////////////

    simple_log_archive(std::ostream & os) :
        m_os(os),
        m_depth(0)
    {}
};

#endif // BOOST_SIMPLE_LOG_ARCHIVE_HPP

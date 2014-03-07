// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: storage.hpp 83253 2013-03-02 21:48:27Z steven_watanabe $

#ifndef BOOST_TYPE_ERASURE_DETAIL_STORAGE_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_STORAGE_HPP_INCLUDED

#include <boost/type_traits/remove_reference.hpp>

namespace boost {
namespace type_erasure {
namespace detail {

struct storage
{
    storage() {}
    template<class T>
    storage(const T& arg) : data(new T(arg)) {}
    void* data;
};


#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<class T>
T extract(T arg) { return std::forward<T>(arg); }

#else

template<class T>
T extract(T arg) { return arg; }

#endif

template<class T>
T extract(storage& arg)
{
    return *static_cast<typename ::boost::remove_reference<T>::type*>(arg.data);
}

template<class T>
T extract(const storage& arg)
{
    return *static_cast<const typename ::boost::remove_reference<T>::type*>(arg.data);
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<class T>
T extract(storage&& arg)
{
    return std::move(*static_cast<typename ::boost::remove_reference<T>::type*>(arg.data));
}

#endif

}
}
}

#endif

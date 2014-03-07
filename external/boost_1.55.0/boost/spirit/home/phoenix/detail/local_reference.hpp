/*=============================================================================
    Copyright (c) 2005-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_DETAIL_LOCAL_REFERENCE_HPP
#define PHOENIX_DETAIL_LOCAL_REFERENCE_HPP

#include <boost/utility/addressof.hpp>

namespace boost { namespace phoenix { namespace detail
{
    template <typename T>
    struct local_reference
    { 
        typedef T type;
    
        explicit local_reference(T& t): t_(boost::addressof(t)) {}
        operator T& () const { return *t_; }
        local_reference& operator=(T const& x) { *t_ = x; return *this; }
        local_reference const& operator=(T const& x) const { *t_ = x; return *this; }
        T& get() const { return *t_; }
        T* get_pointer() const { return t_; }
    
    private:
    
        T* t_;
    };
    
    template <typename T>
    struct unwrap_local_reference
    {
        typedef T type; // T should be a reference
    };

    template <typename T>
    struct unwrap_local_reference<local_reference<T> >
    {
        typedef T type; // unwrap the reference; T is a value
    };
}}}

#endif

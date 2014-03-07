#ifndef BOOST_SMART_PTR_ENABLE_SHARED_FROM_THIS_HPP_INCLUDED
#define BOOST_SMART_PTR_ENABLE_SHARED_FROM_THIS_HPP_INCLUDED

//
//  enable_shared_from_this.hpp
//
//  Copyright 2002, 2009 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  http://www.boost.org/libs/smart_ptr/enable_shared_from_this.html
//

#include <boost/smart_ptr/weak_ptr.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/assert.hpp>
#include <boost/config.hpp>

namespace boost
{

template<class T> class enable_shared_from_this
{
protected:

    enable_shared_from_this() BOOST_NOEXCEPT
    {
    }

    enable_shared_from_this(enable_shared_from_this const &) BOOST_NOEXCEPT
    {
    }

    enable_shared_from_this & operator=(enable_shared_from_this const &) BOOST_NOEXCEPT
    {
        return *this;
    }

    ~enable_shared_from_this() BOOST_NOEXCEPT // ~weak_ptr<T> newer throws, so this call also must not throw
    {
    }

public:

    shared_ptr<T> shared_from_this()
    {
        shared_ptr<T> p( weak_this_ );
        BOOST_ASSERT( p.get() == this );
        return p;
    }

    shared_ptr<T const> shared_from_this() const
    {
        shared_ptr<T const> p( weak_this_ );
        BOOST_ASSERT( p.get() == this );
        return p;
    }

public: // actually private, but avoids compiler template friendship issues

    // Note: invoked automatically by shared_ptr; do not call
    template<class X, class Y> void _internal_accept_owner( shared_ptr<X> const * ppx, Y * py ) const
    {
        if( weak_this_.expired() )
        {
            weak_this_ = shared_ptr<T>( *ppx, py );
        }
    }

private:

    mutable weak_ptr<T> weak_this_;
};

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_ENABLE_SHARED_FROM_THIS_HPP_INCLUDED

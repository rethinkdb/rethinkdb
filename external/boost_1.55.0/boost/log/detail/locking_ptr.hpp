/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   locking_ptr.hpp
 * \author Andrey Semashev
 * \date   15.07.2009
 *
 * This header is the Boost.Log library implementation, see the library documentation
 * at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_LOCKING_PTR_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_LOCKING_PTR_HPP_INCLUDED_

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/utility/explicit_operator_bool.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! Shared lock object to support locking_ptr
struct BOOST_LOG_NO_VTABLE locking_ptr_counter_base
{
    unsigned int m_RefCounter;

    locking_ptr_counter_base() : m_RefCounter(0)
    {
    }

    virtual ~locking_ptr_counter_base() {}
    virtual void lock() = 0;
    virtual bool try_lock() = 0;
    virtual void unlock() = 0;

private:
    locking_ptr_counter_base(locking_ptr_counter_base const&);
    locking_ptr_counter_base& operator= (locking_ptr_counter_base const&);
};

struct try_lock_tag {};
BOOST_CONSTEXPR_OR_CONST try_lock_tag try_lock = {};

//! A pointer type that locks the backend until it's destroyed
template< typename T >
class locking_ptr
{
public:
    //! Pointed type
    typedef T element_type;

private:
    //! The pointer to the backend
    shared_ptr< element_type > m_pElement;
    //! Reference to the shared lock control object
    locking_ptr_counter_base* m_pLock;

public:
    //! Constructor
    locking_ptr(shared_ptr< element_type > const& p, locking_ptr_counter_base& l)
        : m_pElement(p), m_pLock(&l)
    {
        if (m_pLock->m_RefCounter == 0)
            m_pLock->lock();
        ++m_pLock->m_RefCounter;
    }
    //! Constructor
    locking_ptr(shared_ptr< element_type > const& p, locking_ptr_counter_base& l, try_lock_tag const&)
        : m_pElement(p), m_pLock(&l)
    {
        if (m_pLock->m_RefCounter > 0 || m_pLock->try_lock())
        {
            ++m_pLock->m_RefCounter;
        }
        else
        {
            m_pElement.reset();
            m_pLock = NULL;
        }
    }
    //! Copy constructor
    locking_ptr(locking_ptr const& that) : m_pElement(that.m_pElement), m_pLock(that.m_pLock)
    {
        if (m_pLock)
            ++m_pLock->m_RefCounter;
    }
    //! Destructor
    ~locking_ptr()
    {
        if (m_pLock && --m_pLock->m_RefCounter == 0)
            m_pLock->unlock();
    }

    //! Assignment
    locking_ptr& operator= (locking_ptr that)
    {
        this->swap(that);
        return *this;
    }

    //! Indirection
    element_type* operator-> () const { return m_pElement.get(); }
    //! Dereferencing
    element_type& operator* () const { return *m_pElement; }

    //! Accessor to the raw pointer
    element_type* get() const { return m_pElement.get(); }

    //! Checks for null pointer
    BOOST_EXPLICIT_OPERATOR_BOOL()
    //! Checks for null pointer
    bool operator! () const { return !m_pElement; }

    //! Swaps two pointers
    void swap(locking_ptr& that)
    {
        m_pElement.swap(that.m_pElement);
        register locking_ptr_counter_base* p = m_pLock;
        m_pLock = that.m_pLock;
        that.m_pLock = p;
    }
};

//! Free raw pointer getter to assist generic programming
template< typename T >
inline T* get_pointer(locking_ptr< T > const& p)
{
    return p.get();
}
//! Free swap operation
template< typename T >
inline void swap(locking_ptr< T >& left, locking_ptr< T >& right)
{
    left.swap(right);
}

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_LOCKING_PTR_HPP_INCLUDED_

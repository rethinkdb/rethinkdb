/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   counter.hpp
 * \author Andrey Semashev
 * \date   01.05.2007
 *
 * The header contains implementation of the counter attribute.
 */

#ifndef BOOST_LOG_ATTRIBUTES_COUNTER_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_COUNTER_HPP_INCLUDED_

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_cast.hpp>
#include <boost/log/attributes/attribute_value_impl.hpp>
#ifndef BOOST_LOG_NO_THREADS
#include <boost/detail/atomic_count.hpp>
#endif // BOOST_LOG_NO_THREADS
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace attributes {

/*!
 * \brief A class of an attribute that counts an integral value
 *
 * This type of attribute acts as a counter, that is, it returns a monotonously
 * changing value each time requested. The attribute value type can be specified
 * as a template parameter. However, the type must be an integral type of size no
 * more than <tt>sizeof(long)</tt>.
 */
template< typename T >
class counter :
    public attribute
{
    //  For now only integral types up to long are supported
    BOOST_STATIC_ASSERT_MSG(is_integral< T >::value && sizeof(T) <= sizeof(long), "Boost.Log: Only integral types up to long are supported by counter attribute");

public:
    //! A counter value type
    typedef T value_type;

protected:
    //! Base class for factory implementation
    class BOOST_LOG_NO_VTABLE BOOST_SYMBOL_VISIBLE impl :
        public attribute::impl
    {
    };

    //! Generic factory implementation
    class impl_generic;
#ifndef BOOST_LOG_NO_THREADS
    //! Increment-by-one factory implementation
    class impl_inc;
    //! Decrement-by-one factory implementation
    class impl_dec;
#endif

public:
    /*!
     * Constructor
     *
     * \param initial Initial value of the counter
     * \param step Changing step of the counter. Each value acquired from the attribute
     *        will be greater than the previous one to this amount.
     */
    explicit counter(value_type initial = (value_type)0, long step = 1) :
#ifndef BOOST_LOG_NO_THREADS
        attribute()
    {
        if (step == 1)
            this->set_impl(new impl_inc(initial));
        else if (step == -1)
            this->set_impl(new impl_dec(initial));
        else
            this->set_impl(new impl_generic(initial, step));
    }
#else
        attribute(new impl_generic(initial, step))
    {
    }
#endif
    /*!
     * Constructor for casting support
     */
    explicit counter(cast_source const& source) :
        attribute(source.as< impl >())
    {
    }
};

#ifndef BOOST_LOG_NO_THREADS

template< typename T >
class counter< T >::impl_generic :
    public impl
{
private:
    //! Initial value
    const value_type m_Initial;
    //! Step value
    const long m_Step;
    //! The counter
    boost::detail::atomic_count m_Counter;

public:
    /*!
     * Initializing constructor
     */
    impl_generic(value_type initial, long step) : m_Initial(initial), m_Step(step), m_Counter(-1)
    {
    }

    attribute_value get_value()
    {
        register unsigned long next_counter = static_cast< unsigned long >(++m_Counter);
        register value_type next = static_cast< value_type >(m_Initial + (next_counter * m_Step));
        return make_attribute_value(next);
    }
};

template< typename T >
class counter< T >::impl_inc :
    public impl
{
private:
    //! The counter
    boost::detail::atomic_count m_Counter;

public:
    /*!
     * Initializing constructor
     */
    explicit impl_inc(value_type initial) : m_Counter(initial - 1)
    {
    }

    attribute_value get_value()
    {
        return make_attribute_value(static_cast< value_type >(++m_Counter));
    }
};

template< typename T >
class counter< T >::impl_dec :
    public impl
{
private:
    //! The counter
    boost::detail::atomic_count m_Counter;

public:
    /*!
     * Initializing constructor
     */
    explicit impl_dec(value_type initial) : m_Counter(initial + 1)
    {
    }

    attribute_value get_value()
    {
        return make_attribute_value(static_cast< value_type >(--m_Counter));
    }
};

#else // BOOST_LOG_NO_THREADS

template< typename T >
class counter< T >::impl_generic :
    public impl
{
private:
    //! Step value
    const long m_Step;
    //! The counter
    value_type m_Counter;

public:
    /*!
     * Initializing constructor
     */
    impl_generic(value_type initial, long step) : m_Step(step), m_Counter(initial - step)
    {
    }

    attribute_value get_value()
    {
        m_Counter += m_Step;
        return make_attribute_value(m_Counter);
    }
};

#endif // BOOST_LOG_NO_THREADS

} // namespace attributes

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_ATTRIBUTES_COUNTER_HPP_INCLUDED_

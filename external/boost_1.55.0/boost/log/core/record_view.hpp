/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   record_view.hpp
 * \author Andrey Semashev
 * \date   09.03.2009
 *
 * This header contains a logging record view class definition.
 */

#ifndef BOOST_LOG_CORE_RECORD_VIEW_HPP_INCLUDED_
#define BOOST_LOG_CORE_RECORD_VIEW_HPP_INCLUDED_

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/move/core.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/utility/explicit_operator_bool.hpp>
#include <boost/log/attributes/attribute_value_set.hpp>
#include <boost/log/expressions/keyword_fwd.hpp>
#ifndef BOOST_LOG_NO_THREADS
#include <boost/detail/atomic_count.hpp>
#endif // BOOST_LOG_NO_THREADS
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

#ifndef BOOST_LOG_DOXYGEN_PASS
class core;
class record;
#endif // BOOST_LOG_DOXYGEN_PASS

/*!
 * \brief Logging record view class
 *
 * The logging record encapsulates all information related to a single logging statement,
 * in particular, attribute values view and the log message string. The view is immutable,
 * it is implemented as a wrapper around a reference-counted implementation.
 */
class record_view
{
    BOOST_COPYABLE_AND_MOVABLE(record_view)

    friend class core;
    friend class record;

#ifndef BOOST_LOG_DOXYGEN_PASS
private:
    //! Private data
    struct private_data;
    friend struct private_data;

    //! Publicly available record data
    struct public_data
    {
        //! Reference counter
#ifndef BOOST_LOG_NO_THREADS
        mutable boost::detail::atomic_count m_ref_counter;
#else
        mutable unsigned int m_ref_counter;
#endif // BOOST_LOG_NO_THREADS

        //! Attribute values view
        attribute_value_set m_attribute_values;

        //! Constructor from the attribute sets
        explicit public_data(BOOST_RV_REF(attribute_value_set) values) :
            m_ref_counter(1),
            m_attribute_values(values)
        {
        }

        //! Destructor
        BOOST_LOG_API static void destroy(const public_data* p) BOOST_NOEXCEPT;

    protected:
        ~public_data() {}

        BOOST_DELETED_FUNCTION(public_data(public_data const&))
        BOOST_DELETED_FUNCTION(public_data& operator= (public_data const&))

        friend void intrusive_ptr_add_ref(const public_data* p) { ++p->m_ref_counter; }
        friend void intrusive_ptr_release(const public_data* p) { if (--p->m_ref_counter == 0) public_data::destroy(p); }
    };

private:
    //! A pointer to the log record implementation
    intrusive_ptr< public_data > m_impl;

private:
    //  A private constructor, accessible from record
    explicit record_view(public_data* impl) : m_impl(impl, false) {}

#endif // BOOST_LOG_DOXYGEN_PASS

public:
    /*!
     * Default constructor. Creates an empty record view that is equivalent to the invalid record handle.
     *
     * \post <tt>!*this == true</tt>
     */
    BOOST_DEFAULTED_FUNCTION(record_view(), {})

    /*!
     * Copy constructor
     */
    record_view(record_view const& that) BOOST_NOEXCEPT : m_impl(that.m_impl) {}

    /*!
     * Move constructor. Source record contents unspecified after the operation.
     */
    record_view(BOOST_RV_REF(record_view) that) BOOST_NOEXCEPT
    {
        m_impl.swap(that.m_impl);
    }

    /*!
     * Destructor. Destroys the record, releases any sinks and attribute values that were involved in processing this record.
     */
    ~record_view() BOOST_NOEXCEPT {}

    /*!
     * Copy assignment
     */
    record_view& operator= (BOOST_COPY_ASSIGN_REF(record_view) that) BOOST_NOEXCEPT
    {
        m_impl = that.m_impl;
        return *this;
    }

    /*!
     * Move assignment. Source record contents unspecified after the operation.
     */
    record_view& operator= (BOOST_RV_REF(record_view) that) BOOST_NOEXCEPT
    {
        m_impl.swap(that.m_impl);
        return *this;
    }

    /*!
     * \return A reference to the set of attribute values attached to this record
     *
     * \pre <tt>!!*this</tt>
     */
    attribute_value_set const& attribute_values() const BOOST_NOEXCEPT
    {
        return m_impl->m_attribute_values;
    }

    /*!
     * Equality comparison
     *
     * \param that Comparand
     * \return \c true if both <tt>*this</tt> and \a that identify the same log record or both do not
     *         identify any record, \c false otherwise.
     */
    bool operator== (record_view const& that) const BOOST_NOEXCEPT
    {
        return m_impl == that.m_impl;
    }

    /*!
     * Inequality comparison
     *
     * \param that Comparand
     * \return <tt>!(*this == that)</tt>
     */
    bool operator!= (record_view const& that) const BOOST_NOEXCEPT
    {
        return !operator== (that);
    }

    /*!
     * Conversion to an unspecified boolean type
     *
     * \return \c true, if the <tt>*this</tt> identifies a log record, \c false, if the <tt>*this</tt> is not valid
     */
    BOOST_EXPLICIT_OPERATOR_BOOL()

    /*!
     * Inverted conversion to an unspecified boolean type
     *
     * \return \c false, if the <tt>*this</tt> identifies a log record, \c true, if the <tt>*this</tt> is not valid
     */
    bool operator! () const BOOST_NOEXCEPT
    {
        return !m_impl;
    }

    /*!
     * Swaps two handles
     *
     * \param that Another record to swap with
     * <b>Throws:</b> Nothing
     */
    void swap(record_view& that) BOOST_NOEXCEPT
    {
        m_impl.swap(that.m_impl);
    }

    /*!
     * Resets the log record handle. If there are no other handles left, the log record is closed
     * and all resources referenced by the record are released.
     *
     * \post <tt>!*this == true</tt>
     */
    void reset() BOOST_NOEXCEPT
    {
        m_impl.reset();
    }

    /*!
     * Attribute value lookup.
     *
     * \param keyword Attribute keyword.
     * \return A \c value_ref with extracted attribute value if it is found, empty \c value_ref otherwise.
     */
    template< typename DescriptorT, template< typename > class ActorT >
    typename result_of::extract< typename expressions::attribute_keyword< DescriptorT, ActorT >::value_type, DescriptorT >::type
    operator[] (expressions::attribute_keyword< DescriptorT, ActorT > const& keyword) const
    {
        return m_impl->m_attribute_values[keyword];
    }
};

/*!
 * A free-standing swap function overload for \c record_view
 */
inline void swap(record_view& left, record_view& right) BOOST_NOEXCEPT
{
    left.swap(right);
}

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_CORE_RECORD_VIEW_HPP_INCLUDED_

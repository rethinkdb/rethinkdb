/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   util_exception_handler.cpp
 * \author Andrey Semashev
 * \date   13.07.2009
 *
 * \brief  This header contains tests for the exception handler functional objects.
 */

#define BOOST_TEST_MODULE util_exception_handler

#include <memory>
#include <string>
#include <typeinfo>
#include <stdexcept>
#include <boost/mpl/vector.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/log/utility/exception_handler.hpp>

namespace logging = boost::log;

namespace {

    struct my_handler1
    {
        typedef void result_type;

        std::type_info const*& m_pExceptionType;

        my_handler1(std::type_info const*& p) : m_pExceptionType(p) {}

        void operator() (std::exception&) const
        {
            m_pExceptionType = &typeid(std::exception);
        }
        void operator() (std::runtime_error&) const
        {
            m_pExceptionType = &typeid(std::runtime_error);
        }
    };

    struct my_handler2
    {
        typedef void result_type;
        typedef boost::mpl::vector< std::runtime_error, std::exception >::type exception_types;

        std::type_info const*& m_pExceptionType;

        explicit my_handler2(std::type_info const*& p) : m_pExceptionType(p) {}

        void operator() (std::exception&) const
        {
            m_pExceptionType = &typeid(std::exception);
        }
        void operator() (std::runtime_error&) const
        {
            m_pExceptionType = &typeid(std::runtime_error);
        }
    };

    struct my_handler1_nothrow
    {
        typedef void result_type;

        std::type_info const*& m_pExceptionType;

        my_handler1_nothrow(std::type_info const*& p) : m_pExceptionType(p) {}

        void operator() (std::exception&) const
        {
            m_pExceptionType = &typeid(std::exception);
        }
        void operator() (std::runtime_error&) const
        {
            m_pExceptionType = &typeid(std::runtime_error);
        }
        void operator() () const
        {
            m_pExceptionType = &typeid(void);
        }
    };

    struct my_handler2_nothrow
    {
        typedef void result_type;
        typedef boost::mpl::vector< std::runtime_error, std::exception >::type exception_types;

        std::type_info const*& m_pExceptionType;

        explicit my_handler2_nothrow(std::type_info const*& p) : m_pExceptionType(p) {}

        void operator() (std::exception&) const
        {
            m_pExceptionType = &typeid(std::exception);
        }
        void operator() (std::runtime_error&) const
        {
            m_pExceptionType = &typeid(std::runtime_error);
        }
        void operator() () const
        {
            m_pExceptionType = &typeid(void);
        }
    };

    struct my_exception {};

    struct my_function0
    {
        struct impl_base
        {
            virtual ~impl_base() {}
            virtual void invoke() = 0;
        };

        template< typename T >
        struct impl : public impl_base
        {
            T m_Fun;

            explicit impl(T const& fun) : m_Fun(fun) {}
            void invoke() { m_Fun(); }
        };

    private:
        std::auto_ptr< impl_base > m_pImpl;

    public:
        template< typename T >
        my_function0& operator= (T const& fun)
        {
            m_pImpl.reset(new impl< T >(fun));
            return *this;
        }

        void operator() () const
        {
            m_pImpl->invoke();
        }
    };

} // namespace

// Tests for handler with explicit exception types specification
BOOST_AUTO_TEST_CASE(explicit_exception_types)
{
    std::type_info const* pExceptionType = 0;
    my_function0 handler;
    handler = logging::make_exception_handler<
        std::runtime_error,
        std::exception
    >(my_handler1(pExceptionType));

    try
    {
        throw std::runtime_error("error");
    }
    catch (...)
    {
        handler();
    }
    BOOST_REQUIRE(pExceptionType != 0);
    BOOST_CHECK(*pExceptionType == typeid(std::runtime_error));
    pExceptionType = 0;

    try
    {
        throw std::logic_error("error");
    }
    catch (...)
    {
        handler();
    }
    BOOST_REQUIRE(pExceptionType != 0);
    BOOST_CHECK(*pExceptionType == typeid(std::exception));
    pExceptionType = 0;

    try
    {
        throw my_exception();
    }
    catch (...)
    {
        BOOST_CHECK_THROW(handler(), my_exception);
    }
    BOOST_REQUIRE(pExceptionType == 0);

    // Verify that exception types are checked in the specified order
    handler = logging::make_exception_handler<
        std::exception,
        std::runtime_error
    >(my_handler1(pExceptionType));

    try
    {
        throw std::runtime_error("error");
    }
    catch (...)
    {
        handler();
    }
    BOOST_REQUIRE(pExceptionType != 0);
    BOOST_CHECK(*pExceptionType == typeid(std::exception));
    pExceptionType = 0;
}

// Tests for handler with explicit exception types specification (no-throw version)
BOOST_AUTO_TEST_CASE(explicit_exception_types_nothrow)
{
    std::type_info const* pExceptionType = 0;
    my_function0 handler;
    handler = logging::make_exception_handler<
        std::runtime_error,
        std::exception
    >(my_handler1_nothrow(pExceptionType), std::nothrow);

    try
    {
        throw std::runtime_error("error");
    }
    catch (...)
    {
        handler();
    }
    BOOST_REQUIRE(pExceptionType != 0);
    BOOST_CHECK(*pExceptionType == typeid(std::runtime_error));
    pExceptionType = 0;

    try
    {
        throw std::logic_error("error");
    }
    catch (...)
    {
        handler();
    }
    BOOST_REQUIRE(pExceptionType != 0);
    BOOST_CHECK(*pExceptionType == typeid(std::exception));
    pExceptionType = 0;

    try
    {
        throw my_exception();
    }
    catch (...)
    {
        BOOST_CHECK_NO_THROW(handler());
    }
    BOOST_REQUIRE(pExceptionType != 0);
    BOOST_CHECK(*pExceptionType == typeid(void));
    pExceptionType = 0;

    // Verify that exception types are checked in the specified order
    handler = logging::make_exception_handler<
        std::exception,
        std::runtime_error
    >(my_handler1_nothrow(pExceptionType), std::nothrow);

    try
    {
        throw std::runtime_error("error");
    }
    catch (...)
    {
        handler();
    }
    BOOST_REQUIRE(pExceptionType != 0);
    BOOST_CHECK(*pExceptionType == typeid(std::exception));
    pExceptionType = 0;
}

// Tests for handler with self-contained exception types
BOOST_AUTO_TEST_CASE(self_contained_exception_types)
{
    std::type_info const* pExceptionType = 0;
    my_function0 handler;
    handler = logging::make_exception_handler(my_handler2(pExceptionType));

    try
    {
        throw std::runtime_error("error");
    }
    catch (...)
    {
        handler();
    }
    BOOST_REQUIRE(pExceptionType != 0);
    BOOST_CHECK(*pExceptionType == typeid(std::runtime_error));
    pExceptionType = 0;

    try
    {
        throw std::logic_error("error");
    }
    catch (...)
    {
        handler();
    }
    BOOST_REQUIRE(pExceptionType != 0);
    BOOST_CHECK(*pExceptionType == typeid(std::exception));
    pExceptionType = 0;

    try
    {
        throw my_exception();
    }
    catch (...)
    {
        BOOST_CHECK_THROW(handler(), my_exception);
    }
    BOOST_REQUIRE(pExceptionType == 0);
}

// Tests for handler with self-contained exception types (no-throw version)
BOOST_AUTO_TEST_CASE(self_contained_exception_types_nothrow)
{
    std::type_info const* pExceptionType = 0;
    my_function0 handler;
    handler = logging::make_exception_handler(my_handler2_nothrow(pExceptionType), std::nothrow);

    try
    {
        throw std::runtime_error("error");
    }
    catch (...)
    {
        handler();
    }
    BOOST_REQUIRE(pExceptionType != 0);
    BOOST_CHECK(*pExceptionType == typeid(std::runtime_error));
    pExceptionType = 0;

    try
    {
        throw std::logic_error("error");
    }
    catch (...)
    {
        handler();
    }
    BOOST_REQUIRE(pExceptionType != 0);
    BOOST_CHECK(*pExceptionType == typeid(std::exception));
    pExceptionType = 0;

    try
    {
        throw my_exception();
    }
    catch (...)
    {
        BOOST_CHECK_NO_THROW(handler());
    }
    BOOST_REQUIRE(pExceptionType != 0);
    BOOST_CHECK(*pExceptionType == typeid(void));
    pExceptionType = 0;
}

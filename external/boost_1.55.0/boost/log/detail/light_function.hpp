/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   light_function.hpp
 * \author Andrey Semashev
 * \date   20.06.2010
 *
 * \brief  This header is the Boost.Log library impl, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 *
 * The file contains a lightweight alternative of Boost.Function. It does not provide all
 * features of Boost.Function but doesn't introduce dependency on Boost.Bind.
 */

#ifndef BOOST_LOG_DETAIL_LIGHT_FUNCTION_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_LIGHT_FUNCTION_HPP_INCLUDED_

#include <cstddef>
#include <boost/move/core.hpp>
#include <boost/move/utility.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/utility/explicit_operator_bool.hpp>
#include <boost/type_traits/remove_cv.hpp>
#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_binary_params.hpp>
#endif
#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/or.hpp>
#else
#include <boost/type_traits/remove_reference.hpp>
#endif
#if defined(BOOST_NO_CXX11_NULLPTR)
#include <boost/assert.hpp>
#endif
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#ifndef BOOST_LOG_LIGHT_FUNCTION_LIMIT
#define BOOST_LOG_LIGHT_FUNCTION_LIMIT 2
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

template< typename SignatureT >
class light_function;

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

template< typename ResultT, typename... ArgsT >
class light_function< ResultT (ArgsT...) >
{
    typedef light_function this_type;
    BOOST_COPYABLE_AND_MOVABLE(this_type)

public:
    typedef ResultT result_type;

private:
    struct impl_base
    {
        typedef result_type (*invoke_type)(impl_base*, ArgsT...);
        const invoke_type invoke;

        typedef impl_base* (*clone_type)(const impl_base*);
        const clone_type clone;

        typedef void (*destroy_type)(impl_base*);
        const destroy_type destroy;

        impl_base(invoke_type inv, clone_type cl, destroy_type dstr) : invoke(inv), clone(cl), destroy(dstr)
        {
        }
    };

#if !defined(BOOST_LOG_NO_MEMBER_TEMPLATE_FRIENDS)
    template< typename FunT >
    class impl;
    template< typename FunT >
    friend class impl;
#endif

    template< typename FunT >
    class impl :
        public impl_base
    {
        typedef impl< FunT > this_type;

        FunT m_Function;

    public:
        explicit impl(FunT const& fun) :
            impl_base(&this_type::invoke_impl, &this_type::clone_impl, &this_type::destroy_impl),
            m_Function(fun)
        {
        }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        explicit impl(FunT&& fun) :
            impl_base(&this_type::invoke_impl, &this_type::clone_impl, &this_type::destroy_impl),
            m_Function(fun)
        {
        }
#endif // !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

        static void destroy_impl(impl_base* self)
        {
            delete static_cast< impl* >(self);
        }
        static impl_base* clone_impl(const impl_base* self)
        {
            return new impl(static_cast< const impl* >(self)->m_Function);
        }
        static result_type invoke_impl(impl_base* self, ArgsT... args)
        {
            return static_cast< impl* >(self)->m_Function(args...);
        }
    };

private:
    impl_base* m_pImpl;

public:
    BOOST_CONSTEXPR light_function() BOOST_NOEXCEPT : m_pImpl(NULL)
    {
    }
    light_function(this_type const& that)
    {
        if (that.m_pImpl)
            m_pImpl = that.m_pImpl->clone(that.m_pImpl);
        else
            m_pImpl = NULL;
    }

    light_function(BOOST_RV_REF(this_type) that) BOOST_NOEXCEPT
    {
        m_pImpl = that.m_pImpl;
        that.m_pImpl = NULL;
    }

    light_function(BOOST_RV_REF(const this_type) that) BOOST_NOEXCEPT
    {
        m_pImpl = that.m_pImpl;
        ((this_type&)that).m_pImpl = NULL;
    }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template< typename FunT >
    light_function(FunT&& fun) :
        m_pImpl(new impl< typename remove_cv< typename remove_reference< FunT >::type >::type >(boost::forward< FunT >(fun)))
    {
    }
#else
    template< typename FunT >
    light_function(FunT const& fun, typename disable_if< mpl::or_< move_detail::is_rv< FunT >, is_same< FunT, this_type > >, int >::type = 0) :
        m_pImpl(new impl< FunT >(fun))
    {
    }
    template< typename FunT >
    light_function(rv< FunT > const& fun, typename disable_if< is_same< typename remove_cv< FunT >::type, this_type >, int >::type = 0) :
        m_pImpl(new impl< typename remove_cv< FunT >::type >(fun))
    {
    }
#endif

    //! Constructor from NULL
#if !defined(BOOST_NO_CXX11_NULLPTR)
    BOOST_CONSTEXPR light_function(std::nullptr_t) BOOST_NOEXCEPT
#else
    BOOST_CONSTEXPR light_function(int p) BOOST_NOEXCEPT
#endif
        : m_pImpl(NULL)
    {
#if defined(BOOST_NO_CXX11_NULLPTR)
        BOOST_ASSERT(p == 0);
#endif
    }
    ~light_function()
    {
        clear();
    }

    light_function& operator= (BOOST_RV_REF(this_type) that) BOOST_NOEXCEPT
    {
        this->swap(that);
        return *this;
    }
    light_function& operator= (BOOST_COPY_ASSIGN_REF(this_type) that)
    {
        light_function tmp(that);
        this->swap(tmp);
        return *this;
    }
    //! Assignment of NULL
#if !defined(BOOST_NO_CXX11_NULLPTR)
    light_function& operator= (std::nullptr_t)
#else
    light_function& operator= (int p)
#endif
    {
#if defined(BOOST_NO_CXX11_NULLPTR)
        BOOST_ASSERT(p == 0);
#endif
        clear();
        return *this;
    }
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template< typename FunT >
    light_function& operator= (FunT&& fun)
    {
        light_function tmp(boost::forward< FunT >(fun));
        this->swap(tmp);
        return *this;
    }
#else
    template< typename FunT >
    typename disable_if< mpl::or_< move_detail::is_rv< FunT >, is_same< FunT, this_type > >, this_type& >::type
    operator= (FunT const& fun)
    {
        light_function tmp(fun);
        this->swap(tmp);
        return *this;
    }
#endif

    result_type operator() (ArgsT... args) const
    {
        return m_pImpl->invoke(m_pImpl, args...);
    }

    BOOST_EXPLICIT_OPERATOR_BOOL()
    bool operator! () const BOOST_NOEXCEPT { return (m_pImpl == NULL); }
    bool empty() const BOOST_NOEXCEPT { return (m_pImpl == NULL); }
    void clear() BOOST_NOEXCEPT
    {
        if (m_pImpl)
        {
            m_pImpl->destroy(m_pImpl);
            m_pImpl = NULL;
        }
    }

    void swap(this_type& that) BOOST_NOEXCEPT
    {
        register impl_base* p = m_pImpl;
        m_pImpl = that.m_pImpl;
        that.m_pImpl = p;
    }
};

template< typename... ArgsT >
class light_function< void (ArgsT...) >
{
    typedef light_function this_type;
    BOOST_COPYABLE_AND_MOVABLE(this_type)

public:
    typedef void result_type;

private:
    struct impl_base
    {
        typedef void (*invoke_type)(impl_base*, ArgsT...);
        const invoke_type invoke;

        typedef impl_base* (*clone_type)(const impl_base*);
        const clone_type clone;

        typedef void (*destroy_type)(impl_base*);
        const destroy_type destroy;

        impl_base(invoke_type inv, clone_type cl, destroy_type dstr) : invoke(inv), clone(cl), destroy(dstr)
        {
        }
    };

#if !defined(BOOST_LOG_NO_MEMBER_TEMPLATE_FRIENDS)
    template< typename FunT >
    class impl;
    template< typename FunT >
    friend class impl;
#endif

    template< typename FunT >
    class impl :
        public impl_base
    {
        typedef impl< FunT > this_type;

        FunT m_Function;

    public:
        explicit impl(FunT const& fun) :
            impl_base(&this_type::invoke_impl, &this_type::clone_impl, &this_type::destroy_impl),
            m_Function(fun)
        {
        }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        explicit impl(FunT&& fun) :
            impl_base(&this_type::invoke_impl, &this_type::clone_impl, &this_type::destroy_impl),
            m_Function(fun)
        {
        }
#endif // !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

        static void destroy_impl(impl_base* self)
        {
            delete static_cast< impl* >(self);
        }
        static impl_base* clone_impl(const impl_base* self)
        {
            return new impl(static_cast< const impl* >(self)->m_Function);
        }
        static result_type invoke_impl(impl_base* self, ArgsT... args)
        {
            static_cast< impl* >(self)->m_Function(args...);
        }
    };

private:
    impl_base* m_pImpl;

public:
    BOOST_CONSTEXPR light_function() BOOST_NOEXCEPT : m_pImpl(NULL)
    {
    }
    light_function(this_type const& that)
    {
        if (that.m_pImpl)
            m_pImpl = that.m_pImpl->clone(that.m_pImpl);
        else
            m_pImpl = NULL;
    }
    light_function(BOOST_RV_REF(this_type) that) BOOST_NOEXCEPT
    {
        m_pImpl = that.m_pImpl;
        that.m_pImpl = NULL;
    }

    light_function(BOOST_RV_REF(const this_type) that) BOOST_NOEXCEPT
    {
        m_pImpl = that.m_pImpl;
        ((this_type&)that).m_pImpl = NULL;
    }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template< typename FunT >
    light_function(FunT&& fun) :
        m_pImpl(new impl< typename remove_cv< typename remove_reference< FunT >::type >::type >(boost::forward< FunT >(fun)))
    {
    }
#else
    template< typename FunT >
    light_function(FunT const& fun, typename disable_if< mpl::or_< move_detail::is_rv< FunT >, is_same< FunT, this_type > >, int >::type = 0) :
        m_pImpl(new impl< FunT >(fun))
    {
    }
    template< typename FunT >
    light_function(rv< FunT > const& fun, typename disable_if< is_same< typename remove_cv< FunT >::type, this_type >, int >::type = 0) :
        m_pImpl(new impl< typename remove_cv< FunT >::type >(fun))
    {
    }
#endif

    //! Constructor from NULL
#if !defined(BOOST_NO_CXX11_NULLPTR)
    BOOST_CONSTEXPR light_function(std::nullptr_t) BOOST_NOEXCEPT
#else
    BOOST_CONSTEXPR light_function(int p) BOOST_NOEXCEPT
#endif
        : m_pImpl(NULL)
    {
#if defined(BOOST_NO_CXX11_NULLPTR)
        BOOST_ASSERT(p == 0);
#endif
    }
    ~light_function()
    {
        clear();
    }

    light_function& operator= (BOOST_RV_REF(this_type) that) BOOST_NOEXCEPT
    {
        this->swap(that);
        return *this;
    }
    light_function& operator= (BOOST_COPY_ASSIGN_REF(this_type) that)
    {
        light_function tmp = that;
        this->swap(tmp);
        return *this;
    }
    //! Assignment of NULL
#if !defined(BOOST_NO_CXX11_NULLPTR)
    light_function& operator= (std::nullptr_t)
#else
    light_function& operator= (int p)
#endif
    {
#if defined(BOOST_NO_CXX11_NULLPTR)
        BOOST_ASSERT(p == 0);
#endif
        clear();
        return *this;
    }
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template< typename FunT >
    light_function& operator= (FunT&& fun)
    {
        light_function tmp(boost::forward< FunT >(fun));
        this->swap(tmp);
        return *this;
    }
#else
    template< typename FunT >
    typename disable_if< mpl::or_< move_detail::is_rv< FunT >, is_same< FunT, this_type > >, this_type& >::type
    operator= (FunT const& fun)
    {
        light_function tmp(fun);
        this->swap(tmp);
        return *this;
    }
#endif

    result_type operator() (ArgsT... args) const
    {
        m_pImpl->invoke(m_pImpl, args...);
    }

    BOOST_EXPLICIT_OPERATOR_BOOL()
    bool operator! () const BOOST_NOEXCEPT { return (m_pImpl == NULL); }
    bool empty() const BOOST_NOEXCEPT { return (m_pImpl == NULL); }
    void clear() BOOST_NOEXCEPT
    {
        if (m_pImpl)
        {
            m_pImpl->destroy(m_pImpl);
            m_pImpl = NULL;
        }
    }

    void swap(this_type& that) BOOST_NOEXCEPT
    {
        register impl_base* p = m_pImpl;
        m_pImpl = that.m_pImpl;
        that.m_pImpl = p;
    }
};

#else // !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#define BOOST_PP_FILENAME_1 <boost/log/detail/light_function_pp.hpp>
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_LOG_LIGHT_FUNCTION_LIMIT)
#include BOOST_PP_ITERATE()

#endif // !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

template< typename SignatureT >
inline void swap(light_function< SignatureT >& left, light_function< SignatureT >& right)
{
    left.swap(right);
}

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_LIGHT_FUNCTION_HPP_INCLUDED_

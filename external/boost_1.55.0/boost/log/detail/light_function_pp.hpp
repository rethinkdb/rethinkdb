/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

template<
    typename ResultT
    BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(), typename ArgT)
>
class light_function< ResultT (BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), ArgT)) >
{
    typedef light_function this_type;
    BOOST_COPYABLE_AND_MOVABLE(this_type)

public:
    typedef ResultT result_type;

private:
    struct impl_base
    {
        typedef result_type (*invoke_type)(impl_base* BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(), ArgT));
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
        static result_type invoke_impl(impl_base* self BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(BOOST_PP_ITERATION(), ArgT, arg))
        {
            return static_cast< impl* >(self)->m_Function(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), arg));
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

    result_type operator() (BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), ArgT, arg)) const
    {
        return m_pImpl->invoke(m_pImpl BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(), arg));
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

template<
    BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), typename ArgT)
>
class light_function< void (BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), ArgT)) >
{
    typedef light_function this_type;
    BOOST_COPYABLE_AND_MOVABLE(this_type)

public:
    typedef void result_type;

private:
    struct impl_base
    {
        typedef void (*invoke_type)(impl_base* BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(), ArgT));
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
        static result_type invoke_impl(impl_base* self BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(BOOST_PP_ITERATION(), ArgT, arg))
        {
            static_cast< impl* >(self)->m_Function(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), arg));
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

    result_type operator() (BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), ArgT, arg)) const
    {
        m_pImpl->invoke(m_pImpl BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_ITERATION(), arg));
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

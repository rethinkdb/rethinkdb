
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_UNIDIRECT_COROUTINE_H
#define BOOST_COROUTINES_UNIDIRECT_COROUTINE_H

#include <cstddef>
#include <iterator>
#include <memory>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/optional.hpp>
#include <boost/range.hpp>
#include <boost/throw_exception.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/type_traits/function_traits.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>

#include <boost/coroutine/attributes.hpp>
#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/detail/coroutine_context.hpp>
#include <boost/coroutine/detail/param.hpp>
#include <boost/coroutine/exceptions.hpp>
#include <boost/coroutine/stack_allocator.hpp>
#include <boost/coroutine/v2/detail/pull_coroutine_base.hpp>
#include <boost/coroutine/v2/detail/pull_coroutine_caller.hpp>
#include <boost/coroutine/v2/detail/pull_coroutine_object.hpp>
#include <boost/coroutine/v2/detail/push_coroutine_base.hpp>
#include <boost/coroutine/v2/detail/push_coroutine_caller.hpp>
#include <boost/coroutine/v2/detail/push_coroutine_object.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {

template< typename Arg >
class pull_coroutine;

template< typename Arg >
class push_coroutine
{
private:
    template<
        typename X, typename Y, typename Z, typename V, typename W
    >
    friend class detail::pull_coroutine_object;

    typedef detail::push_coroutine_base< Arg >  base_t;
    typedef typename base_t::ptr_t              ptr_t;

    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    ptr_t  impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( push_coroutine)

    template< typename Allocator >
    push_coroutine( detail::coroutine_context const& callee,
                    bool unwind, bool preserve_fpu,
                    Allocator const& alloc) :
        impl_()
    {
        typedef detail::push_coroutine_caller<
                Arg, Allocator
        >                               caller_t;
        typename caller_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) caller_t(
                callee, unwind, preserve_fpu, a) );
    }

public:
    push_coroutine() BOOST_NOEXCEPT :
        impl_()
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#ifdef BOOST_MSVC
    typedef void ( * coroutine_fn) ( pull_coroutine< Arg > &);

    explicit push_coroutine( coroutine_fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< coroutine_fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

	template< typename StackAllocator >
    explicit push_coroutine( coroutine_fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< coroutine_fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

    template< typename StackAllocator, typename Allocator >
    explicit push_coroutine(coroutine_fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< coroutine_fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);
#endif
    template< typename Fn >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);
#else
    template< typename Fn >
    explicit push_coroutine( Fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit push_coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0);

    template< typename Fn >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);
#endif

    push_coroutine( BOOST_RV_REF( push_coroutine) other) BOOST_NOEXCEPT :
        impl_()
    { swap( other); }

    push_coroutine & operator=( BOOST_RV_REF( push_coroutine) other) BOOST_NOEXCEPT
    {
        push_coroutine tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    bool empty() const BOOST_NOEXCEPT
    { return ! impl_; }

    operator safe_bool() const BOOST_NOEXCEPT
    { return ( empty() || impl_->is_complete() ) ? 0 : & dummy::nonnull; }

    bool operator!() const BOOST_NOEXCEPT
    { return empty() || impl_->is_complete(); }

    void swap( push_coroutine & other) BOOST_NOEXCEPT
    { impl_.swap( other.impl_); }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    push_coroutine & operator()( Arg const& arg)
    {
        BOOST_ASSERT( * this);

        impl_->push( arg);
        return * this;
    }

    push_coroutine & operator()( Arg && arg) {
        BOOST_ASSERT( * this);

        impl_->push( arg);
        return * this;
    }
#else
    push_coroutine & operator()( Arg const& arg)
    {
        BOOST_ASSERT( * this);

        impl_->push( forward< Arg >( arg) );
        return * this;
    }

    push_coroutine & operator()( BOOST_RV_REF( Arg) arg)
    {
        BOOST_ASSERT( * this);

        impl_->push( forward< Arg >( arg) );
        return * this;
    }
#endif

    class iterator : public std::iterator< std::output_iterator_tag, void, void, void, void >
    {
    private:
       push_coroutine< Arg >    *   c_;

    public:
        iterator() :
           c_( 0)
        {}

        explicit iterator( push_coroutine< Arg > * c) :
            c_( c)
        {}

        iterator & operator=( Arg a)
        {
            BOOST_ASSERT( c_);
            if ( ! ( * c_)( a) ) c_ = 0;
            return * this;
        }

        bool operator==( iterator const& other)
        { return other.c_ == c_; }

        bool operator!=( iterator const& other)
        { return other.c_ != c_; }

        iterator & operator*()
        { return * this; }

        iterator & operator++()
        { return * this; }
    };

    struct const_iterator;
};

template< typename Arg >
class push_coroutine< Arg & >
{
private:
    template<
        typename X, typename Y, typename Z, typename V, typename W
    >
    friend class detail::pull_coroutine_object;

    typedef detail::push_coroutine_base< Arg & >    base_t;
    typedef typename base_t::ptr_t                  ptr_t;

    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    ptr_t  impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( push_coroutine)

    template< typename Allocator >
    push_coroutine( detail::coroutine_context const& callee,
                    bool unwind, bool preserve_fpu,
                    Allocator const& alloc) :
        impl_()
    {
        typedef detail::push_coroutine_caller<
                Arg &, Allocator
        >                               caller_t;
        typename caller_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) caller_t(
                callee, unwind, preserve_fpu, a) );
    }

public:
    push_coroutine() BOOST_NOEXCEPT :
        impl_()
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#ifdef BOOST_MSVC
    typedef void ( * coroutine_fn) ( pull_coroutine< Arg & > &);

    explicit push_coroutine( coroutine_fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< coroutine_fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

    template< typename StackAllocator >
    explicit push_coroutine( coroutine_fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< coroutine_fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

    template< typename StackAllocator, typename Allocator >
    explicit push_coroutine( coroutine_fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< coroutine_fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);
#endif
    template< typename Fn >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);
#else
    template< typename Fn >
    explicit push_coroutine( Fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit push_coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0);

    template< typename Fn >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);
#endif

    push_coroutine( BOOST_RV_REF( push_coroutine) other) BOOST_NOEXCEPT :
        impl_()
    { swap( other); }

    push_coroutine & operator=( BOOST_RV_REF( push_coroutine) other) BOOST_NOEXCEPT
    {
        push_coroutine tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    bool empty() const BOOST_NOEXCEPT
    { return ! impl_; }

    operator safe_bool() const BOOST_NOEXCEPT
    { return ( empty() || impl_->is_complete() ) ? 0 : & dummy::nonnull; }

    bool operator!() const BOOST_NOEXCEPT
    { return empty() || impl_->is_complete(); }

    void swap( push_coroutine & other) BOOST_NOEXCEPT
    { impl_.swap( other.impl_); }

    push_coroutine & operator()( Arg & arg)
    {
        BOOST_ASSERT( * this);

        impl_->push( arg);
        return * this;
    }

    class iterator : public std::iterator< std::output_iterator_tag, void, void, void, void >
    {
    private:
       push_coroutine< Arg & >    *   c_;

    public:
        iterator() :
           c_( 0)
        {}

        explicit iterator( push_coroutine< Arg & > * c) :
            c_( c)
        {}

        iterator & operator=( Arg & a)
        {
            BOOST_ASSERT( c_);
            if ( ! ( * c_)( a) ) c_ = 0;
            return * this;
        }

        bool operator==( iterator const& other)
        { return other.c_ == c_; }

        bool operator!=( iterator const& other)
        { return other.c_ != c_; }

        iterator & operator*()
        { return * this; }

        iterator & operator++()
        { return * this; }
    };

    struct const_iterator;
};

template<>
class push_coroutine< void >
{
private:
    template<
        typename X, typename Y, typename Z, typename V, typename W
    >
    friend class detail::pull_coroutine_object;

    typedef detail::push_coroutine_base< void >  base_t;
    typedef base_t::ptr_t                        ptr_t;

    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    ptr_t  impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( push_coroutine)

    template< typename Allocator >
    push_coroutine( detail::coroutine_context const& callee,
                    bool unwind, bool preserve_fpu,
                    Allocator const& alloc) :
        impl_()
    {
        typedef detail::push_coroutine_caller<
                void, Allocator
        >                               caller_t;
        typename caller_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) caller_t(
                callee, unwind, preserve_fpu, a) );
    }

public:
    push_coroutine() BOOST_NOEXCEPT :
        impl_()
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#ifdef BOOST_MSVC
    typedef void ( * coroutine_fn) ( pull_coroutine< void > &);

    explicit push_coroutine( coroutine_fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               disable_if<
                   is_same< typename decay< coroutine_fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

	template< typename StackAllocator >
    explicit push_coroutine( coroutine_fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               disable_if<
                   is_same< typename decay< coroutine_fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

    template< typename StackAllocator, typename Allocator >
    explicit push_coroutine( coroutine_fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               disable_if<
                   is_same< typename decay< coroutine_fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);
#endif
    template< typename Fn >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
              typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);
#else
    template< typename Fn >
    explicit push_coroutine( Fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit push_coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0);

    template< typename Fn >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< push_coroutine > const& alloc =
                    std::allocator< push_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, push_coroutine >,
                   dummy *
               >::type = 0);
#endif

    push_coroutine( BOOST_RV_REF( push_coroutine) other) BOOST_NOEXCEPT :
        impl_()
    { swap( other); }

    push_coroutine & operator=( BOOST_RV_REF( push_coroutine) other) BOOST_NOEXCEPT
    {
        push_coroutine tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    bool empty() const BOOST_NOEXCEPT
    { return ! impl_; }

    operator safe_bool() const BOOST_NOEXCEPT
    { return ( empty() || impl_->is_complete() ) ? 0 : & dummy::nonnull; }

    bool operator!() const BOOST_NOEXCEPT
    { return empty() || impl_->is_complete(); }

    void swap( push_coroutine & other) BOOST_NOEXCEPT
    { impl_.swap( other.impl_); }

    push_coroutine & operator()()
    {
        BOOST_ASSERT( * this);

        impl_->push();
        return * this;
    }

    struct iterator;
    struct const_iterator;
};



template< typename R >
class pull_coroutine
{
private:
    template<
        typename X, typename Y, typename Z, typename V, typename W
    >
    friend class detail::push_coroutine_object;

    typedef detail::pull_coroutine_base< R >    base_t;
    typedef typename base_t::ptr_t              ptr_t;

    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    ptr_t  impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( pull_coroutine)

    template< typename Allocator >
    pull_coroutine( detail::coroutine_context const& callee,
                    bool unwind, bool preserve_fpu,
                    Allocator const& alloc,
                    optional< R > const& result) :
        impl_()
    {
        typedef detail::pull_coroutine_caller<
                R, Allocator
        >                               caller_t;
        typename caller_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) caller_t(
                callee, unwind, preserve_fpu, a, result) );
    }

public:
    pull_coroutine() BOOST_NOEXCEPT :
        impl_()
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#ifdef BOOST_MSVC
    typedef void ( * coroutine_fn) ( push_coroutine< R > &);

    explicit pull_coroutine( coroutine_fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_same< typename decay< coroutine_fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R, coroutine_fn, stack_allocator, std::allocator< pull_coroutine >,
                push_coroutine< R >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
    }

    template< typename StackAllocator >
    explicit pull_coroutine( coroutine_fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_same< typename decay< coroutine_fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R, coroutine_fn, StackAllocator, std::allocator< pull_coroutine >,
                push_coroutine< R >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
    }

    template< typename StackAllocator, typename Allocator >
    explicit pull_coroutine( coroutine_fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< coroutine_fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R, coroutine_fn, StackAllocator, Allocator,
                push_coroutine< R >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
    }
#endif
    template< typename Fn >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R, Fn, stack_allocator, std::allocator< pull_coroutine >,
                push_coroutine< R >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R, Fn, StackAllocator, std::allocator< pull_coroutine >,
                push_coroutine< R >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R, Fn, StackAllocator, Allocator,
                push_coroutine< R >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }
#else
    template< typename Fn >
    explicit pull_coroutine( Fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R, Fn, stack_allocator, std::allocator< pull_coroutine >,
                push_coroutine< R >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R, Fn, StackAllocator, std::allocator< pull_coroutine >,
                push_coroutine< R >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit pull_coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R, Fn, StackAllocator, Allocator,
                push_coroutine< R >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R, Fn, stack_allocator, std::allocator< pull_coroutine >,
                push_coroutine< R >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R, Fn, StackAllocator, std::allocator< pull_coroutine >,
                push_coroutine< R >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R, Fn, StackAllocator, Allocator,
                push_coroutine< R >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }
#endif

    pull_coroutine( BOOST_RV_REF( pull_coroutine) other) BOOST_NOEXCEPT :
        impl_()
    { swap( other); }

    pull_coroutine & operator=( BOOST_RV_REF( pull_coroutine) other) BOOST_NOEXCEPT
    {
        pull_coroutine tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    bool empty() const BOOST_NOEXCEPT
    { return ! impl_; }

    operator safe_bool() const BOOST_NOEXCEPT
    { return ( empty() || impl_->is_complete() ) ? 0 : & dummy::nonnull; }

    bool operator!() const BOOST_NOEXCEPT
    { return empty() || impl_->is_complete(); }

    void swap( pull_coroutine & other) BOOST_NOEXCEPT
    { impl_.swap( other.impl_); }

    pull_coroutine & operator()()
    {
        BOOST_ASSERT( * this);

        impl_->pull();
        return * this;
    }

    bool has_result() const
    {
        BOOST_ASSERT( ! empty() );

        return impl_->has_result();
    }

    R get() const
    {
        BOOST_ASSERT( ! empty() );

        return impl_->get();
    }

    class iterator : public std::iterator< std::input_iterator_tag, typename remove_reference< R >::type >
    {
    private:
        pull_coroutine< R > *   c_;
        optional< R >           val_;

        void fetch_()
        {
            BOOST_ASSERT( c_);

            if ( ! c_->has_result() )
            {
                c_ = 0;
                val_ = none;
                return;
            }
            val_ = c_->get();
        }

        void increment_()
        {
            BOOST_ASSERT( c_);
            BOOST_ASSERT( * c_);

            ( * c_)();
            fetch_();
        }

    public:
        typedef typename iterator::pointer      pointer_t;
        typedef typename iterator::reference    reference_t;

        iterator() :
            c_( 0), val_()
        {}

        explicit iterator( pull_coroutine< R > * c) :
            c_( c), val_()
        { fetch_(); }

        iterator( iterator const& other) :
            c_( other.c_), val_( other.val_)
        {}

        iterator & operator=( iterator const& other)
        {
            if ( this == & other) return * this;
            c_ = other.c_;
            val_ = other.val_;
            return * this;
        }

        bool operator==( iterator const& other)
        { return other.c_ == c_ && other.val_ == val_; }

        bool operator!=( iterator const& other)
        { return other.c_ != c_ || other.val_ != val_; }

        iterator & operator++()
        {
            increment_();
            return * this;
        }

        iterator operator++( int)
        {
            iterator tmp( * this);
            ++*this;
            return tmp;
        }

        reference_t operator*() const
        {
            if ( ! val_)
                boost::throw_exception(
                    invalid_result() );
            return const_cast< optional< R > & >( val_).get();
        }

        pointer_t operator->() const
        {
            if ( ! val_)
                boost::throw_exception(
                    invalid_result() );
            return const_cast< optional< R > & >( val_).get_ptr();
        }
    };

    class const_iterator : public std::iterator< std::input_iterator_tag, const typename remove_reference< R >::type >
    {
    private:
        pull_coroutine< R > *   c_;
        optional< R >           val_;

        void fetch_()
        {
            BOOST_ASSERT( c_);

            if ( ! c_->has_result() )
            {
                c_ = 0;
                val_ = none;
                return;
            }
            val_ = c_->get();
        }

        void increment_()
        {
            BOOST_ASSERT( c_);
            BOOST_ASSERT( * c_);

            ( * c_)();
            fetch_();
        }

    public:
        typedef typename const_iterator::pointer      pointer_t;
        typedef typename const_iterator::reference    reference_t;

        const_iterator() :
            c_( 0), val_()
        {}

        explicit const_iterator( pull_coroutine< R > const* c) :
            c_( const_cast< pull_coroutine< R > * >( c) ), val_()
        { fetch_(); }

        const_iterator( const_iterator const& other) :
            c_( other.c_), val_( other.val_)
        {}

        const_iterator & operator=( const_iterator const& other)
        {
            if ( this == & other) return * this;
            c_ = other.c_;
            val_ = other.val_;
            return * this;
        }

        bool operator==( const_iterator const& other)
        { return other.c_ == c_ && other.val_ == val_; }

        bool operator!=( const_iterator const& other)
        { return other.c_ != c_ || other.val_ != val_; }

        const_iterator & operator++()
        {
            increment_();
            return * this;
        }

        const_iterator operator++( int)
        {
            const_iterator tmp( * this);
            ++*this;
            return tmp;
        }

        reference_t operator*() const
        {
            if ( ! val_)
                boost::throw_exception(
                    invalid_result() );
            return val_.get();
        }

        pointer_t operator->() const
        {
            if ( ! val_)
                boost::throw_exception(
                    invalid_result() );
            return val_.get_ptr();
        }
    };
};

template< typename R >
class pull_coroutine< R & >
{
private:
    template<
        typename X, typename Y, typename Z, typename V, typename W
    >
    friend class detail::push_coroutine_object;

    typedef detail::pull_coroutine_base< R & >  base_t;
    typedef typename base_t::ptr_t              ptr_t;

    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    ptr_t  impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( pull_coroutine)

    template< typename Allocator >
    pull_coroutine( detail::coroutine_context const& callee,
                    bool unwind, bool preserve_fpu,
                    Allocator const& alloc,
                    optional< R * > const& result) :
        impl_()
    {
        typedef detail::pull_coroutine_caller<
                R &, Allocator
        >                               caller_t;
        typename caller_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) caller_t(
                callee, unwind, preserve_fpu, a, result) );
    }

public:
    pull_coroutine() BOOST_NOEXCEPT :
        impl_()
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#ifdef BOOST_MSVC
    typedef void ( * coroutine_fn) ( push_coroutine< R & > &);

    explicit pull_coroutine( coroutine_fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_same< typename decay< coroutine_fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R &, coroutine_fn, stack_allocator, std::allocator< pull_coroutine >,
                push_coroutine< R & >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
    }

    template< typename StackAllocator >
    explicit pull_coroutine( coroutine_fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_same< typename decay< coroutine_fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R &, coroutine_fn, StackAllocator, std::allocator< pull_coroutine >,
                push_coroutine< R & >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
    }

    template< typename StackAllocator, typename Allocator >
    explicit pull_coroutine( coroutine_fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< coroutine_fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R &, coroutine_fn, StackAllocator, Allocator,
                push_coroutine< R & >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
    }
#endif
    template< typename Fn >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R &, Fn, stack_allocator, std::allocator< pull_coroutine >,
                push_coroutine< R & >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R &, Fn, StackAllocator, std::allocator< pull_coroutine >,
                push_coroutine< R & >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R &, Fn, StackAllocator, Allocator,
                push_coroutine< R & >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }
#else
    template< typename Fn >
    explicit pull_coroutine( Fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R &, Fn, stack_allocator, std::allocator< pull_coroutine >,
                push_coroutine< R & >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R &, Fn, StackAllocator, std::allocator< pull_coroutine >,
                push_coroutine< R & >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit pull_coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R &, Fn, StackAllocator, Allocator,
                push_coroutine< R & >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R &, Fn, stack_allocator, std::allocator< pull_coroutine >,
                push_coroutine< R & >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R &, Fn, StackAllocator, std::allocator< pull_coroutine >,
                push_coroutine< R & >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                R &, Fn, StackAllocator, Allocator,
                push_coroutine< R & >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }
#endif

    pull_coroutine( BOOST_RV_REF( pull_coroutine) other) BOOST_NOEXCEPT :
        impl_()
    { swap( other); }

    pull_coroutine & operator=( BOOST_RV_REF( pull_coroutine) other) BOOST_NOEXCEPT
    {
        pull_coroutine tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    bool empty() const BOOST_NOEXCEPT
    { return ! impl_; }

    operator safe_bool() const BOOST_NOEXCEPT
    { return ( empty() || impl_->is_complete() ) ? 0 : & dummy::nonnull; }

    bool operator!() const BOOST_NOEXCEPT
    { return empty() || impl_->is_complete(); }

    void swap( pull_coroutine & other) BOOST_NOEXCEPT
    { impl_.swap( other.impl_); }

    pull_coroutine & operator()()
    {
        BOOST_ASSERT( * this);

        impl_->pull();
        return * this;
    }

    bool has_result() const
    {
        BOOST_ASSERT( ! empty() );

        return impl_->has_result();
    }

    R & get() const
    { return impl_->get(); }

    class iterator : public std::iterator< std::input_iterator_tag, R >
    {
    private:
        pull_coroutine< R & > *  c_;
        optional< R & >          val_;

        void fetch_()
        {
            BOOST_ASSERT( c_);

            if ( ! c_->has_result() )
            {
                c_ = 0;
                val_ = none;
                return;
            }
            val_ = c_->get();
        }

        void increment_()
        {
            BOOST_ASSERT( c_);
            BOOST_ASSERT( * c_);

            ( * c_)();
            fetch_();
        }

    public:
        typedef typename iterator::pointer      pointer_t;
        typedef typename iterator::reference    reference_t;

        iterator() :
            c_( 0), val_()
        {}

        explicit iterator( pull_coroutine< R & > * c) :
            c_( c), val_()
        { fetch_(); }

        iterator( iterator const& other) :
            c_( other.c_), val_( other.val_)
        {}

        iterator & operator=( iterator const& other)
        {
            if ( this == & other) return * this;
            c_ = other.c_;
            val_ = other.val_;
            return * this;
        }

        bool operator==( iterator const& other)
        { return other.c_ == c_ && other.val_ == val_; }

        bool operator!=( iterator const& other)
        { return other.c_ != c_ || other.val_ != val_; }

        iterator & operator++()
        {
            increment_();
            return * this;
        }

        iterator operator++( int)
        {
            iterator tmp( * this);
            ++*this;
            return tmp;
        }

        reference_t operator*() const
        {
            if ( ! val_)
                boost::throw_exception(
                    invalid_result() );
            return const_cast< optional< R & > & >( val_).get();
        }

        pointer_t operator->() const
        {
            if ( ! val_)
                boost::throw_exception(
                    invalid_result() );
            return const_cast< optional< R & > & >( val_).get_ptr();
        }
    };

    class const_iterator : public std::iterator< std::input_iterator_tag, R >
    {
    private:
        pull_coroutine< R & >   *   c_;
        optional< R & >             val_;

        void fetch_()
        {
            BOOST_ASSERT( c_);

            if ( ! c_->has_result() )
            {
                c_ = 0;
                val_ = none;
                return;
            }
            val_ = c_->get();
        }

        void increment_()
        {
            BOOST_ASSERT( c_);
            BOOST_ASSERT( * c_);

            ( * c_)();
            fetch_();
        }

    public:
        typedef typename const_iterator::pointer      pointer_t;
        typedef typename const_iterator::reference    reference_t;

        const_iterator() :
            c_( 0), val_()
        {}

        explicit const_iterator( pull_coroutine< R & > const* c) :
            c_( const_cast< pull_coroutine< R & > * >( c) ), val_()
        { fetch_(); }

        const_iterator( const_iterator const& other) :
            c_( other.c_), val_( other.val_)
        {}

        const_iterator & operator=( const_iterator const& other)
        {
            if ( this == & other) return * this;
            c_ = other.c_;
            val_ = other.val_;
            return * this;
        }

        bool operator==( const_iterator const& other)
        { return other.c_ == c_ && other.val_ == val_; }

        bool operator!=( const_iterator const& other)
        { return other.c_ != c_ || other.val_ != val_; }

        const_iterator & operator++()
        {
            increment_();
            return * this;
        }

        const_iterator operator++( int)
        {
            const_iterator tmp( * this);
            ++*this;
            return tmp;
        }

        reference_t operator*() const
        {
            if ( ! val_)
                boost::throw_exception(
                    invalid_result() );
            return val_.get();
        }

        pointer_t operator->() const
        {
            if ( ! val_)
                boost::throw_exception(
                    invalid_result() );
            return val_.get_ptr();
        }
    };
};

template<>
class pull_coroutine< void >
{
private:
    template<
        typename X, typename Y, typename Z, typename V, typename W
    >
    friend class detail::push_coroutine_object;

    typedef detail::pull_coroutine_base< void > base_t;
    typedef base_t::ptr_t                       ptr_t;

    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    ptr_t  impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( pull_coroutine)

    template< typename Allocator >
    pull_coroutine( detail::coroutine_context const& callee,
                    bool unwind, bool preserve_fpu,
                    Allocator const& alloc) :
        impl_()
    {
        typedef detail::pull_coroutine_caller<
                void, Allocator
        >                               caller_t;
        typename caller_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) caller_t(
                callee, unwind, preserve_fpu, a) );
    }

public:
    pull_coroutine() BOOST_NOEXCEPT :
        impl_()
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#ifdef BOOST_MSVC
    typedef void ( * coroutine_fn) ( push_coroutine< void > &);

    explicit pull_coroutine( coroutine_fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               disable_if<
                   is_same< typename decay< coroutine_fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                void, coroutine_fn, stack_allocator, std::allocator< pull_coroutine >,
                push_coroutine< void >
            >                               object_t;
        object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
    }

    template< typename StackAllocator >
    explicit pull_coroutine( coroutine_fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               disable_if<
                   is_same< typename decay< coroutine_fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                void, coroutine_fn, StackAllocator, std::allocator< pull_coroutine >,
                push_coroutine< void >
            >                               object_t;
        object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
    }

    template< typename StackAllocator, typename Allocator >
    explicit pull_coroutine( coroutine_fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               disable_if<
                   is_same< typename decay< coroutine_fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                void, coroutine_fn, StackAllocator, Allocator,
                push_coroutine< void >
            >                               object_t;
        object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
    }
#endif
    template< typename Fn >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                void, Fn, stack_allocator, std::allocator< pull_coroutine >,
                push_coroutine< void >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                void, Fn, StackAllocator, std::allocator< pull_coroutine >,
                push_coroutine< void >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                void, Fn, StackAllocator, Allocator,
                push_coroutine< void >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }
#else
    template< typename Fn >
    explicit pull_coroutine( Fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                void, Fn, stack_allocator, std::allocator< pull_coroutine >,
                push_coroutine< void >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                void, Fn, StackAllocator, std::allocator< pull_coroutine >,
                push_coroutine< void >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit pull_coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                void, Fn, StackAllocator, Allocator,
                push_coroutine< void >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                void, Fn, stack_allocator, std::allocator< pull_coroutine >,
                push_coroutine< void >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< pull_coroutine > const& alloc =
                    std::allocator< pull_coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                void, Fn, StackAllocator, std::allocator< pull_coroutine >,
                push_coroutine< void >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, pull_coroutine >,
                   dummy *
               >::type = 0) :
        impl_()
    {
        typedef detail::pull_coroutine_object<
                void, Fn, StackAllocator, Allocator,
                push_coroutine< void >
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }
#endif

    pull_coroutine( BOOST_RV_REF( pull_coroutine) other) BOOST_NOEXCEPT :
        impl_()
    { swap( other); }

    pull_coroutine & operator=( BOOST_RV_REF( pull_coroutine) other) BOOST_NOEXCEPT
    {
        pull_coroutine tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    bool empty() const BOOST_NOEXCEPT
    { return ! impl_; }

    operator safe_bool() const BOOST_NOEXCEPT
    { return ( empty() || impl_->is_complete() ) ? 0 : & dummy::nonnull; }

    bool operator!() const BOOST_NOEXCEPT
    { return empty() || impl_->is_complete(); }

    void swap( pull_coroutine & other) BOOST_NOEXCEPT
    { impl_.swap( other.impl_); }

    pull_coroutine & operator()()
    {
        BOOST_ASSERT( * this);

        impl_->pull();
        return * this;
    }

    struct iterator;
    struct const_iterator;
};


#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#ifdef BOOST_MSVC
template< typename Arg >
push_coroutine< Arg >::push_coroutine( coroutine_fn fn, attributes const& attr,
           stack_allocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_same< typename decay< coroutine_fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg, coroutine_fn, stack_allocator, std::allocator< push_coroutine >,
            pull_coroutine< Arg >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
}

template< typename Arg >
template< typename StackAllocator >
push_coroutine< Arg >::push_coroutine( coroutine_fn fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_same< typename decay< coroutine_fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg, coroutine_fn, StackAllocator, std::allocator< push_coroutine >,
            pull_coroutine< Arg >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
}

template< typename Arg >
template< typename StackAllocator, typename Allocator >
push_coroutine< Arg >::push_coroutine( coroutine_fn fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           Allocator const& alloc,
           typename disable_if<
               is_same< typename decay< coroutine_fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg, coroutine_fn, StackAllocator, Allocator,
            pull_coroutine< Arg >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
}

template< typename Arg >
push_coroutine< Arg & >::push_coroutine( coroutine_fn fn, attributes const& attr,
           stack_allocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_same< typename decay< coroutine_fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg &, coroutine_fn, stack_allocator, std::allocator< push_coroutine >,
            pull_coroutine< Arg & >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
}

template< typename Arg >
template< typename StackAllocator >
push_coroutine< Arg & >::push_coroutine( coroutine_fn fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_same< typename decay< coroutine_fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg &, coroutine_fn, StackAllocator, std::allocator< push_coroutine >,
            pull_coroutine< Arg & >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
}

template< typename Arg >
template< typename StackAllocator, typename Allocator >
push_coroutine< Arg & >::push_coroutine( coroutine_fn fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           Allocator const& alloc,
           typename disable_if<
               is_same< typename decay< coroutine_fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg &, coroutine_fn, StackAllocator, Allocator,
            pull_coroutine< Arg & >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
}

inline
push_coroutine< void >::push_coroutine( coroutine_fn fn, attributes const& attr,
           stack_allocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           disable_if<
               is_same< typename decay< coroutine_fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            void, coroutine_fn, stack_allocator, std::allocator< push_coroutine >,
            pull_coroutine< void >
        >                               object_t;
    object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
}

template< typename StackAllocator >
push_coroutine< void >::push_coroutine( coroutine_fn fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           disable_if<
               is_same< typename decay< coroutine_fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            void, coroutine_fn, StackAllocator, std::allocator< push_coroutine >,
            pull_coroutine< void >
        >                               object_t;
    object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
}

template< typename StackAllocator, typename Allocator >
push_coroutine< void >::push_coroutine( coroutine_fn fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           Allocator const& alloc,
           disable_if<
               is_same< typename decay< coroutine_fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            void, coroutine_fn, StackAllocator, Allocator,
            pull_coroutine< void >
        >                               object_t;
    object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
}
#endif
template< typename Arg >
template< typename Fn >
push_coroutine< Arg >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           stack_allocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg, Fn, stack_allocator, std::allocator< push_coroutine >,
            pull_coroutine< Arg >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
}

template< typename Arg >
template< typename Fn, typename StackAllocator >
push_coroutine< Arg >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg, Fn, StackAllocator, std::allocator< push_coroutine >,
            pull_coroutine< Arg >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
}

template< typename Arg >
template< typename Fn, typename StackAllocator, typename Allocator >
push_coroutine< Arg >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           Allocator const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg, Fn, StackAllocator, Allocator,
            pull_coroutine< Arg >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
}

template< typename Arg >
template< typename Fn >
push_coroutine< Arg & >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           stack_allocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg &, Fn, stack_allocator, std::allocator< push_coroutine >,
            pull_coroutine< Arg & >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
}

template< typename Arg >
template< typename Fn, typename StackAllocator >
push_coroutine< Arg & >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg &, Fn, StackAllocator, std::allocator< push_coroutine >,
            pull_coroutine< Arg & >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
}

template< typename Arg >
template< typename Fn, typename StackAllocator, typename Allocator >
push_coroutine< Arg & >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           Allocator const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg &, Fn, StackAllocator, Allocator,
            pull_coroutine< Arg & >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
}

template< typename Fn >
push_coroutine< void >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           stack_allocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            void, Fn, stack_allocator, std::allocator< push_coroutine >,
            pull_coroutine< void >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
}

template< typename Fn, typename StackAllocator >
push_coroutine< void >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            void, Fn, StackAllocator, std::allocator< push_coroutine >,
            pull_coroutine< void >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
}

template< typename Fn, typename StackAllocator, typename Allocator >
push_coroutine< void >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           Allocator const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            void, Fn, StackAllocator, Allocator,
            pull_coroutine< void >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
}
#else
template< typename Arg >
template< typename Fn >
push_coroutine< Arg >::push_coroutine( Fn fn, attributes const& attr,
           stack_allocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_convertible< Fn &, BOOST_RV_REF( Fn) >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg, Fn, stack_allocator, std::allocator< push_coroutine >,
            pull_coroutine< Arg >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Arg >
template< typename Fn, typename StackAllocator >
push_coroutine< Arg >::push_coroutine( Fn fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_convertible< Fn &, BOOST_RV_REF( Fn) >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg, Fn, StackAllocator, std::allocator< push_coroutine >,
            pull_coroutine< Arg >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Arg >
template< typename Fn, typename StackAllocator, typename Allocator >
push_coroutine< Arg >::push_coroutine( Fn fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           Allocator const& alloc,
           typename disable_if<
               is_convertible< Fn &, BOOST_RV_REF( Fn) >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg, Fn, StackAllocator, Allocator,
            pull_coroutine< Arg >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Arg >
template< typename Fn >
push_coroutine< Arg >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           stack_allocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg, Fn, stack_allocator, std::allocator< push_coroutine >,
            pull_coroutine< Arg >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Arg >
template< typename Fn, typename StackAllocator >
push_coroutine< Arg >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg, Fn, StackAllocator, std::allocator< push_coroutine >,
            pull_coroutine< Arg >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Arg >
template< typename Fn, typename StackAllocator, typename Allocator >
push_coroutine< Arg >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           Allocator const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg, Fn, StackAllocator, Allocator,
            pull_coroutine< Arg >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Arg >
template< typename Fn >
push_coroutine< Arg & >::push_coroutine( Fn fn, attributes const& attr,
           stack_allocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_convertible< Fn &, BOOST_RV_REF( Fn) >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg &, Fn, stack_allocator, std::allocator< push_coroutine >,
            pull_coroutine< Arg & >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Arg >
template< typename Fn, typename StackAllocator >
push_coroutine< Arg & >::push_coroutine( Fn fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_convertible< Fn &, BOOST_RV_REF( Fn) >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg &, Fn, StackAllocator, std::allocator< push_coroutine >,
            pull_coroutine< Arg & >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Arg >
template< typename Fn, typename StackAllocator, typename Allocator >
push_coroutine< Arg & >::push_coroutine( Fn fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           Allocator const& alloc,
           typename disable_if<
               is_convertible< Fn &, BOOST_RV_REF( Fn) >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg &, Fn, StackAllocator, Allocator,
            pull_coroutine< Arg & >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Arg >
template< typename Fn >
push_coroutine< Arg & >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           stack_allocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg &, Fn, stack_allocator, std::allocator< push_coroutine >,
            pull_coroutine< Arg & >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Arg >
template< typename Fn, typename StackAllocator >
push_coroutine< Arg & >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
           Arg &, Fn, StackAllocator, std::allocator< push_coroutine >,
            pull_coroutine< Arg & >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Arg >
template< typename Fn, typename StackAllocator, typename Allocator >
push_coroutine< Arg & >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           Allocator const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            Arg &, Fn, StackAllocator, Allocator,
            pull_coroutine< Arg & >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Fn >
push_coroutine< void >::push_coroutine( Fn fn, attributes const& attr,
           stack_allocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_convertible< Fn &, BOOST_RV_REF( Fn) >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            void, Fn, stack_allocator, std::allocator< push_coroutine >,
            pull_coroutine< void >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Fn, typename StackAllocator >
push_coroutine< void >::push_coroutine( Fn fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_convertible< Fn &, BOOST_RV_REF( Fn) >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            void, Fn, StackAllocator, std::allocator< push_coroutine >,
            pull_coroutine< void >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Fn, typename StackAllocator, typename Allocator >
push_coroutine< void >::push_coroutine( Fn fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           Allocator const& alloc,
           typename disable_if<
               is_convertible< Fn &, BOOST_RV_REF( Fn) >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            void, Fn, StackAllocator, Allocator,
            pull_coroutine< void >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Fn >
push_coroutine< void >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           stack_allocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            void, Fn, stack_allocator, std::allocator< push_coroutine >,
            pull_coroutine< void >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Fn, typename StackAllocator >
push_coroutine< void >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           std::allocator< push_coroutine > const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            void, Fn, StackAllocator, std::allocator< push_coroutine >,
            pull_coroutine< void >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}

template< typename Fn, typename StackAllocator, typename Allocator >
push_coroutine< void >::push_coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
           StackAllocator const& stack_alloc,
           Allocator const& alloc,
           typename disable_if<
               is_same< typename decay< Fn >::type, push_coroutine >,
               dummy *
           >::type) :
    impl_()
{
    typedef detail::push_coroutine_object<
            void, Fn, StackAllocator, Allocator,
            pull_coroutine< void >
        >                               object_t;
    typename object_t::allocator_t a( alloc);
    impl_ = ptr_t(
        // placement new
        ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
}
#endif

template< typename R >
void swap( pull_coroutine< R > & l, pull_coroutine< R > & r) BOOST_NOEXCEPT
{ l.swap( r); }

template< typename Arg >
void swap( push_coroutine< Arg > & l, push_coroutine< Arg > & r) BOOST_NOEXCEPT
{ l.swap( r); }

template< typename R >
inline
typename pull_coroutine< R >::iterator
range_begin( pull_coroutine< R > & c)
{ return typename pull_coroutine< R >::iterator( & c); }

template< typename R >
inline
typename pull_coroutine< R >::const_iterator
range_begin( pull_coroutine< R > const& c)
{ return typename pull_coroutine< R >::const_iterator( & c); }

template< typename R >
inline
typename pull_coroutine< R >::iterator
range_end( pull_coroutine< R > &)
{ return typename pull_coroutine< R >::iterator(); }

template< typename R >
inline
typename pull_coroutine< R >::const_iterator
range_end( pull_coroutine< R > const&)
{ return typename pull_coroutine< R >::const_iterator(); }

template< typename Arg >
inline
typename push_coroutine< Arg >::iterator
range_begin( push_coroutine< Arg > & c)
{ return typename push_coroutine< Arg >::iterator( & c); }

template< typename Arg >
inline
typename push_coroutine< Arg >::const_iterator
range_begin( push_coroutine< Arg > const& c)
{ return typename push_coroutine< Arg >::const_iterator( & c); }

template< typename Arg >
inline
typename push_coroutine< Arg >::iterator
range_end( push_coroutine< Arg > &)
{ return typename push_coroutine< Arg >::iterator(); }

template< typename Arg >
inline
typename push_coroutine< Arg >::const_iterator
range_end( push_coroutine< Arg > const&)
{ return typename push_coroutine< Arg >::const_iterator(); }

template< typename T >
struct coroutine
{
    typedef push_coroutine< T > push_type;
    typedef pull_coroutine< T > pull_type;
};

}

template< typename Arg >
struct range_mutable_iterator< coroutines::push_coroutine< Arg > >
{ typedef typename coroutines::push_coroutine< Arg >::iterator type; };

template< typename Arg >
struct range_const_iterator< coroutines::push_coroutine< Arg > >
{ typedef typename coroutines::push_coroutine< Arg >::const_iterator type; };

template< typename R >
struct range_mutable_iterator< coroutines::pull_coroutine< R > >
{ typedef typename coroutines::pull_coroutine< R >::iterator type; };

template< typename R >
struct range_const_iterator< coroutines::pull_coroutine< R > >
{ typedef typename coroutines::pull_coroutine< R >::const_iterator type; };

}

namespace std {

template< typename R >
inline
typename boost::coroutines::pull_coroutine< R >::iterator
begin( boost::coroutines::pull_coroutine< R > & c)
{ return boost::begin( c); }

template< typename R >
inline
typename boost::coroutines::pull_coroutine< R >::iterator
end( boost::coroutines::pull_coroutine< R > & c)
{ return boost::end( c); }

template< typename R >
inline
typename boost::coroutines::pull_coroutine< R >::const_iterator
begin( boost::coroutines::pull_coroutine< R > const& c)
{ return boost::const_begin( c); }

template< typename R >
inline
typename boost::coroutines::pull_coroutine< R >::const_iterator
end( boost::coroutines::pull_coroutine< R > const& c)
{ return boost::const_end( c); }

template< typename R >
inline
typename boost::coroutines::push_coroutine< R >::iterator
begin( boost::coroutines::push_coroutine< R > & c)
{ return boost::begin( c); }

template< typename R >
inline
typename boost::coroutines::push_coroutine< R >::iterator
end( boost::coroutines::push_coroutine< R > & c)
{ return boost::end( c); }

template< typename R >
inline
typename boost::coroutines::push_coroutine< R >::const_iterator
begin( boost::coroutines::push_coroutine< R > const& c)
{ return boost::const_begin( c); }

template< typename R >
inline
typename boost::coroutines::push_coroutine< R >::const_iterator
end( boost::coroutines::push_coroutine< R > const& c)
{ return boost::const_end( c); }

}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_UNIDIRECT_COROUTINE_H


//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_OLD_COROUTINE_H
#define BOOST_COROUTINES_OLD_COROUTINE_H

#include <cstddef>
#include <memory>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/range.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/type_traits/function_traits.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility/result_of.hpp>

#include <boost/coroutine/attributes.hpp>
#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/detail/coroutine_context.hpp>
#include <boost/coroutine/stack_allocator.hpp>
#include <boost/coroutine/v1/detail/arg.hpp>
#include <boost/coroutine/v1/detail/coroutine_base.hpp>
#include <boost/coroutine/v1/detail/coroutine_caller.hpp>
#include <boost/coroutine/v1/detail/coroutine_get.hpp>
#include <boost/coroutine/v1/detail/coroutine_object.hpp>
#include <boost/coroutine/v1/detail/coroutine_op.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {
namespace detail {

template<
    typename Signature,
    template< class, int > class C,
    typename Result = typename function_traits< Signature >::result_type,
    int arity = function_traits< Signature >::arity
>
struct caller;

template<
    typename Signature,
    template< class, int > class C
>
struct caller< Signature, C, void, 0 >
{ typedef C< void(), 0 > type; };

template<
    typename Signature,
    template< class, int > class C,
    typename Result
>
struct caller< Signature, C, Result, 0 >
{ typedef C< void( Result), 1 > type; };

template<
    typename Signature,
    template< class, int > class C,
    int arity
>
struct caller< Signature, C, void, arity >
{ typedef C< typename detail::arg< Signature >::type(), 0 > type; };

template<
    typename Signature,
    template< class, int > class C,
    typename Result, int arity
>
struct caller
{ typedef C< typename detail::arg< Signature >::type( Result), 1 > type; };

}

template< typename Signature, int arity = function_traits< Signature >::arity >
class coroutine;

template< typename Signature >
class coroutine< Signature, 0 > : public detail::coroutine_op<
                                        Signature,
                                        coroutine< Signature >,
                                        typename function_traits< Signature >::result_type,
                                        function_traits< Signature >::arity
                                  >,
                                  public detail::coroutine_get<
                                        coroutine< Signature >,
                                        typename function_traits< Signature >::result_type,
                                        function_traits< Signature >::arity
                                  >
{
private:
    typedef detail::coroutine_base< Signature >                 base_t;
    typedef typename base_t::ptr_t                              ptr_t;

    template< typename X, typename Y, int >
    friend struct detail::coroutine_get;
    template< typename X, typename Y, typename Z, int >
    friend struct detail::coroutine_op;
    template< typename X, typename Y, typename Z, typename A, typename B, typename C, int >
    friend class detail::coroutine_object;

    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    ptr_t  impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( coroutine)

    template< typename Allocator >
    coroutine( detail::coroutine_context const& callee,
               bool unwind, bool preserve_fpu,
               Allocator const& alloc) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity

        >(),
        impl_()
    {
        typedef detail::coroutine_caller<
                Signature, Allocator
        >                               caller_t;
        typename caller_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) caller_t(
                callee, unwind, preserve_fpu, a) );
    }

public:
    typedef typename detail::caller<
            Signature,
            boost::coroutines::coroutine
    >::type                                                     caller_type;

    coroutine() BOOST_NOEXCEPT :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#ifdef BOOST_MSVC
    typedef void ( * coroutine_fn) ( caller_type &);

    explicit coroutine( coroutine_fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >() ) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
        typedef detail::coroutine_object<
                Signature,
                coroutine_fn, stack_allocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
    }

    template< typename StackAllocator >
    explicit coroutine( coroutine_fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >() ) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
       typedef detail::coroutine_object<
                Signature,
                coroutine_fn, StackAllocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
    }

    template< typename StackAllocator, typename Allocator >
    explicit coroutine( coroutine_fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
        typedef detail::coroutine_object<
                Signature,
                coroutine_fn, StackAllocator, Allocator,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
    }
#endif
    template< typename Fn >
    explicit coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, coroutine >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, stack_allocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, coroutine >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, StackAllocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, coroutine >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, StackAllocator, Allocator,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }
#else
    template< typename Fn >
    explicit coroutine( Fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, stack_allocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, StackAllocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, StackAllocator, Allocator,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn >
    explicit coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, coroutine >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, stack_allocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, coroutine >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, StackAllocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, coroutine >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, StackAllocator, Allocator,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }
#endif

    coroutine( BOOST_RV_REF( coroutine) other) BOOST_NOEXCEPT :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    { swap( other); }

    coroutine & operator=( BOOST_RV_REF( coroutine) other) BOOST_NOEXCEPT
    {
        coroutine tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    bool empty() const BOOST_NOEXCEPT
    { return ! impl_; }

    operator safe_bool() const BOOST_NOEXCEPT
    { return ( empty() || impl_->is_complete() ) ? 0 : & dummy::nonnull; }

    bool operator!() const BOOST_NOEXCEPT
    { return empty() || impl_->is_complete(); }

    void swap( coroutine & other) BOOST_NOEXCEPT
    { impl_.swap( other.impl_); }
};

template< typename Signature, int arity >
class coroutine : public detail::coroutine_op<
                        Signature,
                        coroutine< Signature >,
                        typename function_traits< Signature >::result_type,
                        function_traits< Signature >::arity
                  >,
                  public detail::coroutine_get<
                        coroutine< Signature >,
                        typename function_traits< Signature >::result_type,
                        function_traits< Signature >::arity
                  >
{
private:
    typedef detail::coroutine_base< Signature >                 base_t;
    typedef typename base_t::ptr_t                              ptr_t;

    template< typename X, typename Y, int >
    friend struct detail::coroutine_get;
    template< typename X, typename Y, typename Z, int >
    friend struct detail::coroutine_op;
    template< typename X, typename Y, typename Z, typename A, typename B, typename C, int >
    friend class detail::coroutine_object;

    struct dummy
    { void nonnull() {} };

    typedef void ( dummy::*safe_bool)();

    ptr_t  impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( coroutine)

    template< typename Allocator >
    coroutine( detail::coroutine_context const& callee,
               bool unwind, bool preserve_fpu,
               Allocator const& alloc) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
        typedef detail::coroutine_caller<
                Signature, Allocator
            >                               caller_t;
        typename caller_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) caller_t(
                callee, unwind, preserve_fpu, a) );
    }

public:
    typedef typename detail::caller<
        Signature,
        boost::coroutines::coroutine
    >::type                                                     caller_type;
    typedef typename detail::arg< Signature >::type             arguments;

    coroutine() BOOST_NOEXCEPT :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#ifdef BOOST_MSVC
    typedef void ( * coroutine_fn) ( caller_type &);

    explicit coroutine( coroutine_fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >() ) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
        typedef detail::coroutine_object<
                Signature,
                coroutine_fn, stack_allocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
    }

    explicit coroutine( coroutine_fn fn, arguments arg, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >() ) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
       typedef detail::coroutine_object<
                Signature,
                coroutine_fn, stack_allocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), arg, attr, stack_alloc, a) );
    }

    template< typename StackAllocator >
    explicit coroutine( coroutine_fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >() ) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
        typedef detail::coroutine_object<
                Signature,
                coroutine_fn, StackAllocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
    }

    template< typename StackAllocator, typename Allocator >
    explicit coroutine( coroutine_fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
        typedef detail::coroutine_object<
                Signature,
                coroutine_fn, StackAllocator, Allocator,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
        >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< coroutine_fn >( fn), attr, stack_alloc, a) );
    }
#endif
    template< typename Fn >
    explicit coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, coroutine >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, stack_allocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }

    template< typename Fn >
    explicit coroutine( BOOST_RV_REF( Fn) fn, arguments arg, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, coroutine >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, stack_allocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), arg, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, coroutine >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, StackAllocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit coroutine( BOOST_RV_REF( Fn) fn, arguments arg, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, coroutine >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, StackAllocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), arg, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, coroutine >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, StackAllocator, Allocator,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit coroutine( BOOST_RV_REF( Fn) fn, arguments arg, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, coroutine >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, StackAllocator, Allocator,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( forward< Fn >( fn), arg, attr, stack_alloc, a) );
    }
#else
    template< typename Fn >
    explicit coroutine( Fn fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, stack_allocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn >
    explicit coroutine( Fn fn, arguments arg, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, stack_allocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, arg, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >(),
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, StackAllocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit coroutine( Fn fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_convertible< Fn &, BOOST_RV_REF( Fn) >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, StackAllocator, Allocator,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn >
    explicit coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr = attributes(),
               stack_allocator const& stack_alloc =
                    stack_allocator(),
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, coroutine >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, stack_allocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator >
    explicit coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               std::allocator< coroutine > const& alloc =
                    std::allocator< coroutine >(),
               typename disable_if<
                   is_same< typename decay< Fn >::type, coroutine >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, StackAllocator, std::allocator< coroutine >,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }

    template< typename Fn, typename StackAllocator, typename Allocator >
    explicit coroutine( BOOST_RV_REF( Fn) fn, attributes const& attr,
               StackAllocator const& stack_alloc,
               Allocator const& alloc,
               typename disable_if<
                   is_same< typename decay< Fn >::type, coroutine >,
                   dummy *
               >::type = 0) :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    {
//      BOOST_STATIC_ASSERT((
//          is_same< void, typename result_of< Fn() >::type >::value));
        typedef detail::coroutine_object<
                Signature,
                Fn, StackAllocator, Allocator,
                caller_type,
                typename function_traits< Signature >::result_type,
                function_traits< Signature >::arity
            >                               object_t;
        typename object_t::allocator_t a( alloc);
        impl_ = ptr_t(
            // placement new
            ::new( a.allocate( 1) ) object_t( fn, attr, stack_alloc, a) );
    }
#endif

    coroutine( BOOST_RV_REF( coroutine) other) BOOST_NOEXCEPT :
        detail::coroutine_op<
            Signature,
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        detail::coroutine_get<
            coroutine< Signature >,
            typename function_traits< Signature >::result_type,
            function_traits< Signature >::arity
        >(),
        impl_()
    { swap( other); }

    coroutine & operator=( BOOST_RV_REF( coroutine) other) BOOST_NOEXCEPT
    {
        coroutine tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    bool empty() const BOOST_NOEXCEPT
    { return ! impl_; }

    operator safe_bool() const BOOST_NOEXCEPT
    { return ( empty() || impl_->is_complete() ) ? 0 : & dummy::nonnull; }

    bool operator!() const BOOST_NOEXCEPT
    { return empty() || impl_->is_complete(); }

    void swap( coroutine & other) BOOST_NOEXCEPT
    { impl_.swap( other.impl_); }
};

template< typename Signature >
void swap( coroutine< Signature > & l, coroutine< Signature > & r) BOOST_NOEXCEPT
{ l.swap( r); }

template< typename Signature >
inline
typename coroutine< Signature >::iterator
range_begin( coroutine< Signature > & c)
{ return typename coroutine< Signature >::iterator( & c); }

template< typename Signature >
inline
typename coroutine< Signature >::const_iterator
range_begin( coroutine< Signature > const& c)
{ return typename coroutine< Signature >::const_iterator( & c); }

template< typename Signature >
inline
typename coroutine< Signature >::iterator
range_end( coroutine< Signature > &)
{ return typename coroutine< Signature >::iterator(); }

template< typename Signature >
inline
typename coroutine< Signature >::const_iterator
range_end( coroutine< Signature > const&)
{ return typename coroutine< Signature >::const_iterator(); }

template< typename Signature >
inline
typename coroutine< Signature >::iterator
begin( coroutine< Signature > & c)
{ return boost::begin( c); }

template< typename Signature >
inline
typename coroutine< Signature >::iterator
end( coroutine< Signature > & c)
{ return boost::end( c); }

template< typename Signature >
inline
typename coroutine< Signature >::const_iterator
begin( coroutine< Signature > const& c)
{ return boost::const_begin( c); }

template< typename Signature >
inline
typename coroutine< Signature >::const_iterator
end( coroutine< Signature > const& c)
{ return boost::const_end( c); }

}

template< typename Signature >
struct range_mutable_iterator< coroutines::coroutine< Signature > >
{ typedef typename coroutines::coroutine< Signature >::iterator type; };

template< typename Signature >
struct range_const_iterator< coroutines::coroutine< Signature > >
{ typedef typename coroutines::coroutine< Signature >::const_iterator type; };

}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_OLD_COROUTINE_H

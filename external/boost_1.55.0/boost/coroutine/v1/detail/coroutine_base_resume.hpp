
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_OLD_DETAIL_COROUTINE_BASE_RESUME_H
#define BOOST_COROUTINES_OLD_DETAIL_COROUTINE_BASE_RESUME_H

#include <iterator>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/fcontext.hpp>
#include <boost/optional.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/range.hpp>

#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/detail/coroutine_context.hpp>
#include <boost/coroutine/exceptions.hpp>
#include <boost/coroutine/detail/holder.hpp>
#include <boost/coroutine/v1/detail/arg.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {
namespace detail {

template< typename Signature, typename D, typename Result, int arity >
class coroutine_base_resume;

template< typename Signature, typename D >
class coroutine_base_resume< Signature, D, void, 0 >
{
public:
    void resume()
    {
        holder< void > hldr_to( & static_cast< D * >( this)->caller_);
        holder< void > * hldr_from(
            reinterpret_cast< holder< void > * >(
                hldr_to.ctx->jump(
                    static_cast< D * >( this)->callee_,
                    reinterpret_cast< intptr_t >( & hldr_to),
                    static_cast< D * >( this)->preserve_fpu() ) ) );
        BOOST_ASSERT( hldr_from->ctx);
        static_cast< D * >( this)->callee_ = * hldr_from->ctx;
        if ( hldr_from->force_unwind) throw forced_unwind();
        if ( static_cast< D * >( this)->except_)
            rethrow_exception( static_cast< D * >( this)->except_);
    }
};

template< typename Signature, typename D, typename Result >
class coroutine_base_resume< Signature, D, Result, 0 >
{
public:
    void resume()
    {
        BOOST_ASSERT( static_cast< D * >( this));
        BOOST_ASSERT( ! static_cast< D * >( this)->is_complete() );

        holder< void > hldr_to( & static_cast< D * >( this)->caller_);
        holder< Result > * hldr_from(
            reinterpret_cast< holder< Result > * >(
                hldr_to.ctx->jump(
                    static_cast< D * >( this)->callee_,
                    reinterpret_cast< intptr_t >( & hldr_to),
                    static_cast< D * >( this)->preserve_fpu() ) ) );
        BOOST_ASSERT( hldr_from->ctx);
        static_cast< D * >( this)->callee_ = * hldr_from->ctx;
        result_ = hldr_from->data;
        if ( hldr_from->force_unwind) throw forced_unwind();
        if ( static_cast< D * >( this)->except_)
            rethrow_exception( static_cast< D * >( this)->except_);
    }

protected:
    template< typename X, typename Y, int >
    friend struct coroutine_get;

    optional< Result >  result_;
};

template< typename Signature, typename D >
class coroutine_base_resume< Signature, D, void, 1 >
{
public:
    typedef typename arg< Signature >::type     arg_type;

    void resume( arg_type a1)
    {
        BOOST_ASSERT( static_cast< D * >( this));
        BOOST_ASSERT( ! static_cast< D * >( this)->is_complete() );

        holder< arg_type > hldr_to( & static_cast< D * >( this)->caller_, a1);
        holder< void > * hldr_from(
            reinterpret_cast< holder< void > * >(
                hldr_to.ctx->jump(
                    static_cast< D * >( this)->callee_,
                    reinterpret_cast< intptr_t >( & hldr_to),
                    static_cast< D * >( this)->preserve_fpu() ) ) );
        BOOST_ASSERT( hldr_from->ctx);
        static_cast< D * >( this)->callee_ = * hldr_from->ctx;
        if ( hldr_from->force_unwind) throw forced_unwind();
        if ( static_cast< D * >( this)->except_)
            rethrow_exception( static_cast< D * >( this)->except_);
    }
};

template< typename Signature, typename D, typename Result >
class coroutine_base_resume< Signature, D, Result, 1 >
{
public:
    typedef typename arg< Signature >::type     arg_type;

    void resume( arg_type a1)
    {
        BOOST_ASSERT( static_cast< D * >( this));
        BOOST_ASSERT( ! static_cast< D * >( this)->is_complete() );

        coroutine_context caller;
        holder< arg_type > hldr_to( & static_cast< D * >( this)->caller_, a1);
        holder< Result > * hldr_from(
            reinterpret_cast< holder< Result > * >(
                hldr_to.ctx->jump(
                    static_cast< D * >( this)->callee_,
                    reinterpret_cast< intptr_t >( & hldr_to),
                    static_cast< D * >( this)->preserve_fpu() ) ) );
        BOOST_ASSERT( hldr_from->ctx);
        static_cast< D * >( this)->callee_ = * hldr_from->ctx;
        result_ = hldr_from->data;
        if ( hldr_from->force_unwind) throw forced_unwind();
        if ( static_cast< D * >( this)->except_)
            rethrow_exception( static_cast< D * >( this)->except_);
    }

protected:
    template< typename X, typename Y, int >
    friend struct coroutine_get;

    optional< Result >  result_;
};

#define BOOST_COROUTINE_BASE_RESUME_COMMA(n) BOOST_PP_COMMA_IF(BOOST_PP_SUB(n,1))
#define BOOST_COROUTINE_BASE_RESUME_VAL(z,n,unused) BOOST_COROUTINE_BASE_RESUME_COMMA(n) BOOST_PP_CAT(a,n)
#define BOOST_COROUTINE_BASE_RESUME_VALS(n) BOOST_PP_REPEAT_FROM_TO(1,BOOST_PP_ADD(n,1),BOOST_COROUTINE_BASE_RESUME_VAL,~)
#define BOOST_COROUTINE_BASE_RESUME_ARG_TYPE(n) \
    typename function_traits< Signature >::BOOST_PP_CAT(BOOST_PP_CAT(arg,n),_type)
#define BOOST_COROUTINE_BASE_RESUME_ARG(z,n,unused) BOOST_COROUTINE_BASE_RESUME_COMMA(n) BOOST_COROUTINE_BASE_RESUME_ARG_TYPE(n) BOOST_PP_CAT(a,n)
#define BOOST_COROUTINE_BASE_RESUME_ARGS(n) BOOST_PP_REPEAT_FROM_TO(1,BOOST_PP_ADD(n,1),BOOST_COROUTINE_BASE_RESUME_ARG,~)
#define BOOST_COROUTINE_BASE_RESUME(z,n,unused) \
template< typename Signature, typename D > \
class coroutine_base_resume< Signature, D, void, n > \
{ \
public: \
    typedef typename arg< Signature >::type     arg_type; \
\
    void resume( BOOST_COROUTINE_BASE_RESUME_ARGS(n)) \
    { \
        BOOST_ASSERT( static_cast< D * >( this)); \
        BOOST_ASSERT( ! static_cast< D * >( this)->is_complete() ); \
\
        holder< arg_type > hldr_to( \
            & static_cast< D * >( this)->caller_, \
            arg_type(BOOST_COROUTINE_BASE_RESUME_VALS(n) ) ); \
        holder< void > * hldr_from( \
            reinterpret_cast< holder< void > * >( \
                hldr_to.ctx->jump( \
                    static_cast< D * >( this)->callee_, \
                    reinterpret_cast< intptr_t >( & hldr_to), \
                    static_cast< D * >( this)->preserve_fpu() ) ) ); \
        BOOST_ASSERT( hldr_from->ctx); \
        static_cast< D * >( this)->callee_ = * hldr_from->ctx; \
        if ( hldr_from->force_unwind) throw forced_unwind(); \
        if ( static_cast< D * >( this)->except_) \
            rethrow_exception( static_cast< D * >( this)->except_); \
    } \
}; \
\
template< typename Signature, typename D, typename Result > \
class coroutine_base_resume< Signature, D, Result, n > \
{ \
public: \
    typedef typename arg< Signature >::type     arg_type; \
\
    void resume( BOOST_COROUTINE_BASE_RESUME_ARGS(n)) \
    { \
        BOOST_ASSERT( static_cast< D * >( this)); \
        BOOST_ASSERT( ! static_cast< D * >( this)->is_complete() ); \
\
        holder< arg_type > hldr_to( \
            & static_cast< D * >( this)->caller_, \
            arg_type(BOOST_COROUTINE_BASE_RESUME_VALS(n) ) ); \
        holder< Result > * hldr_from( \
            reinterpret_cast< holder< Result > * >( \
                hldr_to.ctx->jump( \
                    static_cast< D * >( this)->callee_, \
                    reinterpret_cast< intptr_t >( & hldr_to), \
                    static_cast< D * >( this)->preserve_fpu() ) ) ); \
        BOOST_ASSERT( hldr_from->ctx); \
        static_cast< D * >( this)->callee_ = * hldr_from->ctx; \
        result_ = hldr_from->data; \
        if ( hldr_from->force_unwind) throw forced_unwind(); \
        if ( static_cast< D * >( this)->except_) \
            rethrow_exception( static_cast< D * >( this)->except_); \
    } \
\
protected: \
    template< typename X, typename Y, int > \
    friend struct coroutine_get; \
\
    optional< Result >  result_; \
};
BOOST_PP_REPEAT_FROM_TO(2,11,BOOST_COROUTINE_BASE_RESUME,~)
#undef BOOST_COROUTINE_BASE_RESUME
#undef BOOST_COROUTINE_BASE_RESUME_ARGS
#undef BOOST_COROUTINE_BASE_RESUME_ARG
#undef BOOST_COROUTINE_BASE_RESUME_ARG_TYPE
#undef BOOST_COROUTINE_BASE_RESUME_VALS
#undef BOOST_COROUTINE_BASE_RESUME_VAL
#undef BOOST_COROUTINE_BASE_RESUME_COMMA

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_OLD_DETAIL_coroutine_base_resume_H


//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_UNIDIRECT_DETAIL_PUSH_COROUTINE_BASE_H
#define BOOST_COROUTINES_UNIDIRECT_DETAIL_PUSH_COROUTINE_BASE_H

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/fcontext.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/type_traits/function_traits.hpp>
#include <boost/utility.hpp>

#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/detail/coroutine_context.hpp>
#include <boost/coroutine/exceptions.hpp>
#include <boost/coroutine/detail/flags.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {

struct stack_context;

namespace detail {

template< typename Arg >
class push_coroutine_base : private noncopyable
{
public:
    typedef intrusive_ptr< push_coroutine_base >     ptr_t;

private:
    template<
        typename X, typename Y, typename Z, typename V, typename W
    >
    friend class pull_coroutine_object;

    unsigned int        use_count_;

protected:
    int                 flags_;
    exception_ptr       except_;
    coroutine_context   caller_;
    coroutine_context   callee_;

    virtual void deallocate_object() = 0;

public:
    push_coroutine_base( coroutine_context::ctx_fn fn,
                         stack_context * stack_ctx,
                         bool unwind, bool preserve_fpu) :
        use_count_( 0),
        flags_( 0),
        except_(),
        caller_(),
        callee_( fn, stack_ctx)
    {
        if ( unwind) flags_ |= flag_force_unwind;
        if ( preserve_fpu) flags_ |= flag_preserve_fpu;
    }

    push_coroutine_base( coroutine_context const& callee,
                         bool unwind, bool preserve_fpu) :
        use_count_( 0),
        flags_( 0),
        except_(),
        caller_(),
        callee_( callee)
    {
        if ( unwind) flags_ |= flag_force_unwind;
        if ( preserve_fpu) flags_ |= flag_preserve_fpu;
    }

    virtual ~push_coroutine_base()
    {}

    bool force_unwind() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_force_unwind); }

    bool unwind_requested() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_unwind_stack); }

    bool preserve_fpu() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_preserve_fpu); }

    bool is_complete() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_complete); }

    friend inline void intrusive_ptr_add_ref( push_coroutine_base * p) BOOST_NOEXCEPT
    { ++p->use_count_; }

    friend inline void intrusive_ptr_release( push_coroutine_base * p) BOOST_NOEXCEPT
    { if ( --p->use_count_ == 0) p->deallocate_object(); }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    void push( Arg const& arg)
    {
        BOOST_ASSERT( ! is_complete() );

        holder< Arg > hldr_to( & caller_, arg);
        holder< Arg > * hldr_from(
            reinterpret_cast< holder< Arg > * >(
                hldr_to.ctx->jump(
                    callee_,
                    reinterpret_cast< intptr_t >( & hldr_to),
                    preserve_fpu() ) ) );
        BOOST_ASSERT( hldr_from->ctx);
        callee_ = * hldr_from->ctx;
        if ( hldr_from->force_unwind) throw forced_unwind();
        if ( except_) rethrow_exception( except_);
    }

    void push( Arg && arg)
    {
        BOOST_ASSERT( ! is_complete() );

        holder< Arg > hldr_to( & caller_, boost::forward( arg) );
        holder< Arg > * hldr_from(
            reinterpret_cast< holder< Arg > * >(
                hldr_to.ctx->jump(
                    callee_,
                    reinterpret_cast< intptr_t >( & hldr_to),
                    preserve_fpu() ) ) );
        BOOST_ASSERT( hldr_from->ctx);
        callee_ = * hldr_from->ctx;
        if ( hldr_from->force_unwind) throw forced_unwind();
        if ( except_) rethrow_exception( except_);
    }
#else
    void push( Arg const& arg)
    {
        BOOST_ASSERT( ! is_complete() );

        holder< Arg > hldr_to( & caller_, arg);
        holder< Arg > * hldr_from(
            reinterpret_cast< holder< Arg > * >(
                hldr_to.ctx->jump(
                    callee_,
                    reinterpret_cast< intptr_t >( & hldr_to),
                    preserve_fpu() ) ) );
        BOOST_ASSERT( hldr_from->ctx);
        callee_ = * hldr_from->ctx;
        if ( hldr_from->force_unwind) throw forced_unwind();
        if ( except_) rethrow_exception( except_);
    }

    void push( BOOST_RV_REF( Arg) arg)
    {
        BOOST_ASSERT( ! is_complete() );

        holder< Arg > hldr_to( & caller_, arg);
        holder< Arg > * hldr_from(
            reinterpret_cast< holder< Arg > * >(
                hldr_to.ctx->jump(
                    callee_,
                    reinterpret_cast< intptr_t >( & hldr_to),
                    preserve_fpu() ) ) );
        BOOST_ASSERT( hldr_from->ctx);
        callee_ = * hldr_from->ctx;
        if ( hldr_from->force_unwind) throw forced_unwind();
        if ( except_) rethrow_exception( except_);
    }
#endif
};

template< typename Arg >
class push_coroutine_base< Arg & > : private noncopyable
{
public:
    typedef intrusive_ptr< push_coroutine_base >     ptr_t;

private:
    template<
        typename X, typename Y, typename Z, typename V, typename W
    >
    friend class pull_coroutine_object;

    unsigned int        use_count_;

protected:
    int                 flags_;
    exception_ptr       except_;
    coroutine_context   caller_;
    coroutine_context   callee_;

    virtual void deallocate_object() = 0;

public:
    push_coroutine_base( coroutine_context::ctx_fn fn,
                         stack_context * stack_ctx,
                         bool unwind, bool preserve_fpu) :
        use_count_( 0),
        flags_( 0),
        except_(),
        caller_(),
        callee_( fn, stack_ctx)
    {
        if ( unwind) flags_ |= flag_force_unwind;
        if ( preserve_fpu) flags_ |= flag_preserve_fpu;
    }

    push_coroutine_base( coroutine_context const& callee,
                         bool unwind, bool preserve_fpu) :
        use_count_( 0),
        flags_( 0),
        except_(),
        caller_(),
        callee_( callee)
    {
        if ( unwind) flags_ |= flag_force_unwind;
        if ( preserve_fpu) flags_ |= flag_preserve_fpu;
    }

    virtual ~push_coroutine_base()
    {}

    bool force_unwind() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_force_unwind); }

    bool unwind_requested() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_unwind_stack); }

    bool preserve_fpu() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_preserve_fpu); }

    bool is_complete() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_complete); }

    friend inline void intrusive_ptr_add_ref( push_coroutine_base * p) BOOST_NOEXCEPT
    { ++p->use_count_; }

    friend inline void intrusive_ptr_release( push_coroutine_base * p) BOOST_NOEXCEPT
    { if ( --p->use_count_ == 0) p->deallocate_object(); }

    void push( Arg & arg)
    {
        BOOST_ASSERT( ! is_complete() );

        holder< Arg * > hldr_to( & caller_, & arg);
        holder< Arg * > * hldr_from(
            reinterpret_cast< holder< Arg * > * >(
                hldr_to.ctx->jump(
                    callee_,
                    reinterpret_cast< intptr_t >( & hldr_to),
                    preserve_fpu() ) ) );
        BOOST_ASSERT( hldr_from->ctx);
        callee_ = * hldr_from->ctx;
        if ( hldr_from->force_unwind) throw forced_unwind();
        if ( except_) rethrow_exception( except_);
    }
};

template<>
class push_coroutine_base< void > : private noncopyable
{
public:
    typedef intrusive_ptr< push_coroutine_base >     ptr_t;

private:
    template<
        typename X, typename Y, typename Z, typename V, typename W
    >
    friend class pull_coroutine_object;

    unsigned int        use_count_;

protected:
    int                 flags_;
    exception_ptr       except_;
    coroutine_context   caller_;
    coroutine_context   callee_;

    virtual void deallocate_object() = 0;

public:
    push_coroutine_base( coroutine_context::ctx_fn fn,
                         stack_context * stack_ctx,
                         bool unwind, bool preserve_fpu) :
        use_count_( 0),
        flags_( 0),
        except_(),
        caller_(),
        callee_( fn, stack_ctx)
    {
        if ( unwind) flags_ |= flag_force_unwind;
        if ( preserve_fpu) flags_ |= flag_preserve_fpu;
    }

    push_coroutine_base( coroutine_context const& callee,
                         bool unwind, bool preserve_fpu) :
        use_count_( 0),
        flags_( 0),
        except_(),
        caller_(),
        callee_( callee)
    {
        if ( unwind) flags_ |= flag_force_unwind;
        if ( preserve_fpu) flags_ |= flag_preserve_fpu;
    }

    virtual ~push_coroutine_base()
    {}

    bool force_unwind() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_force_unwind); }

    bool unwind_requested() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_unwind_stack); }

    bool preserve_fpu() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_preserve_fpu); }

    bool is_complete() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_complete); }

    friend inline void intrusive_ptr_add_ref( push_coroutine_base * p) BOOST_NOEXCEPT
    { ++p->use_count_; }

    friend inline void intrusive_ptr_release( push_coroutine_base * p) BOOST_NOEXCEPT
    { if ( --p->use_count_ == 0) p->deallocate_object(); }

    void push()
    {
        BOOST_ASSERT( ! is_complete() );

        holder< void > hldr_to( & caller_);
        holder< void > * hldr_from(
            reinterpret_cast< holder< void > * >(
                hldr_to.ctx->jump(
                    callee_,
                    reinterpret_cast< intptr_t >( & hldr_to),
                    preserve_fpu() ) ) );
        BOOST_ASSERT( hldr_from->ctx);
        callee_ = * hldr_from->ctx;
        if ( hldr_from->force_unwind) throw forced_unwind();
        if ( except_) rethrow_exception( except_);
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_UNIDIRECT_DETAIL_PUSH_COROUTINE_BASE_H

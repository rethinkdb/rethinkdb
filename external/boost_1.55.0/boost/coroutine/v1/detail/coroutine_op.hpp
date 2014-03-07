
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_OLD_DETAIL_COROUTINE_OP_H
#define BOOST_COROUTINES_OLD_DETAIL_COROUTINE_OP_H

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
#include <boost/type_traits/function_traits.hpp>
#include <boost/type_traits/remove_reference.hpp>

#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/v1/detail/arg.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {
namespace detail {

template< typename Signature, typename D, typename Result, int arity >
struct coroutine_op;

template< typename Signature, typename D >
struct coroutine_op< Signature, D, void, 0 >
{
    D & operator()()
    {
        BOOST_ASSERT( static_cast< D * >( this)->impl_);
        BOOST_ASSERT( ! static_cast< D * >( this)->impl_->is_complete() );

        static_cast< D * >( this)->impl_->resume();

        return * static_cast< D * >( this);
    }
};

template< typename Signature, typename D, typename Result >
struct coroutine_op< Signature, D, Result, 0 >
{
    class iterator : public std::iterator< std::input_iterator_tag, typename remove_reference< Result >::type >
    {
    private:
        D               *   dp_;
        optional< Result >  val_;

        void fetch_()
        {
            BOOST_ASSERT( dp_);

            if ( ! dp_->has_result() )
            {
                dp_ = 0;
                val_ = none;
                return;
            }
            val_ = dp_->get();
        }

        void increment_()
        {
            BOOST_ASSERT( dp_);
            BOOST_ASSERT( * dp_);

            ( * dp_)();
            fetch_();
        }

    public:
        typedef typename iterator::pointer      pointer_t;
        typedef typename iterator::reference    reference_t;

        iterator() :
            dp_( 0), val_()
        {}

        explicit iterator( D * dp) :
            dp_( dp), val_()
        { fetch_(); }

        iterator( iterator const& other) :
            dp_( other.dp_), val_( other.val_)
        {}

        iterator & operator=( iterator const& other)
        {
            if ( this == & other) return * this;
            dp_ = other.dp_;
            val_ = other.val_;
            return * this;
        }

        bool operator==( iterator const& other)
        { return other.dp_ == dp_ && other.val_ == val_; }

        bool operator!=( iterator const& other)
        { return other.dp_ != dp_ || other.val_ != val_; }

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
        { return const_cast< optional< Result > & >( val_).get(); }

        pointer_t operator->() const
        { return const_cast< optional< Result > & >( val_).get_ptr(); }
    };

    class const_iterator : public std::iterator< std::input_iterator_tag, const typename remove_reference< Result >::type >
    {
    private:
        D                   *   dp_;
        optional< Result >      val_;

        void fetch_()
        {
            BOOST_ASSERT( dp_);

            if ( ! dp_->has_result() )
            {
                dp_ = 0;
                val_ = none;
                return;
            }
            val_ = dp_->get();
        }

        void increment_()
        {
            BOOST_ASSERT( dp_);
            BOOST_ASSERT( * dp_);

            ( * dp_)();
            fetch_();
        }

    public:
        typedef typename const_iterator::pointer      pointer_t;
        typedef typename const_iterator::reference    reference_t;

        const_iterator() :
            dp_( 0), val_()
        {}

        explicit const_iterator( D const* dp) :
            dp_( const_cast< D * >( dp) ), val_()
        { fetch_(); }

        const_iterator( const_iterator const& other) :
            dp_( other.dp_), val_( other.val_)
        {}

        const_iterator & operator=( const_iterator const& other)
        {
            if ( this == & other) return * this;
            dp_ = other.dp_;
            val_ = other.val_;
            return * this;
        }

        bool operator==( const_iterator const& other)
        { return other.dp_ == dp_ && other.val_ == val_; }

        bool operator!=( const_iterator const& other)
        { return other.dp_ != dp_ || other.val_ != val_; }

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
        { return val_.get(); }

        pointer_t operator->() const
        { return val_.get_ptr(); }
    };

    D & operator()()
    {
        BOOST_ASSERT( static_cast< D * >( this)->impl_);
        BOOST_ASSERT( ! static_cast< D * >( this)->impl_->is_complete() );

        static_cast< D * >( this)->impl_->resume();

        return * static_cast< D * >( this);
    }
};

template< typename Signature, typename D >
struct coroutine_op< Signature, D, void, 1 >
{
    typedef typename arg< Signature >::type   arg_type;

    class iterator : public std::iterator< std::output_iterator_tag, void, void, void, void >
    {
    private:
       D    *   dp_;

    public:
        iterator() :
           dp_( 0)
        {}

        explicit iterator( D * dp) :
            dp_( dp)
        {}

        iterator & operator=( arg_type a1)
        {
            BOOST_ASSERT( dp_);
            if ( ! ( * dp_)( a1) ) dp_ = 0;
            return * this;
        }

        bool operator==( iterator const& other)
        { return other.dp_ == dp_; }

        bool operator!=( iterator const& other)
        { return other.dp_ != dp_; }

        iterator & operator*()
        { return * this; }

        iterator & operator++()
        { return * this; }
    };

    struct const_iterator;

    D & operator()( arg_type a1)
    {
        BOOST_ASSERT( static_cast< D * >( this)->impl_);
        BOOST_ASSERT( ! static_cast< D * >( this)->impl_->is_complete() );

        static_cast< D * >( this)->impl_->resume( a1);

        return * static_cast< D * >( this);
    }
};

template< typename Signature, typename D, typename Result >
struct coroutine_op< Signature, D, Result, 1 >
{
    typedef typename arg< Signature >::type   arg_type;

    D & operator()( arg_type a1)
    {
        BOOST_ASSERT( static_cast< D * >( this)->impl_);
        BOOST_ASSERT( ! static_cast< D * >( this)->impl_->is_complete() );

        static_cast< D * >( this)->impl_->resume( a1);

        return * static_cast< D * >( this);
    }
};

#define BOOST_COROUTINE_OP_COMMA(n) BOOST_PP_COMMA_IF(BOOST_PP_SUB(n,1))
#define BOOST_COROUTINE_OP_VAL(z,n,unused) BOOST_COROUTINE_OP_COMMA(n) BOOST_PP_CAT(a,n)
#define BOOST_COROUTINE_OP_VALS(n) BOOST_PP_REPEAT_FROM_TO(1,BOOST_PP_ADD(n,1),BOOST_COROUTINE_OP_VAL,~)
#define BOOST_COROUTINE_OP_ARG_TYPE(n) \
    typename function_traits< Signature >::BOOST_PP_CAT(BOOST_PP_CAT(arg,n),_type)
#define BOOST_COROUTINE_OP_ARG(z,n,unused) BOOST_COROUTINE_OP_COMMA(n) BOOST_COROUTINE_OP_ARG_TYPE(n) BOOST_PP_CAT(a,n)
#define BOOST_COROUTINE_OP_ARGS(n) BOOST_PP_REPEAT_FROM_TO(1,BOOST_PP_ADD(n,1),BOOST_COROUTINE_OP_ARG,~)
#define BOOST_COROUTINE_OP(z,n,unused) \
template< typename Signature, typename D, typename Result > \
struct coroutine_op< Signature, D, Result, n > \
{ \
    D & operator()( BOOST_COROUTINE_OP_ARGS(n)) \
    { \
        BOOST_ASSERT( static_cast< D * >( this)->impl_); \
        BOOST_ASSERT( ! static_cast< D * >( this)->impl_->is_complete() ); \
\
        static_cast< D * >( this)->impl_->resume(BOOST_COROUTINE_OP_VALS(n)); \
\
        return * static_cast< D * >( this); \
    } \
};
BOOST_PP_REPEAT_FROM_TO(2,11,BOOST_COROUTINE_OP,~)
#undef BOOST_COROUTINE_OP
#undef BOOST_COROUTINE_OP_ARGS
#undef BOOST_COROUTINE_OP_ARG
#undef BOOST_COROUTINE_OP_ARG_TYPE
#undef BOOST_COROUTINE_OP_VALS
#undef BOOST_COROUTINE_OP_VAL
#undef BOOST_COROUTINE_OP_COMMA

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_OLD_DETAIL_COROUTINE_OP_H

//
// Boost.Pointer Container
//
//  Copyright Thorsten Ottosen 2003-2005. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/ptr_container/
//


#ifndef BOOST_PTR_CONTAINER_DETAIL_REVERSIBLE_PTR_CONTAINER_HPP
#define BOOST_PTR_CONTAINER_DETAIL_REVERSIBLE_PTR_CONTAINER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include <boost/ptr_container/detail/throw_exception.hpp>
#include <boost/ptr_container/detail/scoped_deleter.hpp>
#include <boost/ptr_container/detail/static_move_ptr.hpp>
#include <boost/ptr_container/exception.hpp>
#include <boost/ptr_container/clone_allocator.hpp>
#include <boost/ptr_container/nullable.hpp>

#ifdef BOOST_NO_SFINAE
#else
#include <boost/range/functions.hpp>
#endif

#include <boost/config.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/range/iterator.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <typeinfo>
#include <memory>

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)  
#pragma warning(push)  
#pragma warning(disable:4127)
#endif  

namespace boost
{

namespace ptr_container_detail
{
    template< class CloneAllocator >
    struct clone_deleter
    {
        template< class T >
        void operator()( const T* p ) const
        {
            CloneAllocator::deallocate_clone( p );
        }
    };

    template< class T >
    struct is_pointer_or_integral
    {
        BOOST_STATIC_CONSTANT(bool, value = is_pointer<T>::value || is_integral<T>::value );
    };

    struct is_pointer_or_integral_tag {};
    struct is_range_tag {};
    struct sequence_tag {};
    struct fixed_length_sequence_tag : sequence_tag {};
    struct associative_container_tag {};
    struct ordered_associative_container_tag : associative_container_tag {};
    struct unordered_associative_container_tag : associative_container_tag {};
    

    
    template
    < 
        class Config, 
        class CloneAllocator
    >
    class reversible_ptr_container 
    {
    private:
        BOOST_STATIC_CONSTANT( bool, allow_null = Config::allow_null );
        
        typedef BOOST_DEDUCED_TYPENAME Config::value_type Ty_;

        template< bool allow_null_values >
        struct null_clone_allocator
        {
            template< class Iter >
            static Ty_* allocate_clone_from_iterator( Iter i )
            { 
                return allocate_clone( Config::get_const_pointer( i ) );
            }
            
            static Ty_* allocate_clone( const Ty_* x )
            {
                if( allow_null_values )
                {
                    if( x == 0 )
                        return 0;
                }
                else
                {
                    BOOST_ASSERT( x != 0 && "Cannot insert clone of null!" );
                }

                Ty_* res = CloneAllocator::allocate_clone( *x );
                BOOST_ASSERT( typeid(*res) == typeid(*x) &&
                              "CloneAllocator::allocate_clone() does not clone the "
                              "object properly. Check that new_clone() is implemented"
                              " correctly" );
                return res;
            }
            
            static void deallocate_clone( const Ty_* x )
            {
                if( allow_null_values )
                {
                    if( x == 0 )
                        return;
                }

                CloneAllocator::deallocate_clone( x );
            }
        };

        typedef BOOST_DEDUCED_TYPENAME Config::void_container_type  Cont;
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))    
        typedef  null_clone_allocator<reversible_ptr_container::allow_null> 
                                                                    null_cloner_type;
#else
        typedef null_clone_allocator<allow_null>                    null_cloner_type;
#endif        
        typedef clone_deleter<null_cloner_type>                     Deleter;

        Cont      c_;

    public:
        Cont&       base()               { return c_; }
    protected: // having this public could break encapsulation
        const Cont& base() const         { return c_; }        
        
    public: // typedefs
        typedef  Ty_*          value_type;
        typedef  Ty_*          pointer;
        typedef  Ty_&          reference;
        typedef  const Ty_&    const_reference;
        
        typedef  BOOST_DEDUCED_TYPENAME Config::iterator 
                                   iterator;
        typedef  BOOST_DEDUCED_TYPENAME Config::const_iterator
                                   const_iterator;
        typedef  boost::reverse_iterator< iterator > 
                                   reverse_iterator;  
        typedef  boost::reverse_iterator< const_iterator >     
                                   const_reverse_iterator;
        typedef  BOOST_DEDUCED_TYPENAME Cont::difference_type
                                   difference_type; 
        typedef  BOOST_DEDUCED_TYPENAME Cont::size_type
                                   size_type;
        typedef  BOOST_DEDUCED_TYPENAME Config::allocator_type
                                   allocator_type;
        typedef CloneAllocator     clone_allocator_type;
        typedef ptr_container_detail::static_move_ptr<Ty_,Deleter> 
                                   auto_type;
            
    protected: 
            
        typedef ptr_container_detail::scoped_deleter<Ty_,null_cloner_type>
                                   scoped_deleter;
        typedef BOOST_DEDUCED_TYPENAME Cont::iterator
                                   ptr_iterator; 
        typedef BOOST_DEDUCED_TYPENAME Cont::const_iterator
                                   ptr_const_iterator; 
    private:

        template< class InputIterator >  
        void copy( InputIterator first, InputIterator last ) 
        {
            std::copy( first, last, begin() );
        }
        
        void copy( const reversible_ptr_container& r )
        { 
            copy( r.begin(), r.end() );
        }
        
        void copy_clones_and_release( scoped_deleter& sd ) // nothrow
        {
            BOOST_ASSERT( size_type( std::distance( sd.begin(), sd.end() ) ) == c_.size() );
            std::copy( sd.begin(), sd.end(), c_.begin() );
            sd.release(); 
        }
        
        template< class ForwardIterator >
        void clone_assign( ForwardIterator first, 
                           ForwardIterator last ) // strong 
        {
            BOOST_ASSERT( first != last );
            scoped_deleter sd( first, last );      // strong
            copy_clones_and_release( sd );         // nothrow
        }

        template< class ForwardIterator >
        void clone_back_insert( ForwardIterator first,
                                ForwardIterator last )
        {
            BOOST_ASSERT( first != last );
            scoped_deleter sd( first, last );
            insert_clones_and_release( sd, end() );
        }
        
        void remove_all() 
        {
            remove( begin(), end() ); 
        }

    protected:

        void insert_clones_and_release( scoped_deleter& sd, 
                                        iterator where ) // strong
        {
            //
            // 'c_.insert' always provides the strong guarantee for T* elements
            // since a copy constructor of a pointer cannot throw
            //
            c_.insert( where.base(), 
                       sd.begin(), sd.end() ); 
            sd.release();
        }

        void insert_clones_and_release( scoped_deleter& sd ) // strong
        {
            c_.insert( sd.begin(), sd.end() );
            sd.release();
        }

        template< class U >
        void remove( U* ptr )
        {
            null_policy_deallocate_clone( ptr );
        }
        
        template< class I >
        void remove( I i )
        { 
            null_policy_deallocate_clone( Config::get_const_pointer(i) );
        }

        template< class I >
        void remove( I first, I last ) 
        {
            for( ; first != last; ++first )
                remove( first );
        }

        static void enforce_null_policy( const Ty_* x, const char* msg )
        {
            if( !allow_null )
            {
                BOOST_PTR_CONTAINER_THROW_EXCEPTION( 0 == x && "null not allowed", 
                                                     bad_pointer, msg );
            }
        }

        static Ty_* null_policy_allocate_clone( const Ty_* x )
        {
            return null_cloner_type::allocate_clone( x );
        }

        static void null_policy_deallocate_clone( const Ty_* x )
        {
            null_cloner_type::deallocate_clone( x );
        }

    private:
        template< class ForwardIterator >
        ForwardIterator advance( ForwardIterator begin, size_type n ) 
        {
            ForwardIterator iter = begin;
            std::advance( iter, n );
            return iter;
        }        

        template< class I >
        void constructor_impl( I first, I last, std::input_iterator_tag ) // basic
        {
            while( first != last )
            {
                insert( end(), null_cloner_type::allocate_clone_from_iterator(first) );
                ++first;
            }
        }

        template< class I >
        void constructor_impl( I first, I last, std::forward_iterator_tag ) // strong
        {
            if( first == last )
                return;
            clone_back_insert( first, last );
        }

        template< class I >
        void associative_constructor_impl( I first, I last ) // strong
        {
            if( first == last )
                return;

            scoped_deleter sd( first, last );
            insert_clones_and_release( sd );             
        }

    public: // foundation! should be protected!
        reversible_ptr_container()
        { }

        template< class SizeType >
        reversible_ptr_container( SizeType n, unordered_associative_container_tag )
          : c_( n )
        { }

        template< class SizeType >
        reversible_ptr_container( SizeType n, fixed_length_sequence_tag )
          : c_( n )
        { }

        template< class SizeType >
        reversible_ptr_container( SizeType n, const allocator_type& a, 
                                  fixed_length_sequence_tag )
          : c_( n, a )
        { }
        
        explicit reversible_ptr_container( const allocator_type& a ) 
         : c_( a )
        { }
        
        template< class PtrContainer >
        explicit reversible_ptr_container( std::auto_ptr<PtrContainer> clone )                
        { 
            swap( *clone ); 
        }

        reversible_ptr_container( const reversible_ptr_container& r ) 
        {
            constructor_impl( r.begin(), r.end(), std::forward_iterator_tag() ); 
        }

        template< class C, class V >
        reversible_ptr_container( const reversible_ptr_container<C,V>& r ) 
        {
            constructor_impl( r.begin(), r.end(), std::forward_iterator_tag() ); 
        }

        template< class PtrContainer >
        reversible_ptr_container& operator=( std::auto_ptr<PtrContainer> clone ) // nothrow
        {
            swap( *clone );
            return *this;
        }

        reversible_ptr_container& operator=( reversible_ptr_container r ) // strong 
        {
            swap( r );
            return *this;
        }
        
        // overhead: null-initilization of container pointer (very cheap compared to cloning)
        // overhead: 1 heap allocation (very cheap compared to cloning)
        template< class InputIterator >
        reversible_ptr_container( InputIterator first, 
                                  InputIterator last,
                                  const allocator_type& a = allocator_type() ) // basic, strong
          : c_( a )
        { 
            constructor_impl( first, last, 
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
#else
                              BOOST_DEDUCED_TYPENAME
#endif                              
                              iterator_category<InputIterator>::type() );
        }

        template< class Compare >
        reversible_ptr_container( const Compare& comp,
                                  const allocator_type& a )
          : c_( comp, a ) {}

        template< class ForwardIterator >
        reversible_ptr_container( ForwardIterator first,
                                  ForwardIterator last,
                                  fixed_length_sequence_tag )
          : c_( std::distance(first,last) )
        {
            constructor_impl( first, last, 
                              std::forward_iterator_tag() );
        }

        template< class SizeType, class InputIterator >
        reversible_ptr_container( SizeType n,
                                  InputIterator first,
                                  InputIterator last,
                                  fixed_length_sequence_tag )
          : c_( n )
        {
            constructor_impl( first, last, 
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
#else
                              BOOST_DEDUCED_TYPENAME
#endif                              
                              iterator_category<InputIterator>::type() );
        }

        template< class Compare >
        reversible_ptr_container( const Compare& comp,
                                  const allocator_type& a,
                                  associative_container_tag )
          : c_( comp, a )
        { }
                
        template< class InputIterator >
        reversible_ptr_container( InputIterator first,
                                  InputIterator last,
                                  associative_container_tag )
        {
            associative_constructor_impl( first, last );
        }
        
        template< class InputIterator, class Compare >
        reversible_ptr_container( InputIterator first,
                                  InputIterator last,
                                  const Compare& comp,
                                  const allocator_type& a,
                                  associative_container_tag )
          : c_( comp, a ) 
        {
            associative_constructor_impl( first, last );
        }

        explicit reversible_ptr_container( size_type n )
          : c_( n ) {}

        template< class Hash, class Pred >
        reversible_ptr_container( const Hash& h,
                                  const Pred& pred,
                                  const allocator_type& a )
          : c_( h, pred, a ) {}

        template< class InputIterator, class Hash, class Pred >
        reversible_ptr_container( InputIterator first,
                                  InputIterator last,
                                  const Hash& h,
                                  const Pred& pred,
                                  const allocator_type& a )
          : c_( h, pred, a )
        {
            associative_constructor_impl( first, last );
        }

    public:        
        ~reversible_ptr_container()
        { 
            remove_all();
        }
        
    public:
        
        allocator_type get_allocator() const                   
        {
            return c_.get_allocator(); 
        }
 
    public: // container requirements
        iterator begin()            
            { return iterator( c_.begin() ); }
        const_iterator begin() const      
            { return const_iterator( c_.begin() ); }
        iterator end()              
            { return iterator( c_.end() ); }
        const_iterator end() const        
            { return const_iterator( c_.end() ); }
        
        reverse_iterator rbegin()           
            { return reverse_iterator( this->end() ); } 
        const_reverse_iterator rbegin() const     
            { return const_reverse_iterator( this->end() ); } 
        reverse_iterator rend()             
            { return reverse_iterator( this->begin() ); } 
        const_reverse_iterator rend() const       
            { return const_reverse_iterator( this->begin() ); } 

        const_iterator cbegin() const      
            { return const_iterator( c_.begin() ); }
        const_iterator cend() const             
            { return const_iterator( c_.end() ); }

        const_reverse_iterator crbegin() const      
            { return const_reverse_iterator( this->end() ); }
        const_reverse_iterator crend() const             
            { return const_reverse_iterator( this->begin() ); }

        void swap( reversible_ptr_container& r ) // nothrow
        { 
            c_.swap( r.c_ );
        }
          
        size_type size() const // nothrow
        {
            return c_.size();
        }

        size_type max_size() const // nothrow
        {
            return c_.max_size(); 
        }
        
        bool empty() const // nothrow
        {
            return c_.empty();
        }

    public: // optional container requirements

        bool operator==( const reversible_ptr_container& r ) const // nothrow
        { 
            if( size() != r.size() )
                return false;
            else
                return std::equal( begin(), end(), r.begin() );
        }

        bool operator!=( const reversible_ptr_container& r ) const // nothrow
        {
            return !(*this == r);
        }
        
        bool operator<( const reversible_ptr_container& r ) const // nothrow 
        {
             return std::lexicographical_compare( begin(), end(), r.begin(), r.end() );
        }

        bool operator<=( const reversible_ptr_container& r ) const // nothrow 
        {
            return !(r < *this);
        }

        bool operator>( const reversible_ptr_container& r ) const // nothrow 
        {
            return r < *this;
        }

        bool operator>=( const reversible_ptr_container& r ) const // nothrow 
        {
            return !(*this < r);
        }

    public: // modifiers

        iterator insert( iterator before, Ty_* x )
        {
            enforce_null_policy( x, "Null pointer in 'insert()'" );

            auto_type ptr( x );                            // nothrow
            iterator res( c_.insert( before.base(), x ) ); // strong, commit
            ptr.release();                                 // nothrow
            return res;
        }

        template< class U >
        iterator insert( iterator before, std::auto_ptr<U> x )
        {
            return insert( before, x.release() );
        }

        iterator erase( iterator x ) // nothrow
        {
            BOOST_ASSERT( !empty() );
            BOOST_ASSERT( x != end() );

            remove( x );
            return iterator( c_.erase( x.base() ) );
        }

        iterator erase( iterator first, iterator last ) // nothrow
        {
            remove( first, last );
            return iterator( c_.erase( first.base(),
                                       last.base() ) );
        }

        template< class Range >
        iterator erase( const Range& r )
        {
            return erase( boost::begin(r), boost::end(r) );
        }

        void clear()
        {
            remove_all();
            c_.clear();
        }
        
    public: // access interface
        
        auto_type release( iterator where )
        { 
            BOOST_ASSERT( where != end() );
            
            BOOST_PTR_CONTAINER_THROW_EXCEPTION( empty(), bad_ptr_container_operation,
                                                 "'release()' on empty container" ); 
            
            auto_type ptr( Config::get_pointer( where ) );  // nothrow
            c_.erase( where.base() );                       // nothrow
            return boost::ptr_container_detail::move( ptr ); 
        }

        auto_type replace( iterator where, Ty_* x ) // strong  
        { 
            BOOST_ASSERT( where != end() );

            enforce_null_policy( x, "Null pointer in 'replace()'" );
            
            auto_type ptr( x );
            
            BOOST_PTR_CONTAINER_THROW_EXCEPTION( empty(), bad_ptr_container_operation,
                                                 "'replace()' on empty container" );

            auto_type old( Config::get_pointer( where ) );  // nothrow            
            const_cast<void*&>(*where.base()) = ptr.release();                
            return boost::ptr_container_detail::move( old );
        }

        template< class U >
        auto_type replace( iterator where, std::auto_ptr<U> x )
        {
            return replace( where, x.release() ); 
        }

        auto_type replace( size_type idx, Ty_* x ) // strong
        {
            enforce_null_policy( x, "Null pointer in 'replace()'" );
            
            auto_type ptr( x ); 
            
            BOOST_PTR_CONTAINER_THROW_EXCEPTION( idx >= size(), bad_index, 
                                                 "'replace()' out of bounds" );
            
            auto_type old( static_cast<Ty_*>( c_[idx] ) ); // nothrow
            c_[idx] = ptr.release();                       // nothrow, commit
            return boost::ptr_container_detail::move( old );
        } 

        template< class U >
        auto_type replace( size_type idx, std::auto_ptr<U> x )
        {
            return replace( idx, x.release() );
        }
                
    }; // 'reversible_ptr_container'


#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))    
#define BOOST_PTR_CONTAINER_DEFINE_RELEASE( base_type ) \
    typename base_type::auto_type                   \
    release( typename base_type::iterator i )       \
    {                                               \
        return boost::ptr_container_detail::move(base_type::release(i)); \
    }                                               
#else
#define BOOST_PTR_CONTAINER_DEFINE_RELEASE( base_type ) \
    using base_type::release;
#endif
    
    //
    // two-phase lookup of template functions 
    // is buggy on most compilers, so we use a macro instead
    //
#define BOOST_PTR_CONTAINER_DEFINE_RELEASE_AND_CLONE( PC, base_type, this_type ) \
    explicit PC( std::auto_ptr<this_type> r )       \
    : base_type ( r ) { }                           \
                                                    \
    PC& operator=( std::auto_ptr<this_type> r )     \
    {                                               \
        base_type::operator=( r );                  \
        return *this;                               \
    }                                               \
                                                    \
    std::auto_ptr<this_type> release()              \
    {                                               \
      std::auto_ptr<this_type> ptr( new this_type );\
      this->swap( *ptr );                           \
      return ptr;                                   \
    }                                               \
    BOOST_PTR_CONTAINER_DEFINE_RELEASE( base_type ) \
                                                    \
    std::auto_ptr<this_type> clone() const          \
    {                                               \
       return std::auto_ptr<this_type>( new this_type( this->begin(), this->end() ) ); \
    }

#define BOOST_PTR_CONTAINER_DEFINE_COPY_CONSTRUCTORS( PC, base_type ) \
                                                                      \
    template< class U >                                               \
    PC( const PC<U>& r ) : base_type( r ) { }                         \
                                                                      \
    PC& operator=( PC r )                                             \
    {                                                                 \
        this->swap( r );                                              \
        return *this;                                                 \
    }                                                                 \
                                                                           

#define BOOST_PTR_CONTAINER_DEFINE_CONSTRUCTORS( PC, base_type )                       \
    typedef BOOST_DEDUCED_TYPENAME base_type::iterator        iterator;                \
    typedef BOOST_DEDUCED_TYPENAME base_type::size_type       size_type;               \
    typedef BOOST_DEDUCED_TYPENAME base_type::const_reference const_reference;         \
    typedef BOOST_DEDUCED_TYPENAME base_type::allocator_type  allocator_type;          \
    PC() {}                                                                            \
    explicit PC( const allocator_type& a ) : base_type(a) {}                           \
    template< class InputIterator >                                                    \
    PC( InputIterator first, InputIterator last ) : base_type( first, last ) {}        \
    template< class InputIterator >                                                    \
    PC( InputIterator first, InputIterator last,                                       \
        const allocator_type& a ) : base_type( first, last, a ) {}  
                 
#define BOOST_PTR_CONTAINER_DEFINE_NON_INHERITED_MEMBERS( PC, base_type, this_type )           \
   BOOST_PTR_CONTAINER_DEFINE_CONSTRUCTORS( PC, base_type )                                    \
   BOOST_PTR_CONTAINER_DEFINE_RELEASE_AND_CLONE( PC, base_type, this_type )

#define BOOST_PTR_CONTAINER_DEFINE_SEQEUENCE_MEMBERS( PC, base_type, this_type )  \
    BOOST_PTR_CONTAINER_DEFINE_NON_INHERITED_MEMBERS( PC, base_type, this_type )  \
    BOOST_PTR_CONTAINER_DEFINE_COPY_CONSTRUCTORS( PC, base_type )

} // namespace 'ptr_container_detail'

    //
    // @remark: expose movability of internal move-pointer
    //
    namespace ptr_container
    {        
        using ptr_container_detail::move;
    }

} // namespace 'boost'  

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)  
#pragma warning(pop)  
#endif  

#endif

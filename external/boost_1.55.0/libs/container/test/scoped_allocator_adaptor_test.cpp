//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2011-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/container/detail/config_begin.hpp>
#include <boost/container/scoped_allocator_fwd.hpp>
#include <boost/container/detail/utilities.hpp>
#include <cstddef>
#include <boost/container/detail/mpl.hpp>
#include <boost/move/utility.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <memory>

using namespace boost::container;

template<class T, unsigned int Id, bool Propagate = false>
class test_allocator
{
   BOOST_COPYABLE_AND_MOVABLE(test_allocator)
   public:

   template<class U>
   struct rebind
   {
      typedef test_allocator<U, Id, Propagate> other;
   };

   typedef container_detail::bool_<Propagate>  propagate_on_container_copy_assignment;
   typedef container_detail::bool_<Propagate>  propagate_on_container_move_assignment;
   typedef container_detail::bool_<Propagate>  propagate_on_container_swap;
   typedef T value_type;

   test_allocator()
      : m_move_contructed(false), m_move_assigned(false)
   {}

   test_allocator(const test_allocator&)
      : m_move_contructed(false), m_move_assigned(false)
   {}

   test_allocator(BOOST_RV_REF(test_allocator) )
      : m_move_contructed(true), m_move_assigned(false)
   {}

   template<class U>
   test_allocator(BOOST_RV_REF_BEG test_allocator<U, Id, Propagate> BOOST_RV_REF_END)
      : m_move_contructed(true), m_move_assigned(false)
   {}

   template<class U>
   test_allocator(const test_allocator<U, Id, Propagate> &)
   {}

   test_allocator & operator=(BOOST_COPY_ASSIGN_REF(test_allocator))
   {
      return *this;
   }

   test_allocator & operator=(BOOST_RV_REF(test_allocator))
   {
      m_move_assigned = true;
      return *this;
   }

   std::size_t max_size() const
   {  return std::size_t(Id);  }

   T* allocate(std::size_t n)
   {  return (T*)::new char[n*sizeof(T)];  }

   void deallocate(T*p, std::size_t)
   {  delete []static_cast<char*>(static_cast<void*>(p));  }

   bool m_move_contructed;
   bool m_move_assigned;
};

template <class T1, class T2, unsigned int Id, bool Propagate>
bool operator==( const test_allocator<T1, Id, Propagate>&
               , const test_allocator<T2, Id, Propagate>&)
{  return true;   }

template <class T1, class T2, unsigned int Id, bool Propagate>
bool operator!=( const test_allocator<T1, Id, Propagate>&
               , const test_allocator<T2, Id, Propagate>&)
{  return false;   }


template<unsigned int Type>
struct tagged_integer
{};

struct mark_on_destructor
{
   mark_on_destructor()
      : destroyed(false)
   {}

   ~mark_on_destructor()
   {
      destroyed = true;
   }

   bool destroyed;
};

//This enum lists the construction options
//for an allocator-aware type
enum ConstructionTypeEnum
{
   ConstructiblePrefix,
   ConstructibleSuffix,
   NotUsesAllocator,
};

//This base class provices types for
//the derived class to implement each construction
//type. If a construction type does not apply
//the typedef is set to an internal nat
//so that the class is not constructible from
//the user arguments.
template<ConstructionTypeEnum ConstructionType, unsigned int AllocatorTag>
struct uses_allocator_base;

template<unsigned int AllocatorTag>
struct uses_allocator_base<ConstructibleSuffix, AllocatorTag>
{
   typedef test_allocator<int, AllocatorTag> allocator_type;
   typedef allocator_type allocator_constructor_type;
   struct nat{};
   typedef nat allocator_arg_type;
};

template<unsigned int AllocatorTag>
struct uses_allocator_base<ConstructiblePrefix, AllocatorTag>
{
   typedef test_allocator<int, AllocatorTag> allocator_type;
   typedef allocator_type allocator_constructor_type;
   typedef allocator_arg_t allocator_arg_type;
};

template<unsigned int AllocatorTag>
struct uses_allocator_base<NotUsesAllocator, AllocatorTag>
{
   struct nat{};
   typedef nat allocator_constructor_type;
   typedef nat allocator_arg_type;
};

template<ConstructionTypeEnum ConstructionType, unsigned int AllocatorTag>
struct mark_on_scoped_allocation
   : uses_allocator_base<ConstructionType, AllocatorTag>
{
   private:
   BOOST_COPYABLE_AND_MOVABLE(mark_on_scoped_allocation)

   public:

   typedef uses_allocator_base<ConstructionType, AllocatorTag> base_type;

   //0 user argument constructors
   mark_on_scoped_allocation()
      : construction_type(NotUsesAllocator), value(0)
   {}

   explicit mark_on_scoped_allocation
      (typename base_type::allocator_constructor_type)
      : construction_type(ConstructibleSuffix), value(0)
   {}

   explicit mark_on_scoped_allocation
      (typename base_type::allocator_arg_type, typename base_type::allocator_constructor_type)
      : construction_type(ConstructiblePrefix), value(0)
   {}

   //1 user argument constructors
   explicit mark_on_scoped_allocation(int i)
      : construction_type(NotUsesAllocator), value(i)
   {}

   mark_on_scoped_allocation
      (int i, typename base_type::allocator_constructor_type)
      : construction_type(ConstructibleSuffix), value(i)
   {}

   mark_on_scoped_allocation
      ( typename base_type::allocator_arg_type
      , typename base_type::allocator_constructor_type
      , int i)
      : construction_type(ConstructiblePrefix), value(i)
   {}

   //Copy constructors
   mark_on_scoped_allocation(const mark_on_scoped_allocation &other)
      : construction_type(NotUsesAllocator), value(other.value)
   {}

   mark_on_scoped_allocation( const mark_on_scoped_allocation &other
                            , typename base_type::allocator_constructor_type)
      : construction_type(ConstructibleSuffix), value(other.value)
   {}

   mark_on_scoped_allocation( typename base_type::allocator_arg_type
                            , typename base_type::allocator_constructor_type
                            , const mark_on_scoped_allocation &other)
      : construction_type(ConstructiblePrefix), value(other.value)
   {}

   //Move constructors
   mark_on_scoped_allocation(BOOST_RV_REF(mark_on_scoped_allocation) other)
      : construction_type(NotUsesAllocator), value(other.value)
   {  other.value = 0;  other.construction_type = NotUsesAllocator;  }

   mark_on_scoped_allocation( BOOST_RV_REF(mark_on_scoped_allocation) other
                            , typename base_type::allocator_constructor_type)
      : construction_type(ConstructibleSuffix), value(other.value)
   {  other.value = 0;  other.construction_type = ConstructibleSuffix;  }

   mark_on_scoped_allocation( typename base_type::allocator_arg_type
                            , typename base_type::allocator_constructor_type
                            , BOOST_RV_REF(mark_on_scoped_allocation) other)
      : construction_type(ConstructiblePrefix), value(other.value)
   {  other.value = 0;  other.construction_type = ConstructiblePrefix;  }

   ConstructionTypeEnum construction_type;
   int                  value;
};

namespace boost {
namespace container {

template<unsigned int AllocatorTag>
struct constructible_with_allocator_prefix
   < ::mark_on_scoped_allocation<ConstructiblePrefix, AllocatorTag> >
   : ::boost::true_type
{};

template<unsigned int AllocatorTag>
struct constructible_with_allocator_suffix
   < ::mark_on_scoped_allocation<ConstructibleSuffix, AllocatorTag> >
   : ::boost::true_type
{};

}  //namespace container {
}  //namespace boost {


#include <boost/container/scoped_allocator.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>
#include <boost/container/vector.hpp>
#include <boost/container/detail/pair.hpp>

int main()
{
   typedef test_allocator<tagged_integer<0>, 0>   OuterAlloc;
   typedef test_allocator<tagged_integer<0>, 10>  Outer10IdAlloc;
   typedef test_allocator<tagged_integer<9>, 0>   Rebound9OuterAlloc;
   typedef test_allocator<tagged_integer<1>, 1>   InnerAlloc1;
   typedef test_allocator<tagged_integer<2>, 2>   InnerAlloc2;
   typedef test_allocator<tagged_integer<1>, 11>  Inner11IdAlloc1;

   typedef test_allocator<tagged_integer<0>, 0, false>      OuterAllocFalsePropagate;
   typedef test_allocator<tagged_integer<0>, 0, true>       OuterAllocTruePropagate;
   typedef test_allocator<tagged_integer<1>, 1, false>      InnerAlloc1FalsePropagate;
   typedef test_allocator<tagged_integer<1>, 1, true>       InnerAlloc1TruePropagate;
   typedef test_allocator<tagged_integer<2>, 2, false>      InnerAlloc2FalsePropagate;
   typedef test_allocator<tagged_integer<2>, 2, true>       InnerAlloc2TruePropagate;

   //
   typedef scoped_allocator_adaptor< OuterAlloc  >          Scoped0Inner;
   typedef scoped_allocator_adaptor< OuterAlloc
                                   , InnerAlloc1 >          Scoped1Inner;
   typedef scoped_allocator_adaptor< OuterAlloc
                                   , InnerAlloc1
                                   , InnerAlloc2 >          Scoped2Inner;
   typedef scoped_allocator_adaptor
      < scoped_allocator_adaptor
         <Outer10IdAlloc>
      >                                                     ScopedScoped0Inner;
   typedef scoped_allocator_adaptor
      < scoped_allocator_adaptor
         <Outer10IdAlloc, Inner11IdAlloc1>
      , InnerAlloc1
      >                                                     ScopedScoped1Inner;
   typedef scoped_allocator_adaptor< Rebound9OuterAlloc  >  Rebound9Scoped0Inner;
   typedef scoped_allocator_adaptor< Rebound9OuterAlloc
                                   , InnerAlloc1 >          Rebound9Scoped1Inner;
   typedef scoped_allocator_adaptor< Rebound9OuterAlloc
                                   , InnerAlloc1
                                   , InnerAlloc2 >          Rebound9Scoped2Inner;

   //outer_allocator_type
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< OuterAlloc
                       , Scoped0Inner::outer_allocator_type>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< OuterAlloc
                       , Scoped1Inner::outer_allocator_type>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< OuterAlloc
                       , Scoped2Inner::outer_allocator_type>::value ));
   //value_type
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::value_type
                       , Scoped0Inner::value_type>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::value_type
                       , Scoped1Inner::value_type>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::value_type
                       , Scoped2Inner::value_type>::value ));
   //size_type
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::size_type
                       , Scoped0Inner::size_type>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::size_type
                       , Scoped1Inner::size_type>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::size_type
                       , Scoped2Inner::size_type>::value ));

   //difference_type
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::difference_type
                       , Scoped0Inner::difference_type>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::difference_type
                       , Scoped1Inner::difference_type>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::difference_type
                       , Scoped2Inner::difference_type>::value ));

   //pointer
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::pointer
                       , Scoped0Inner::pointer>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::pointer
                       , Scoped1Inner::pointer>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::pointer
                       , Scoped2Inner::pointer>::value ));

   //const_pointer
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::const_pointer
                       , Scoped0Inner::const_pointer>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::const_pointer
                       , Scoped1Inner::const_pointer>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::const_pointer
                       , Scoped2Inner::const_pointer>::value ));

   //void_pointer
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::void_pointer
                       , Scoped0Inner::void_pointer>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::void_pointer
                       , Scoped1Inner::void_pointer>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::void_pointer
                       , Scoped2Inner::void_pointer>::value ));

   //const_void_pointer
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::const_void_pointer
                       , Scoped0Inner::const_void_pointer>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::const_void_pointer
                       , Scoped1Inner::const_void_pointer>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< allocator_traits<OuterAlloc>::const_void_pointer
                       , Scoped2Inner::const_void_pointer>::value ));

   //rebind
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same<Scoped0Inner::rebind< tagged_integer<9> >::other
                       , Rebound9Scoped0Inner >::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same<Scoped1Inner::rebind< tagged_integer<9> >::other
                       , Rebound9Scoped1Inner >::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same<Scoped2Inner::rebind< tagged_integer<9> >::other
                       , Rebound9Scoped2Inner >::value ));

   //inner_allocator_type
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< Scoped0Inner
                       , Scoped0Inner::inner_allocator_type>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< scoped_allocator_adaptor<InnerAlloc1>
                       , Scoped1Inner::inner_allocator_type>::value ));
   BOOST_STATIC_ASSERT(( boost::container::container_detail::is_same< scoped_allocator_adaptor<InnerAlloc1, InnerAlloc2>
                       , Scoped2Inner::inner_allocator_type>::value ));

   {
      //Propagation test
      typedef scoped_allocator_adaptor< OuterAllocFalsePropagate  >  Scoped0InnerF;
      typedef scoped_allocator_adaptor< OuterAllocTruePropagate  >   Scoped0InnerT;
      typedef scoped_allocator_adaptor< OuterAllocFalsePropagate
                                    , InnerAlloc1FalsePropagate >  Scoped1InnerFF;
      typedef scoped_allocator_adaptor< OuterAllocFalsePropagate
                                    , InnerAlloc1TruePropagate >   Scoped1InnerFT;
      typedef scoped_allocator_adaptor< OuterAllocTruePropagate
                                    , InnerAlloc1FalsePropagate >  Scoped1InnerTF;
      typedef scoped_allocator_adaptor< OuterAllocTruePropagate
                                    , InnerAlloc1TruePropagate >   Scoped1InnerTT;
      typedef scoped_allocator_adaptor< OuterAllocFalsePropagate
                                    , InnerAlloc1FalsePropagate
                                    , InnerAlloc2FalsePropagate >  Scoped2InnerFFF;
      typedef scoped_allocator_adaptor< OuterAllocFalsePropagate
                                    , InnerAlloc1FalsePropagate
                                    , InnerAlloc2TruePropagate >  Scoped2InnerFFT;
      typedef scoped_allocator_adaptor< OuterAllocFalsePropagate
                                    , InnerAlloc1TruePropagate
                                    , InnerAlloc2FalsePropagate >  Scoped2InnerFTF;
      typedef scoped_allocator_adaptor< OuterAllocFalsePropagate
                                    , InnerAlloc1TruePropagate
                                    , InnerAlloc2TruePropagate >  Scoped2InnerFTT;
      typedef scoped_allocator_adaptor< OuterAllocTruePropagate
                                    , InnerAlloc1FalsePropagate
                                    , InnerAlloc2FalsePropagate >  Scoped2InnerTFF;
      typedef scoped_allocator_adaptor< OuterAllocTruePropagate
                                    , InnerAlloc1FalsePropagate
                                    , InnerAlloc2TruePropagate >  Scoped2InnerTFT;
      typedef scoped_allocator_adaptor< OuterAllocTruePropagate
                                    , InnerAlloc1TruePropagate
                                    , InnerAlloc2FalsePropagate >  Scoped2InnerTTF;
      typedef scoped_allocator_adaptor< OuterAllocTruePropagate
                                    , InnerAlloc1TruePropagate
                                    , InnerAlloc2TruePropagate >  Scoped2InnerTTT;

      //propagate_on_container_copy_assignment
      //0 inner
      BOOST_STATIC_ASSERT(( !Scoped0InnerF::propagate_on_container_copy_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped0InnerT::propagate_on_container_copy_assignment::value ));
      //1 inner
      BOOST_STATIC_ASSERT(( !Scoped1InnerFF::propagate_on_container_copy_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped1InnerFT::propagate_on_container_copy_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped1InnerTF::propagate_on_container_copy_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped1InnerTT::propagate_on_container_copy_assignment::value ));
      //2 inner
      BOOST_STATIC_ASSERT(( !Scoped2InnerFFF::propagate_on_container_copy_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerFFT::propagate_on_container_copy_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerFTF::propagate_on_container_copy_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerFTT::propagate_on_container_copy_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerTFF::propagate_on_container_copy_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerTFT::propagate_on_container_copy_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerTTF::propagate_on_container_copy_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerTTT::propagate_on_container_copy_assignment::value ));

      //propagate_on_container_move_assignment
      //0 inner
      BOOST_STATIC_ASSERT(( !Scoped0InnerF::propagate_on_container_move_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped0InnerT::propagate_on_container_move_assignment::value ));
      //1 inner
      BOOST_STATIC_ASSERT(( !Scoped1InnerFF::propagate_on_container_move_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped1InnerFT::propagate_on_container_move_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped1InnerTF::propagate_on_container_move_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped1InnerTT::propagate_on_container_move_assignment::value ));
      //2 inner
      BOOST_STATIC_ASSERT(( !Scoped2InnerFFF::propagate_on_container_move_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerFFT::propagate_on_container_move_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerFTF::propagate_on_container_move_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerFTT::propagate_on_container_move_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerTFF::propagate_on_container_move_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerTFT::propagate_on_container_move_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerTTF::propagate_on_container_move_assignment::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerTTT::propagate_on_container_move_assignment::value ));

      //propagate_on_container_swap
      //0 inner
      BOOST_STATIC_ASSERT(( !Scoped0InnerF::propagate_on_container_swap::value ));
      BOOST_STATIC_ASSERT((  Scoped0InnerT::propagate_on_container_swap::value ));
      //1 inner
      BOOST_STATIC_ASSERT(( !Scoped1InnerFF::propagate_on_container_swap::value ));
      BOOST_STATIC_ASSERT((  Scoped1InnerFT::propagate_on_container_swap::value ));
      BOOST_STATIC_ASSERT((  Scoped1InnerTF::propagate_on_container_swap::value ));
      BOOST_STATIC_ASSERT((  Scoped1InnerTT::propagate_on_container_swap::value ));
      //2 inner
      BOOST_STATIC_ASSERT(( !Scoped2InnerFFF::propagate_on_container_swap::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerFFT::propagate_on_container_swap::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerFTF::propagate_on_container_swap::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerFTT::propagate_on_container_swap::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerTFF::propagate_on_container_swap::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerTFT::propagate_on_container_swap::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerTTF::propagate_on_container_swap::value ));
      BOOST_STATIC_ASSERT((  Scoped2InnerTTT::propagate_on_container_swap::value ));
   }

   //Default constructor
   {
      Scoped0Inner s0i;
      Scoped1Inner s1i;
      //Swap
      {
         Scoped0Inner s0i2;
         Scoped1Inner s1i2;
         boost::container::swap_dispatch(s0i, s0i2);
         boost::container::swap_dispatch(s1i, s1i2);
      }
   }

   //Default constructor
   {
      Scoped0Inner s0i;
      Scoped1Inner s1i;
   }

   //Copy constructor/assignment
   {
      Scoped0Inner s0i;
      Scoped1Inner s1i;
      Scoped2Inner s2i;

      Scoped0Inner s0i_b(s0i);
      Scoped1Inner s1i_b(s1i);
      Scoped2Inner s2i_b(s2i);

      if(!(s0i == s0i_b) ||
         !(s1i == s1i_b) ||
         !(s2i == s2i_b)
         ){
         return 1;
      }

      s0i_b = s0i;
      s1i_b = s1i;
      s2i_b = s2i;

      if(!(s0i == s0i_b) ||
         !(s1i == s1i_b) ||
         !(s2i == s2i_b)
         ){
         return 1;
      }
   }

   //Copy/move constructor/assignment
   {
      Scoped0Inner s0i;
      Scoped1Inner s1i;
      Scoped2Inner s2i;

      Scoped0Inner s0i_b(::boost::move(s0i));
      Scoped1Inner s1i_b(::boost::move(s1i));
      Scoped2Inner s2i_b(::boost::move(s2i));

      if(!(s0i_b.outer_allocator().m_move_contructed) ||
         !(s1i_b.outer_allocator().m_move_contructed) ||
         !(s2i_b.outer_allocator().m_move_contructed)
         ){
         return 1;
      }

      s0i_b = ::boost::move(s0i);
      s1i_b = ::boost::move(s1i);
      s2i_b = ::boost::move(s2i);

      if(!(s0i_b.outer_allocator().m_move_assigned) ||
         !(s1i_b.outer_allocator().m_move_assigned) ||
         !(s2i_b.outer_allocator().m_move_assigned)
         ){
         return 1;
      }
   }

   //inner_allocator()
   {
      Scoped0Inner s0i;
      Scoped1Inner s1i;
      Scoped2Inner s2i;
      const Scoped0Inner const_s0i;
      const Scoped1Inner const_s1i;
      const Scoped2Inner const_s2i;

      Scoped0Inner::inner_allocator_type &s0i_inner =             s0i.inner_allocator();
      (void)s0i_inner;
      const Scoped0Inner::inner_allocator_type &const_s0i_inner = const_s0i.inner_allocator();
      (void)const_s0i_inner;
      Scoped1Inner::inner_allocator_type &s1i_inner =             s1i.inner_allocator();
      (void)s1i_inner;
      const Scoped1Inner::inner_allocator_type &const_s1i_inner = const_s1i.inner_allocator();
      (void)const_s1i_inner;
      Scoped2Inner::inner_allocator_type &s2i_inner =             s2i.inner_allocator();
      (void)s2i_inner;
      const Scoped2Inner::inner_allocator_type &const_s2i_inner = const_s2i.inner_allocator();
      (void)const_s2i_inner;
   }

   //operator==/!=
   {
      const Scoped0Inner const_s0i;
      const Rebound9Scoped0Inner const_rs0i;
      if(!(const_s0i == const_s0i) ||
         !(const_rs0i == const_s0i)){
         return 1;
      }
      if(  const_s0i != const_s0i ||
           const_s0i != const_rs0i ){
         return 1;
      }

      const Scoped1Inner const_s1i;
      const Rebound9Scoped1Inner const_rs1i;
      if(!(const_s1i == const_s1i) ||
         !(const_rs1i == const_s1i)){
         return 1;
      }
      if(  const_s1i != const_s1i ||
           const_s1i != const_rs1i ){
         return 1;
      }
      const Scoped2Inner const_s2i;
      const Rebound9Scoped2Inner const_rs2i;
      if(!(const_s2i == const_s2i) ||
         !(const_s2i == const_rs2i) ){
         return 1;
      }
      if(  const_s2i != const_s2i ||
           const_s2i != const_rs2i ){
         return 1;
      }
   }

   //outer_allocator()
   {
      Scoped0Inner s0i;
      Scoped1Inner s1i;
      Scoped2Inner s2i;
      const Scoped0Inner const_s0i;
      const Scoped1Inner const_s1i;
      const Scoped2Inner const_s2i;

      Scoped0Inner::outer_allocator_type &s0i_inner =             s0i.outer_allocator();
      (void)s0i_inner;
      const Scoped0Inner::outer_allocator_type &const_s0i_inner = const_s0i.outer_allocator();
      (void)const_s0i_inner;
      Scoped1Inner::outer_allocator_type &s1i_inner =             s1i.outer_allocator();
      (void)s1i_inner;
      const Scoped1Inner::outer_allocator_type &const_s1i_inner = const_s1i.outer_allocator();
      (void)const_s1i_inner;
      Scoped2Inner::outer_allocator_type &s2i_inner =             s2i.outer_allocator();
      (void)s2i_inner;
      const Scoped2Inner::outer_allocator_type &const_s2i_inner = const_s2i.outer_allocator();
      (void)const_s2i_inner;
   }

   //max_size()
   {
      const Scoped0Inner const_s0i;
      const Scoped1Inner const_s1i;
      const Scoped2Inner const_s2i;
      const OuterAlloc  const_oa;
      const InnerAlloc1 const_ia1;
      const InnerAlloc2 const_ia2;

      if(const_s0i.max_size() != const_oa.max_size()){
         return 1;
      }

      if(const_s1i.max_size() != const_oa.max_size()){
         return 1;
      }

      if(const_s2i.max_size() != const_oa.max_size()){
         return 1;
      }

      if(const_s1i.inner_allocator().max_size() != const_ia1.max_size()){
         return 1;
      }

      if(const_s2i.inner_allocator().inner_allocator().max_size() != const_ia2.max_size()){
         return 1;
      }
   }
   //Copy and move operations
   {
      //Construction
      {
         Scoped0Inner s0i_a, s0i_b(s0i_a), s0i_c(::boost::move(s0i_b));
         Scoped1Inner s1i_a, s1i_b(s1i_a), s1i_c(::boost::move(s1i_b));
         Scoped2Inner s2i_a, s2i_b(s2i_a), s2i_c(::boost::move(s2i_b));
      }
      //Assignment
      {
         Scoped0Inner s0i_a, s0i_b;
         s0i_a = s0i_b;
         s0i_a = ::boost::move(s0i_b);
         Scoped1Inner s1i_a, s1i_b;
         s1i_a = s1i_b;
         s1i_a = ::boost::move(s1i_b);
         Scoped2Inner s2i_a, s2i_b;
         s2i_a = s2i_b;
         s2i_a = ::boost::move(s2i_b);
      }

      OuterAlloc  oa;
      InnerAlloc1 ia1;
      InnerAlloc2 ia2;
      Rebound9OuterAlloc roa;
      Rebound9Scoped0Inner rs0i;
      Rebound9Scoped1Inner rs1i;
      Rebound9Scoped2Inner rs2i;

      //Copy from outer
      {
         Scoped0Inner s0i(oa);
         Scoped1Inner s1i(oa, ia1);
         Scoped2Inner s2i(oa, ia1, ia2);
      }
      //Move from outer
      {
         Scoped0Inner s0i(::boost::move(oa));
         Scoped1Inner s1i(::boost::move(oa), ia1);
         Scoped2Inner s2i(::boost::move(oa), ia1, ia2);
      }
      //Copy from rebound outer
      {
         Scoped0Inner s0i(roa);
         Scoped1Inner s1i(roa, ia1);
         Scoped2Inner s2i(roa, ia1, ia2);
      }
      //Move from rebound outer
      {
         Scoped0Inner s0i(::boost::move(roa));
         Scoped1Inner s1i(::boost::move(roa), ia1);
         Scoped2Inner s2i(::boost::move(roa), ia1, ia2);
      }
      //Copy from rebound scoped
      {
         Scoped0Inner s0i(rs0i);
         Scoped1Inner s1i(rs1i);
         Scoped2Inner s2i(rs2i);
      }
      //Move from rebound scoped
      {
         Scoped0Inner s0i(::boost::move(rs0i));
         Scoped1Inner s1i(::boost::move(rs1i));
         Scoped2Inner s2i(::boost::move(rs2i));
      }
   }

   {
      vector<int, scoped_allocator_adaptor< test_allocator<int, 0> > > dummy; 
      dummy.push_back(0);
   }

   //destroy()
   {
      {
         Scoped0Inner s0i;
         mark_on_destructor mod;
         s0i.destroy(&mod);
         if(!mod.destroyed){
            return 1;
         }
      }

      {
         Scoped1Inner s1i;
         mark_on_destructor mod;
         s1i.destroy(&mod);
         if(!mod.destroyed){
            return 1;
         }
      }
      {
         Scoped2Inner s2i;
         mark_on_destructor mod;
         s2i.destroy(&mod);
         if(!mod.destroyed){
            return 1;
         }
      }
   }

   //construct
   {

      BOOST_STATIC_ASSERT(( !boost::container::uses_allocator
                              < ::mark_on_scoped_allocation<NotUsesAllocator, 0>
                              , test_allocator<float, 0>
                              >::value ));
      BOOST_STATIC_ASSERT((  boost::container::uses_allocator
                              < ::mark_on_scoped_allocation<ConstructiblePrefix, 0>
                              , test_allocator<float, 0>
                              >::value ));
      BOOST_STATIC_ASSERT((  boost::container::uses_allocator
                              < ::mark_on_scoped_allocation<ConstructibleSuffix, 0>
                              , test_allocator<float, 0>
                              >::value ));
      BOOST_STATIC_ASSERT(( boost::container::constructible_with_allocator_prefix
                          < ::mark_on_scoped_allocation<ConstructiblePrefix, 0> >::value ));
      BOOST_STATIC_ASSERT(( boost::container::constructible_with_allocator_suffix
                          < ::mark_on_scoped_allocation<ConstructibleSuffix, 0> >::value ));

      ////////////////////////////////////////////////////////////
      //First check scoped allocator with just OuterAlloc.
      //In this case OuterAlloc (test_allocator with tag 0) should be
      //used to construct types.
      ////////////////////////////////////////////////////////////
      {
         Scoped0Inner s0i;
         //Check construction with 0 user arguments
         {
            typedef ::mark_on_scoped_allocation<NotUsesAllocator, 0> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            s0i.construct(&dummy);
            if(dummy.construction_type != NotUsesAllocator ||
               dummy.value != 0){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructibleSuffix, 0> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            s0i.construct(&dummy);
            if(dummy.construction_type != ConstructibleSuffix ||
               dummy.value != 0){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructiblePrefix, 0> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            s0i.construct(&dummy);
            if(dummy.construction_type != ConstructiblePrefix ||
               dummy.value != 0){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }

         //Check construction with 1 user arguments
         {
            typedef ::mark_on_scoped_allocation<NotUsesAllocator, 0> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            s0i.construct(&dummy, 1);
            if(dummy.construction_type != NotUsesAllocator ||
               dummy.value != 1){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructibleSuffix, 0> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            s0i.construct(&dummy, 2);
            if(dummy.construction_type != ConstructibleSuffix ||
               dummy.value != 2){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructiblePrefix, 0> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            s0i.construct(&dummy, 3);
            if(dummy.construction_type != ConstructiblePrefix ||
               dummy.value != 3){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
      }
      ////////////////////////////////////////////////////////////
      //Then check scoped allocator with OuterAlloc and InnerAlloc.
      //In this case InnerAlloc (test_allocator with tag 1) should be
      //used to construct types.
      ////////////////////////////////////////////////////////////
      {
         Scoped1Inner s1i;
         //Check construction with 0 user arguments
         {
            typedef ::mark_on_scoped_allocation<NotUsesAllocator, 1> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            s1i.construct(&dummy);
            if(dummy.construction_type != NotUsesAllocator ||
               dummy.value != 0){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructibleSuffix, 1> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            s1i.construct(&dummy);
            if(dummy.construction_type != ConstructibleSuffix ||
               dummy.value != 0){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructiblePrefix, 1> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            s1i.construct(&dummy);
            if(dummy.construction_type != ConstructiblePrefix ||
               dummy.value != 0){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }

         //Check construction with 1 user arguments
         {
            typedef ::mark_on_scoped_allocation<NotUsesAllocator, 1> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            s1i.construct(&dummy, 1);
            if(dummy.construction_type != NotUsesAllocator ||
               dummy.value != 1){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructibleSuffix, 1> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            s1i.construct(&dummy, 2);
            if(dummy.construction_type != ConstructibleSuffix ||
               dummy.value != 2){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructiblePrefix, 1> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            s1i.construct(&dummy, 3);
            if(dummy.construction_type != ConstructiblePrefix ||
               dummy.value != 3){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
      }

      //////////////////////////////////////////////////////////////////////////////////
      //Now test recursive OuterAllocator types (OuterAllocator is a scoped_allocator)
      //////////////////////////////////////////////////////////////////////////////////

      ////////////////////////////////////////////////////////////
      //First check scoped allocator with just OuterAlloc.
      //In this case OuterAlloc (test_allocator with tag 0) should be
      //used to construct types.
      ////////////////////////////////////////////////////////////
      {
         //Check outer_allocator_type is scoped
         BOOST_STATIC_ASSERT(( boost::container::is_scoped_allocator
            <ScopedScoped0Inner::outer_allocator_type>::value ));
         BOOST_STATIC_ASSERT(( ::boost::container::container_detail::is_same
            < boost::container::outermost_allocator<ScopedScoped0Inner>::type
            , Outer10IdAlloc
            >::value ));
         BOOST_STATIC_ASSERT(( ::boost::container::container_detail::is_same
            < ScopedScoped0Inner::outer_allocator_type
            , scoped_allocator_adaptor<Outer10IdAlloc>
            >::value ));
         BOOST_STATIC_ASSERT(( ::boost::container::container_detail::is_same
            < scoped_allocator_adaptor<Outer10IdAlloc>::outer_allocator_type
            , Outer10IdAlloc
            >::value ));
         ScopedScoped0Inner ssro0i;
         Outer10IdAlloc & val = boost::container::outermost_allocator<ScopedScoped0Inner>::get(ssro0i);
         (void)val;
         //Check construction with 0 user arguments
         {
            typedef ::mark_on_scoped_allocation<NotUsesAllocator, 10> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            ssro0i.construct(&dummy);
            if(dummy.construction_type != NotUsesAllocator ||
               dummy.value != 0){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructibleSuffix, 10> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            ssro0i.construct(&dummy);
            if(dummy.construction_type != ConstructibleSuffix ||
               dummy.value != 0){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructiblePrefix, 10> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            ssro0i.construct(&dummy);
            if(dummy.construction_type != ConstructiblePrefix ||
               dummy.value != 0){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }

         //Check construction with 1 user arguments
         {
            typedef ::mark_on_scoped_allocation<NotUsesAllocator, 10> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            ssro0i.construct(&dummy, 1);
            if(dummy.construction_type != NotUsesAllocator ||
               dummy.value != 1){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructibleSuffix, 10> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            ssro0i.construct(&dummy, 2);
            if(dummy.construction_type != ConstructibleSuffix ||
               dummy.value != 2){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructiblePrefix, 10> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            ssro0i.construct(&dummy, 3);
            if(dummy.construction_type != ConstructiblePrefix ||
               dummy.value != 3){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
      }
      ////////////////////////////////////////////////////////////
      //Then check scoped allocator with OuterAlloc and InnerAlloc.
      //In this case inner_allocator_type is not convertible to
      //::mark_on_scoped_allocation<XXX, 10> so uses_allocator
      //should be false on all tests.
      ////////////////////////////////////////////////////////////
      {
         //Check outer_allocator_type is scoped
         BOOST_STATIC_ASSERT(( boost::container::is_scoped_allocator
            <ScopedScoped1Inner::outer_allocator_type>::value ));
         BOOST_STATIC_ASSERT(( ::boost::container::container_detail::is_same
            < boost::container::outermost_allocator<ScopedScoped1Inner>::type
            , Outer10IdAlloc
            >::value ));
         BOOST_STATIC_ASSERT(( ::boost::container::container_detail::is_same
            < ScopedScoped1Inner::outer_allocator_type
            , scoped_allocator_adaptor<Outer10IdAlloc, Inner11IdAlloc1>
            >::value ));
         BOOST_STATIC_ASSERT(( ::boost::container::container_detail::is_same
            < scoped_allocator_adaptor<Outer10IdAlloc, Inner11IdAlloc1>::outer_allocator_type
            , Outer10IdAlloc
            >::value ));
         BOOST_STATIC_ASSERT(( !
            ::boost::container::uses_allocator
               < ::mark_on_scoped_allocation<ConstructibleSuffix, 10>
               , ScopedScoped1Inner::inner_allocator_type::outer_allocator_type
               >::value ));
         ScopedScoped1Inner ssro1i;
         Outer10IdAlloc & val = boost::container::outermost_allocator<ScopedScoped1Inner>::get(ssro1i);
         (void)val;

         //Check construction with 0 user arguments
         {
            typedef ::mark_on_scoped_allocation<NotUsesAllocator, 10> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            ssro1i.construct(&dummy);
            if(dummy.construction_type != NotUsesAllocator ||
               dummy.value != 0){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructibleSuffix, 10> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            ssro1i.construct(&dummy);
            if(dummy.construction_type != NotUsesAllocator ||
               dummy.value != 0){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructiblePrefix, 10> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            ssro1i.construct(&dummy);
            if(dummy.construction_type != NotUsesAllocator ||
               dummy.value != 0){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }

         //Check construction with 1 user arguments
         {
            typedef ::mark_on_scoped_allocation<NotUsesAllocator, 10> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            ssro1i.construct(&dummy, 1);
            if(dummy.construction_type != NotUsesAllocator ||
               dummy.value != 1){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructibleSuffix, 10> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            ssro1i.construct(&dummy, 2);
            if(dummy.construction_type != NotUsesAllocator ||
               dummy.value != 2){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructiblePrefix, 10> MarkType;
            MarkType dummy;
            dummy.~MarkType();
            ssro1i.construct(&dummy, 3);
            if(dummy.construction_type != NotUsesAllocator ||
               dummy.value != 3){
               dummy.~MarkType();
               return 1;
            }
            dummy.~MarkType();
         }
      }

      ////////////////////////////////////////////////////////////
      //Now check propagation to pair
      ////////////////////////////////////////////////////////////
      //First check scoped allocator with just OuterAlloc.
      //In this case OuterAlloc (test_allocator with tag 0) should be
      //used to construct types.
      ////////////////////////////////////////////////////////////
      {
         using boost::container::container_detail::pair;
         typedef test_allocator< pair< tagged_integer<0>
                               , tagged_integer<0> >, 0> OuterPairAlloc;
         //
         typedef scoped_allocator_adaptor < OuterPairAlloc  >  ScopedPair0Inner;

         ScopedPair0Inner s0i;
         //Check construction with 0 user arguments
         {
            typedef ::mark_on_scoped_allocation<NotUsesAllocator, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy;
            dummy.~MarkTypePair();
            s0i.construct(&dummy);
            if(dummy.first.construction_type  != NotUsesAllocator ||
               dummy.second.construction_type != NotUsesAllocator ||
               dummy.first.value  != 0 ||
               dummy.second.value != 0 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructibleSuffix, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy;
            dummy.~MarkTypePair();
            s0i.construct(&dummy);
            if(dummy.first.construction_type  != ConstructibleSuffix ||
               dummy.second.construction_type != ConstructibleSuffix ||
               dummy.first.value  != 0 ||
               dummy.second.value != 0 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructiblePrefix, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy;
            dummy.~MarkTypePair();
            s0i.construct(&dummy);
            if(dummy.first.construction_type  != ConstructiblePrefix ||
               dummy.second.construction_type != ConstructiblePrefix ||
               dummy.first.value  != 0  ||
               dummy.second.value != 0 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }

         //Check construction with 1 user arguments for each pair
         {
            typedef ::mark_on_scoped_allocation<NotUsesAllocator, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy;
            dummy.~MarkTypePair();
            s0i.construct(&dummy, 1, 1);
            if(dummy.first.construction_type  != NotUsesAllocator ||
               dummy.second.construction_type != NotUsesAllocator ||
               dummy.first.value  != 1 ||
               dummy.second.value != 1 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructibleSuffix, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy;
            dummy.~MarkTypePair();
            s0i.construct(&dummy, 1, 1);
            if(dummy.first.construction_type  != ConstructibleSuffix ||
               dummy.second.construction_type != ConstructibleSuffix ||
               dummy.first.value  != 1 ||
               dummy.second.value != 1 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructiblePrefix, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy;
            dummy.~MarkTypePair();
            s0i.construct(&dummy, 2, 2);
            if(dummy.first.construction_type  != ConstructiblePrefix ||
               dummy.second.construction_type != ConstructiblePrefix ||
               dummy.first.value  != 2 ||
               dummy.second.value != 2 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
         //Check construction with pair copy construction
         {
            typedef ::mark_on_scoped_allocation<NotUsesAllocator, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy, dummy2;
            dummy.~MarkTypePair();
            s0i.construct(&dummy, dummy2);
            if(dummy.first.construction_type  != NotUsesAllocator ||
               dummy.second.construction_type != NotUsesAllocator ||
               dummy.first.value  != 0 ||
               dummy.second.value != 0 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructibleSuffix, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy, dummy2(1, 1);
            dummy.~MarkTypePair();
            s0i.construct(&dummy, dummy2);
            if(dummy.first.construction_type  != ConstructibleSuffix ||
               dummy.second.construction_type != ConstructibleSuffix ||
               dummy.first.value  != 1 ||
               dummy.second.value != 1 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructiblePrefix, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy, dummy2(2, 2);
            dummy.~MarkTypePair();
            s0i.construct(&dummy, dummy2);
            if(dummy.first.construction_type  != ConstructiblePrefix ||
               dummy.second.construction_type != ConstructiblePrefix ||
               dummy.first.value  != 2 ||
               dummy.second.value != 2 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
         //Check construction with pair move construction
         {
            typedef ::mark_on_scoped_allocation<NotUsesAllocator, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy, dummy2(3, 3);
            dummy2.first.construction_type = dummy2.second.construction_type = ConstructibleSuffix;
            dummy.~MarkTypePair();
            s0i.construct(&dummy, ::boost::move(dummy2));
            if(dummy.first.construction_type  != NotUsesAllocator ||
               dummy.second.construction_type != NotUsesAllocator ||
               dummy.first.value  != 3 ||
               dummy.second.value != 3 ||
               dummy2.first.construction_type  != NotUsesAllocator ||
               dummy2.second.construction_type != NotUsesAllocator ||
               dummy2.first.value  != 0 ||
               dummy2.second.value != 0 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructibleSuffix, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy, dummy2(1, 1);
            dummy.~MarkTypePair();
            s0i.construct(&dummy, ::boost::move(dummy2));
            if(dummy.first.construction_type  != ConstructibleSuffix ||
               dummy.second.construction_type != ConstructibleSuffix ||
               dummy.first.value  != 1 ||
               dummy.second.value != 1 ||
               dummy2.first.construction_type  != ConstructibleSuffix ||
               dummy2.second.construction_type != ConstructibleSuffix ||
               dummy2.first.value  != 0 ||
               dummy2.second.value != 0 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructiblePrefix, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy, dummy2(2, 2);
            dummy.~MarkTypePair();
            s0i.construct(&dummy, ::boost::move(dummy2));
            if(dummy.first.construction_type  != ConstructiblePrefix ||
               dummy.second.construction_type != ConstructiblePrefix ||
               dummy.first.value  != 2 ||
               dummy.second.value != 2 ||
               dummy2.first.construction_type  != ConstructiblePrefix ||
               dummy2.second.construction_type != ConstructiblePrefix ||
               dummy2.first.value  != 0 ||
               dummy2.second.value != 0 ){
               dummy2.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
         //Check construction with related pair copy construction
         {
            typedef ::mark_on_scoped_allocation<NotUsesAllocator, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy;
            pair<int, int> dummy2;
            dummy.~MarkTypePair();
            s0i.construct(&dummy, dummy2);
            if(dummy.first.construction_type  != NotUsesAllocator ||
               dummy.second.construction_type != NotUsesAllocator ||
               dummy.first.value  != 0 ||
               dummy.second.value != 0 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructibleSuffix, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy;
            pair<int, int> dummy2(1, 1);
            dummy.~MarkTypePair();
            s0i.construct(&dummy, dummy2);
            if(dummy.first.construction_type  != ConstructibleSuffix ||
               dummy.second.construction_type != ConstructibleSuffix ||
               dummy.first.value  != 1 ||
               dummy.second.value != 1 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructiblePrefix, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy;
            pair<int, int> dummy2(2, 2);
            dummy.~MarkTypePair();
            s0i.construct(&dummy, dummy2);
            if(dummy.first.construction_type  != ConstructiblePrefix ||
               dummy.second.construction_type != ConstructiblePrefix ||
               dummy.first.value  != 2 ||
               dummy.second.value != 2 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
         //Check construction with related pair move construction
         {
            typedef ::mark_on_scoped_allocation<NotUsesAllocator, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy;
            pair<int, int> dummy2(3, 3);
            dummy.~MarkTypePair();
            s0i.construct(&dummy, ::boost::move(dummy2));
            if(dummy.first.construction_type  != NotUsesAllocator ||
               dummy.second.construction_type != NotUsesAllocator ||
               dummy.first.value  != 3 ||
               dummy.second.value != 3 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructibleSuffix, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy;
            pair<int, int> dummy2(1, 1);
            dummy.~MarkTypePair();
            s0i.construct(&dummy, ::boost::move(dummy2));
            if(dummy.first.construction_type  != ConstructibleSuffix ||
               dummy.second.construction_type != ConstructibleSuffix ||
               dummy.first.value  != 1 ||
               dummy.second.value != 1 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
         {
            typedef ::mark_on_scoped_allocation<ConstructiblePrefix, 0> MarkType;
            typedef pair<MarkType, MarkType> MarkTypePair;
            MarkTypePair dummy;
            pair<int, int> dummy2(2, 2);
            dummy.~MarkTypePair();
            s0i.construct(&dummy, ::boost::move(dummy2));
            if(dummy.first.construction_type  != ConstructiblePrefix ||
               dummy.second.construction_type != ConstructiblePrefix ||
               dummy.first.value  != 2 ||
               dummy.second.value != 2 ){
               dummy.~MarkTypePair();
               return 1;
            }
            dummy.~MarkTypePair();
         }
      }
   }

   return 0;
}
#include <boost/container/detail/config_end.hpp>

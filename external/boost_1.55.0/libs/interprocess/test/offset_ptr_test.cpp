//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2007-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/static_assert.hpp>

using namespace boost::interprocess;

class Base
{};

class Derived
   : public Base
{};

class VirtualDerived
   : public virtual Base
{};

bool test_types_and_conversions()
{
   typedef offset_ptr<int>                pint_t;
   typedef offset_ptr<const int>          pcint_t;
   typedef offset_ptr<volatile int>       pvint_t;
   typedef offset_ptr<const volatile int> pcvint_t;

   BOOST_STATIC_ASSERT((ipcdetail::is_same<pint_t::element_type, int>::value));
   BOOST_STATIC_ASSERT((ipcdetail::is_same<pcint_t::element_type, const int>::value));
   BOOST_STATIC_ASSERT((ipcdetail::is_same<pvint_t::element_type, volatile int>::value));
   BOOST_STATIC_ASSERT((ipcdetail::is_same<pcvint_t::element_type, const volatile int>::value));

   BOOST_STATIC_ASSERT((ipcdetail::is_same<pint_t::value_type,   int>::value));
   BOOST_STATIC_ASSERT((ipcdetail::is_same<pcint_t::value_type,  int>::value));
   BOOST_STATIC_ASSERT((ipcdetail::is_same<pvint_t::value_type,  int>::value));
   BOOST_STATIC_ASSERT((ipcdetail::is_same<pcvint_t::value_type, int>::value));
   int dummy_int = 9;

   {  pint_t pint(&dummy_int);   pcint_t  pcint(pint);
      if(pcint.get()  != &dummy_int)   return false;  }
   {  pint_t pint(&dummy_int);   pvint_t  pvint(pint);
      if(pvint.get()  != &dummy_int)   return false;  }
   {  pint_t pint(&dummy_int);   pcvint_t  pcvint(pint);
      if(pcvint.get()  != &dummy_int)  return false;  }
   {  pcint_t pcint(&dummy_int); pcvint_t  pcvint(pcint);
         if(pcvint.get()  != &dummy_int)  return false;  }
   {  pvint_t pvint(&dummy_int); pcvint_t  pcvint(pvint);
      if(pcvint.get()  != &dummy_int)  return false;  }

   pint_t   pint(0);
   pcint_t  pcint(0);
   pvint_t  pvint(0);
   pcvint_t pcvint(0);

   pint     = &dummy_int;
   pcint    = &dummy_int;
   pvint    = &dummy_int;
   pcvint   = &dummy_int;

   {   pcint  = pint;   if(pcint.get() != &dummy_int)   return false;  }
   {   pvint  = pint;   if(pvint.get() != &dummy_int)   return false;  }
   {   pcvint = pint;   if(pcvint.get() != &dummy_int)  return false;  }
   {   pcvint = pcint;  if(pcvint.get() != &dummy_int)  return false;  }
   {   pcvint = pvint;  if(pcvint.get() != &dummy_int)  return false;  }

   if(!pint)
      return false;

   pint = 0;
   if(pint)
      return false;

   if(pint != 0)
      return false;

   if(0 != pint)
      return false;

   pint = &dummy_int;
   if(0 == pint)
      return false;

   if(pint == 0)
      return false;

   pcint = &dummy_int;

   if( (pcint - pint) != 0)
      return false;

   if( (pint - pcint) != 0)
      return false;

   return true;
}

template<class BasePtr, class DerivedPtr>
bool test_base_derived_impl()
{
   typename DerivedPtr::element_type d;
   DerivedPtr pderi(&d);

   BasePtr pbase(pderi);
   pbase = pderi;
   if(pbase != pderi)
      return false;
   if(!(pbase == pderi))
      return false;
   if((pbase - pderi) != 0)
      return false;
   if(pbase < pderi)
      return false;
   if(pbase > pderi)
      return false;
   if(!(pbase <= pderi))
      return false;
   if(!(pbase >= pderi))
      return false;

   return true;
}

bool test_base_derived()
{
   typedef offset_ptr<Base>               pbase_t;
   typedef offset_ptr<const Base>         pcbas_t;
   typedef offset_ptr<Derived>            pderi_t;
   typedef offset_ptr<VirtualDerived>     pvder_t;

   if(!test_base_derived_impl<pbase_t, pderi_t>())
      return false;
   if(!test_base_derived_impl<pbase_t, pvder_t>())
      return false;
   if(!test_base_derived_impl<pcbas_t, pderi_t>())
      return false;
   if(!test_base_derived_impl<pcbas_t, pvder_t>())
      return false;

   return true;
}

bool test_arithmetic()
{
   typedef offset_ptr<int> pint_t;
   const int NumValues = 5;
   int values[NumValues];

   //Initialize p
   pint_t p = values;
   if(p.get() != values)
      return false;

   //Initialize p + NumValues
   pint_t pe = &values[NumValues];
   if(pe == p)
      return false;
   if(pe.get() != &values[NumValues])
      return false;

   //ptr - ptr
   if((pe - p) != NumValues)
      return false;
   //ptr - integer
   if((pe - NumValues) != p)
      return false;
   //ptr + integer
   if((p + NumValues) != pe)
      return false;
   //integer + ptr
   if((NumValues + p) != pe)
      return false;
   //indexing
   if(pint_t(&p[NumValues])   != pe)
      return false;
   if(pint_t(&pe[-NumValues]) != p)
      return false;

   //ptr -= integer
   pint_t p0 = pe;
   p0-= NumValues;
   if(p != p0)
      return false;
   //ptr += integer
   pint_t penew = p0;
   penew += NumValues;
   if(penew != pe)
      return false;

   //++ptr
   penew = p0;
   for(int j = 0; j != NumValues; ++j, ++penew);
   if(penew != pe)
      return false;
   //--ptr
   p0 = pe;
   for(int j = 0; j != NumValues; ++j, --p0);
   if(p != p0)
      return false;
   //ptr++
   penew = p0;
   for(int j = 0; j != NumValues; ++j){
      pint_t p_new_copy = penew;
      if(p_new_copy != penew++)
         return false;
   }
   //ptr--
   p0 = pe;
   for(int j = 0; j != NumValues; ++j){
      pint_t p0_copy = p0;
      if(p0_copy != p0--)
         return false;
   }

   return true;
}

bool test_comparison()
{
   typedef offset_ptr<int> pint_t;
   const int NumValues = 5;
   int values[NumValues];

   //Initialize p
   pint_t p = values;
   if(p.get() != values)
      return false;

   //Initialize p + NumValues
   pint_t pe = &values[NumValues];
   if(pe == p)
      return false;
   if(pe.get() != &values[NumValues])
      return false;

   //operators
   if(p == pe)
      return false;
   if(p != p)
      return false;
   if(!(p < pe))
      return false;
   if(!(p <= pe))
      return false;
   if(!(pe > p))
      return false;
   if(!(pe >= p))
      return false;

   return true;
}

bool test_pointer_traits()
{
   typedef offset_ptr<int> OInt;
   typedef boost::intrusive::pointer_traits< OInt > PTOInt;
   BOOST_STATIC_ASSERT((ipcdetail::is_same<PTOInt::element_type, int>::value));
   BOOST_STATIC_ASSERT((ipcdetail::is_same<PTOInt::pointer, OInt >::value));
   BOOST_STATIC_ASSERT((ipcdetail::is_same<PTOInt::difference_type, OInt::difference_type >::value));
   BOOST_STATIC_ASSERT((ipcdetail::is_same<PTOInt::rebind_pointer<double>::type, offset_ptr<double> >::value));
   int dummy;
   OInt oi(&dummy);
   if(boost::intrusive::pointer_traits<OInt>::pointer_to(dummy) != oi){
      return false;
   }
   return true;
}

int main()
{
   if(!test_types_and_conversions())
      return 1;
   if(!test_base_derived())
      return 1;
   if(!test_arithmetic())
      return 1;
   if(!test_comparison())
      return 1;
   if(!test_pointer_traits())
      return 1;
   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>

/*
//Offset ptr benchmark
#include <vector>
#include <iostream>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/timer.hpp>
#include <cstddef>

template<class InIt,
	class Ty> inline
	Ty accumulate2(InIt First, InIt Last, Ty Val)
	{	// return sum of Val and all in [First, Last)
	for (; First != Last; ++First) //First = First + 1)
		Val = Val + *First;
	return (Val);
	}

template <typename Vector>
void time_test(const Vector& vec, std::size_t iterations, const char* label) {
  // assert(!vec.empty())
  boost::timer t;
  typename Vector::const_iterator first = vec.begin();
  typename Vector::value_type result(0);
  while (iterations != 0) {
    result = accumulate2(first, first + vec.size(), result);
    --iterations;
  }
  std::cout << label << t.elapsed() << " " << result << std::endl;
}

int main()
{
   using namespace boost::interprocess;
   typedef allocator<double, managed_shared_memory::segment_manager> alloc_t;

   std::size_t n = 0x1 << 26; 
   std::size_t file_size = n * sizeof(double) + 1000000;

   {  
      shared_memory_object::remove("MyMappedFile");
      managed_shared_memory segment(open_or_create, "MyMappedFile", file_size);
      shared_memory_object::remove("MyMappedFile");
      alloc_t alloc_inst(segment.get_segment_manager());
      vector<double, alloc_t>  v0(n, double(42.42), alloc_inst);
      time_test(v0, 10, "iterator   shared:     ");
   }
   {
      std::vector<double>      v1(n, double(42.42));
      time_test(v1, 10, "iterator   non-shared: ");
   }
  return 0;
}

*/

//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef TEST_STD_HEADERS
#include <type_traits>
#else
#include <boost/tr1/type_traits.hpp>
#endif

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>

template <class T>
void check_value_trait(const T&)
{
   T t;
   (void)t;
   typedef typename T::type type;
   typedef typename T::value_type value_type;

   BOOST_STATIC_ASSERT((::boost::is_base_and_derived<
      std::tr1::integral_constant<value_type, T::value>, T>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<
      std::tr1::integral_constant<value_type, T::value>, type>::value));
}

template <class T>
void check_transform_trait(const T* p)
{
   typedef typename T::type type;
}

struct UDT{};
union union_type{ char c; int i; };
enum enum_type{ one, two };

#define check_unary_trait(trait)\
   check_value_trait(std::tr1::trait<void>());\
   check_value_trait(std::tr1::trait<int>());\
   check_value_trait(std::tr1::trait<float>());\
   check_value_trait(std::tr1::trait<UDT>());\
   /*check_value_trait(std::tr1::trait<union_type>());*/\
   check_value_trait(std::tr1::trait<enum_type>());\
   check_value_trait(std::tr1::trait<void *>());\
   check_value_trait(std::tr1::trait<UDT&>());\
   check_value_trait(std::tr1::trait<int (void)>());\
   check_value_trait(std::tr1::trait<int (&)(void)>());\
   check_value_trait(std::tr1::trait<int (*)(void)>());\
   check_value_trait(std::tr1::trait<int (UDT::*)(void)>());\
   check_value_trait(std::tr1::trait<int (UDT::*)>());\
   check_value_trait(std::tr1::trait<int[4]>());\
   check_value_trait(std::tr1::trait<int[]>());

#define check_binary_trait(trait)\
   check_value_trait(std::tr1::trait<void, int>());\
   check_value_trait(std::tr1::trait<int, double>());\
   check_value_trait(std::tr1::trait<float, float>());\
   check_value_trait(std::tr1::trait<UDT, UDT&>());\
   /*check_value_trait(std::tr1::trait<union_type>());*/\
   check_value_trait(std::tr1::trait<enum_type, enum_type>());\
   check_value_trait(std::tr1::trait<void *, const void*>());\
   check_value_trait(std::tr1::trait<UDT&, int>());\
   check_value_trait(std::tr1::trait<int (void), int (void)>());\
   check_value_trait(std::tr1::trait<int (&)(void), int (void)>());\
   check_value_trait(std::tr1::trait<int (*)(void), int (void)>());\
   check_value_trait(std::tr1::trait<int (UDT::*)(void), int (void)>());\
   check_value_trait(std::tr1::trait<int (UDT::*), int (UDT::*)>());\
   check_value_trait(std::tr1::trait<int[4], int[3]>());\
   check_value_trait(std::tr1::trait<int[4], int[4]>());

#define check_transform(trait)\
   check_transform_trait((std::tr1::trait<void>*)(0));\
   check_transform_trait((std::tr1::trait<int>*)(0));\
   check_transform_trait((std::tr1::trait<float>*)(0));\
   check_transform_trait((std::tr1::trait<UDT>*)(0));\
   check_transform_trait((std::tr1::trait<enum_type>*)(0));\
   check_transform_trait((std::tr1::trait<void *>*)(0));\
   check_transform_trait((std::tr1::trait<UDT&>*)(0));\
   check_transform_trait((std::tr1::trait<int (void)>*)(0));\
   check_transform_trait((std::tr1::trait<int (&)(void)>*)(0));\
   check_transform_trait((std::tr1::trait<int (*)(void)>*)(0));\
   check_transform_trait((std::tr1::trait<int (UDT::*)(void)>*)(0));\
   check_transform_trait((std::tr1::trait<int (UDT::*)>*)(0));\
   check_transform_trait((std::tr1::trait<int[4]>*)(0));

int main()
{
   check_unary_trait(is_void);
   check_unary_trait(is_integral);
   check_unary_trait(is_floating_point);
   check_unary_trait(is_array);
   check_unary_trait(is_pointer);
   check_unary_trait(is_reference);
   check_unary_trait(is_member_object_pointer);
   check_unary_trait(is_member_function_pointer);
   check_unary_trait(is_enum);
   check_unary_trait(is_union);
   check_unary_trait(is_class);
   check_unary_trait(is_function);
   check_unary_trait(is_arithmetic);
   check_unary_trait(is_fundamental);
   check_unary_trait(is_object);
   check_unary_trait(is_scalar);
   check_unary_trait(is_compound);
   check_unary_trait(is_member_pointer);
   check_unary_trait(is_const);
   check_unary_trait(is_volatile);
   check_unary_trait(is_pod);
   check_unary_trait(is_empty);
   check_unary_trait(is_polymorphic);
   check_unary_trait(is_abstract);
   check_unary_trait(has_trivial_constructor);
   check_unary_trait(has_trivial_copy);
   check_unary_trait(has_trivial_assign);
   check_unary_trait(has_trivial_destructor);
   check_unary_trait(has_nothrow_constructor);
   check_unary_trait(has_nothrow_copy);
   check_unary_trait(has_nothrow_assign);
   check_unary_trait(has_virtual_destructor);
   check_unary_trait(is_signed);
   check_unary_trait(is_unsigned);

   check_value_trait(std::tr1::alignment_of<int>());
   check_value_trait(std::tr1::alignment_of<UDT>());
   check_value_trait(std::tr1::alignment_of<int[4]>());

   check_unary_trait(rank);
   check_unary_trait(extent);
   
   check_binary_trait(is_same);
   check_binary_trait(is_base_of);
   check_binary_trait(is_convertible);

   check_transform(remove_const);
   check_transform(remove_volatile);
   check_transform(remove_cv);
   check_transform(add_const);
   check_transform(add_volatile);
   check_transform(add_cv);
   check_transform(remove_reference);
   check_transform(add_reference);
   check_transform(remove_extent);
   check_transform(remove_all_extents);
   check_transform(remove_pointer);
   check_transform(add_pointer);
   check_transform_trait((::std::tr1::aligned_storage<1,1>*)(0));
   check_transform_trait((::std::tr1::aligned_storage<4,1>*)(0));
   check_transform_trait((::std::tr1::aligned_storage<2,2>*)(0));
   check_transform_trait((::std::tr1::aligned_storage<4,2>*)(0));
   check_transform_trait((::std::tr1::aligned_storage<4,4>*)(0));
   check_transform_trait((::std::tr1::aligned_storage<4,4>*)(0));
   check_transform_trait((::std::tr1::aligned_storage<8,8>*)(0));

   return 0;
}


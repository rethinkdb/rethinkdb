//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef TEST_STD_HEADERS
#include <functional>
#else
#include <boost/tr1/functional.hpp>
#endif

#include <string>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include "verify_return.hpp"

template <class T>
void check_hash(T t)
{
   typedef std::tr1::hash<T> hash_type;
   typedef typename hash_type::argument_type arg_type;
   typedef typename hash_type::result_type result_type;
   BOOST_STATIC_ASSERT((::boost::is_same<result_type, std::size_t>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<arg_type, T>::value));
   BOOST_STATIC_ASSERT((::boost::is_base_of<std::unary_function<arg_type, result_type>, hash_type>::value));

   hash_type h;
   const hash_type& ch = h;

   verify_return_type(ch(t), std::size_t(0));
}

class UDT
{
   int m_value;
public:
   UDT(int v) : m_value(v) {}
   int value()const { return m_value; }
};

namespace std{ namespace tr1{

template<>
struct hash<UDT> : public std::unary_function<UDT, std::size_t>
{
   typedef UDT argument_type;
   typedef std::size_t result_type;
   std::size_t operator()(const UDT& u)const
   {
      std::tr1::hash<int> h;
      return h(u.value());
   }
};

}}

int main()
{
   check_hash(0);
   check_hash(0u);
   check_hash(0L);
   check_hash(0uL);
   check_hash(static_cast<short>(0));
   check_hash(static_cast<unsigned short>(0));
   check_hash(static_cast<signed char>(0));
   check_hash(static_cast<unsigned char>(0));
   check_hash(static_cast<char>(0));
   check_hash(static_cast<wchar_t>(0));
   check_hash(static_cast<bool>(0));
   check_hash(static_cast<float>(0));
   check_hash(static_cast<double>(0));
   check_hash(static_cast<long double>(0));
   check_hash(std::string(""));
   check_hash(std::wstring(L""));
   check_hash(static_cast<UDT*>(0));
   check_hash(static_cast<const UDT*>(0));
   check_hash(static_cast<volatile UDT*>(0));
   check_hash(static_cast<const volatile UDT*>(0));
   check_hash(UDT(1));
   return 0;
}


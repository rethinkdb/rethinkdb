///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#ifndef BOOST_MP_TABLE_TYPE

#include <libs/math/test/table_type.hpp>
#include <boost/multiprecision/number.hpp>

struct string_table_entry
{
private:
   const char* m_data;
public:
   string_table_entry(const char* p) : m_data(p) {}

   template <class T>
   operator T () const
   {
      return static_cast<T>(m_data);
   }
};

inline std::ostream& operator << (std::ostream& os, string_table_entry const & what)
{
   return os << static_cast<const char*>(what);
}

template <class Backend, boost::multiprecision::expression_template_option ExpressionTemplates>
struct table_type<boost::multiprecision::number<Backend, ExpressionTemplates> >
{
   typedef string_table_entry type;
};

#define SC_(x) string_table_entry(BOOST_STRINGIZE(x))

#endif


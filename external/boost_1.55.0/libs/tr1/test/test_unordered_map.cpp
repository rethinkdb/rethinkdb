//  (C) Copyright John Maddock 2008.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// These are the headers included by the Boost.TR1 implementation,
// by including these directly we probe for problems with cyclic
// dependencies when the TR1 headers are in the include search path.

#ifdef TEST_STD_HEADERS
#include <unordered_map>
#else
#include <boost/tr1/unordered_map.hpp>
#endif

#include "unordered_concepts.hpp"
#include "boost/concept_archetype.hpp"


int main()
{
   typedef boost::copy_constructible_archetype<boost::assignable_archetype<> > value_type;
   typedef boost::default_constructible_unary_function_archetype<value_type, std::size_t> func_type;
   typedef boost::default_constructible_binary_predicate_archetype<value_type, value_type> pred_type;
   typedef std::tr1::unordered_map<value_type, value_type, func_type, pred_type> hash_map_type;
   typedef std::tr1::unordered_multimap<value_type, value_type, func_type, pred_type> hash_multimap_type;

   boost::function_requires<boost::UniqueUnorderedContainer<hash_map_type> >();
   boost::function_requires<boost::MultiUnorderedContainer<hash_multimap_type> >();

   do{
      // unordered_map specific functions:
      typedef hash_map_type::key_type key_type;
      typedef hash_map_type::hasher hasher;
      typedef hash_map_type::key_equal key_equal;
      typedef hash_map_type::local_iterator local_iterator;
      typedef hash_map_type::const_local_iterator const_local_iterator;
      typedef hash_map_type::value_type value_type;
      typedef hash_map_type::iterator iterator;
      typedef hash_map_type::const_iterator const_iterator;
      typedef hash_map_type::size_type size_type;
      typedef hash_map_type::allocator_type allocator_type;
      typedef boost::input_iterator_archetype<value_type> input_iterator;
      input_iterator i = boost::static_object<input_iterator>::get();
      input_iterator j = i;

      size_type n = 1;
      const hasher& hf = boost::static_object<hasher>::get();
      const key_equal& eq = boost::static_object<key_equal>::get();
      //value_type const& t = boost::static_object<value_type>::get();
      //key_type const& k = boost::static_object<key_type>::get();
      allocator_type const& a = boost::static_object<allocator_type>::get();

      hash_map_type x1(n, hf, eq, a);
      hash_map_type x2(i, j, n, hf, eq, a);
      swap(x1, x2);
      std::tr1::swap(x1, x2);
   }while(0);
   do{
      // unordered_map specific functions:
      typedef hash_multimap_type::key_type key_type;
      typedef hash_multimap_type::hasher hasher;
      typedef hash_multimap_type::key_equal key_equal;
      typedef hash_multimap_type::local_iterator local_iterator;
      typedef hash_multimap_type::const_local_iterator const_local_iterator;
      typedef hash_multimap_type::value_type value_type;
      typedef hash_multimap_type::iterator iterator;
      typedef hash_multimap_type::const_iterator const_iterator;
      typedef hash_multimap_type::size_type size_type;
      typedef hash_multimap_type::allocator_type allocator_type;
      typedef boost::input_iterator_archetype<value_type> input_iterator;
      input_iterator i = boost::static_object<input_iterator>::get();
      input_iterator j = i;

      size_type n = 1;
      const hasher& hf = boost::static_object<hasher>::get();
      const key_equal& eq = boost::static_object<key_equal>::get();
      //value_type const& t = boost::static_object<value_type>::get();
      //key_type const& k = boost::static_object<key_type>::get();
      allocator_type const& a = boost::static_object<allocator_type>::get();

      hash_multimap_type x1(n, hf, eq, a);
      hash_multimap_type x2(i, j, n, hf, eq, a);
      swap(x1, x2);
      std::tr1::swap(x1, x2);
   }while(0);
}





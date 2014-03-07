//  (C) Copyright John Maddock 2008.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "boost/concept_check.hpp"
#include "boost/concept_archetype.hpp"
#include "verify_return.hpp"

namespace boost{

template <class Arg, class Return>
class default_constructible_unary_function_archetype {
public:
   const Return& operator()(const Arg&) const {
      return static_object<Return>::get(); 
   }
};

template <class Arg1, class Arg2, class Base = null_archetype<> >
class default_constructible_binary_predicate_archetype {
   typedef boolean_archetype Return;
public:
   const Return& operator()(const Arg1&, const Arg2&) const {
      return static_object<Return>::get(); 
   }
};

template <class Unordered>
struct UnorderedContainer
{
   void constraints()
   {
      function_requires<Container<Unordered> >();

      typedef typename Unordered::key_type key_type;
      typedef typename Unordered::hasher hasher;
      typedef typename Unordered::key_equal key_equal;
      typedef typename Unordered::local_iterator local_iterator;
      typedef typename Unordered::const_local_iterator const_local_iterator;
      typedef typename Unordered::value_type value_type;
      typedef typename Unordered::iterator iterator;
      typedef typename Unordered::const_iterator const_iterator;
      typedef typename Unordered::size_type size_type;

      function_requires<Assignable<key_type> >();
      function_requires<CopyConstructible<key_type> >();
      function_requires<UnaryFunction<hasher, std::size_t, key_type> >();
      function_requires<BinaryPredicate<key_equal, key_type, key_type> >();
      function_requires<ForwardIterator<local_iterator> >();
      function_requires<ForwardIterator<const_local_iterator> >();

      size_type n = 1;
      const hasher& hf = static_object<hasher>::get();
      const key_equal& eq = static_object<key_equal>::get();
      value_type const& t = static_object<value_type>::get();
      key_type const& k = static_object<key_type>::get();
      Unordered x1(n, hf, eq);
      Unordered x2(n, hf);
      Unordered x3(n);
      Unordered x4;

      typedef input_iterator_archetype<typename Unordered::value_type> input_iterator;
      input_iterator i = static_object<input_iterator>::get();
      input_iterator j = i;
      x1 = Unordered(i, j, n, hf, eq);
      x2 = Unordered(i, j, n, hf);
      x3 = Unordered(i, j, n);
      x4 = Unordered(i, j);
      x1 = x2;
      Unordered x5(x1);
      iterator q = x1.begin();
      const_iterator r = x1.begin();

      verify_return_type(x1.hash_function(), hf);
      verify_return_type(x1.key_eq(), eq);
      verify_return_type(x1.insert(q, t), q);
      x1.insert(i, j);
      verify_return_type(x1.erase(k), n);
      verify_return_type(x1.erase(q), q);
      verify_return_type(x1.erase(q, q), q);
      x1.clear();

      const Unordered& b = x1;
      verify_return_type(x1.find(k), q);
      verify_return_type(b.find(k), r);
      verify_return_type(b.count(k), n);
      verify_return_type(x1.equal_range(k), std::make_pair(q, q));
      verify_return_type(b.equal_range(k), std::make_pair(r, r));
      verify_return_type(b.bucket_count(), n);
      verify_return_type(b.max_bucket_count(), n);
      verify_return_type(b.bucket(k), n);
      verify_return_type(b.bucket_size(n), n);

      local_iterator li = x1.begin(n);
      const_local_iterator cli = b.begin(n);
      verify_return_type(x1.begin(n), li);
      verify_return_type(b.begin(n), cli);
      verify_return_type(x1.end(n), li);
      verify_return_type(b.end(n), cli);
      verify_return_type(b.load_factor(), 1.0f);
      verify_return_type(b.max_load_factor(), 1.0f);
      x1.max_load_factor(1.0f);
      x1.rehash(n);
   }
};

template <class Unordered>
struct UniqueUnorderedContainer
{
   void constraints()
   {
      typedef typename Unordered::key_type key_type;
      typedef typename Unordered::hasher hasher;
      typedef typename Unordered::key_equal key_equal;
      typedef typename Unordered::local_iterator local_iterator;
      typedef typename Unordered::const_local_iterator const_local_iterator;
      typedef typename Unordered::value_type value_type;

      Unordered x;
      value_type const& t = static_object<value_type>::get();
      function_requires<UnorderedContainer<Unordered> >();
      verify_return_type(x.insert(t), static_object<std::pair<typename Unordered::iterator, bool> >::get());
   }
};

template <class Unordered>
struct MultiUnorderedContainer
{
   void constraints()
   {
      typedef typename Unordered::key_type key_type;
      typedef typename Unordered::hasher hasher;
      typedef typename Unordered::key_equal key_equal;
      typedef typename Unordered::local_iterator local_iterator;
      typedef typename Unordered::const_local_iterator const_local_iterator;
      typedef typename Unordered::value_type value_type;

      Unordered x;
      value_type const& t = static_object<value_type>::get();
      function_requires<UnorderedContainer<Unordered> >();
      verify_return_type(x.insert(t), static_object<typename Unordered::iterator>::get());
   }
};

}


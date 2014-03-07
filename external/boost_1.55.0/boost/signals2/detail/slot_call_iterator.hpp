// Boost.Signals2 library

// Copyright Douglas Gregor 2001-2004.
// Copyright Frank Mori Hess 2007-2008.
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS2_SLOT_CALL_ITERATOR_HPP
#define BOOST_SIGNALS2_SLOT_CALL_ITERATOR_HPP

#include <boost/assert.hpp>
#include <boost/aligned_storage.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/signals2/connection.hpp>
#include <boost/signals2/slot_base.hpp>
#include <boost/signals2/detail/auto_buffer.hpp>
#include <boost/signals2/detail/unique_lock.hpp>
#include <boost/weak_ptr.hpp>

namespace boost {
  namespace signals2 {
    namespace detail {
      template<typename ResultType, typename Function>
        class slot_call_iterator_cache
      {
      public:
        slot_call_iterator_cache(const Function &f_arg):
          f(f_arg),
          connected_slot_count(0),
          disconnected_slot_count(0)
        {}
        optional<ResultType> result;
        typedef auto_buffer<void_shared_ptr_variant, store_n_objects<10> > tracked_ptrs_type;
        tracked_ptrs_type tracked_ptrs;
        Function f;
        unsigned connected_slot_count;
        unsigned disconnected_slot_count;
      };

      // Generates a slot call iterator. Essentially, this is an iterator that:
      //   - skips over disconnected slots in the underlying list
      //   - calls the connected slots when dereferenced
      //   - caches the result of calling the slots
      template<typename Function, typename Iterator, typename ConnectionBody>
      class slot_call_iterator_t
        : public boost::iterator_facade<slot_call_iterator_t<Function, Iterator, ConnectionBody>,
        typename Function::result_type,
        boost::single_pass_traversal_tag,
        typename Function::result_type const&>
      {
        typedef boost::iterator_facade<slot_call_iterator_t<Function, Iterator, ConnectionBody>,
          typename Function::result_type,
          boost::single_pass_traversal_tag,
          typename Function::result_type const&>
        inherited;

        typedef typename Function::result_type result_type;

        friend class boost::iterator_core_access;

      public:
        slot_call_iterator_t(Iterator iter_in, Iterator end_in,
          slot_call_iterator_cache<result_type, Function> &c):
          iter(iter_in), end(end_in),
          cache(&c), callable_iter(end_in)
        {
          lock_next_callable();
        }

        typename inherited::reference
        dereference() const
        {
          if (!cache->result) {
            try
            {
              cache->result.reset(cache->f(*iter));
            }
            catch(expired_slot &)
            {
              (*iter)->disconnect();
              throw;
            }
          }
          return cache->result.get();
        }

        void increment()
        {
          ++iter;
          lock_next_callable();
          cache->result.reset();
        }

        bool equal(const slot_call_iterator_t& other) const
        {
          return iter == other.iter;
        }

      private:
        typedef unique_lock<connection_body_base> lock_type;

        void lock_next_callable() const
        {
          if(iter == callable_iter)
          {
            return;
          }
          for(;iter != end; ++iter)
          {
            lock_type lock(**iter);
            cache->tracked_ptrs.clear();
            (*iter)->nolock_grab_tracked_objects(std::back_inserter(cache->tracked_ptrs));
            if((*iter)->nolock_nograb_connected())
            {
              ++cache->connected_slot_count;
            }else
            {
              ++cache->disconnected_slot_count;
            }
            if((*iter)->nolock_nograb_blocked() == false)
            {
              callable_iter = iter;
              break;
            }
          }
          if(iter == end)
          {
            callable_iter = end;
          }
        }

        mutable Iterator iter;
        Iterator end;
        slot_call_iterator_cache<result_type, Function> *cache;
        mutable Iterator callable_iter;
      };
    } // end namespace detail
  } // end namespace BOOST_SIGNALS_NAMESPACE
} // end namespace boost

#endif // BOOST_SIGNALS2_SLOT_CALL_ITERATOR_HPP

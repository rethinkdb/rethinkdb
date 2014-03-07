// Boost.Signals2 library

// Copyright Frank Mori Hess 2007,2009.
// Copyright Timmo Stange 2007.
// Copyright Douglas Gregor 2001-2004. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Compatibility class to ease porting from the original
// Boost.Signals library.  However,
// boost::signals2::trackable is NOT thread-safe.

// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS2_TRACKABLE_HPP
#define BOOST_SIGNALS2_TRACKABLE_HPP

#include <boost/assert.hpp>
#include <boost/shared_ptr.hpp>

namespace boost {
  namespace signals2 {
    namespace detail
    {
        class tracked_objects_visitor;
    }
    class trackable {
    protected:
      trackable(): _tracked_ptr(static_cast<int*>(0)) {}
      trackable(const trackable &): _tracked_ptr(static_cast<int*>(0)) {}
      trackable& operator=(const trackable &)
      {
          return *this;
      }
      ~trackable() {}
    private:
      friend class detail::tracked_objects_visitor;
      const shared_ptr<void>& get_shared_ptr() const
      {
          return _tracked_ptr;
      }

      shared_ptr<void> _tracked_ptr;
    };
  } // end namespace signals2
} // end namespace boost

#endif // BOOST_SIGNALS2_TRACKABLE_HPP

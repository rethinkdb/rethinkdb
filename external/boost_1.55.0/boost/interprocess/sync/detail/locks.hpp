//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2012-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_LOCKS_HPP
#define BOOST_INTERPROCESS_DETAIL_LOCKS_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

namespace boost {
namespace interprocess {
namespace ipcdetail {

template<class Lock>
class internal_mutex_lock
{
   typedef void (internal_mutex_lock::*unspecified_bool_type)();
   public:

   typedef typename Lock::mutex_type::internal_mutex_type  mutex_type;
   

   internal_mutex_lock(Lock &l)
      : l_(l)
   {}

   mutex_type* mutex() const
   {  return l_ ? &l_.mutex()->internal_mutex() : 0;  }

   void lock()    { l_.lock(); }

   void unlock()  { l_.unlock(); }

   operator unspecified_bool_type() const
   {  return l_ ? &internal_mutex_lock::lock : 0;  }

   private:
   Lock &l_;
};

template <class Lock>
class lock_inverter
{
   Lock &l_;
   public:
   lock_inverter(Lock &l)
      :  l_(l)
   {}
   void lock()    {   l_.unlock();   }
   void unlock()  {   l_.lock();     }
};

}  //namespace ipcdetail
}  //namespace interprocess
}  //namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif   //BOOST_INTERPROCESS_DETAIL_LOCKS_HPP

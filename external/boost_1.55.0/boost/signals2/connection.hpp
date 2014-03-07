/*
  boost::signals2::connection provides a handle to a signal/slot connection.

  Author: Frank Mori Hess <fmhess@users.sourceforge.net>
  Begin: 2007-01-23
*/
// Copyright Frank Mori Hess 2007-2008.
// Distributed under the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/signals2 for library home page.

#ifndef BOOST_SIGNALS2_CONNECTION_HPP
#define BOOST_SIGNALS2_CONNECTION_HPP

#include <boost/function.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals2/detail/null_output_iterator.hpp>
#include <boost/signals2/detail/unique_lock.hpp>
#include <boost/signals2/slot.hpp>
#include <boost/weak_ptr.hpp>

namespace boost
{
  namespace signals2
  {
    extern inline void null_deleter(const void*) {}
    namespace detail
    {
      class connection_body_base
      {
      public:
        connection_body_base():
          _connected(true)
        {
        }
        virtual ~connection_body_base() {}
        void disconnect()
        {
          unique_lock<connection_body_base> local_lock(*this);
          nolock_disconnect();
        }
        void nolock_disconnect()
        {
          _connected = false;
        }
        virtual bool connected() const = 0;
        shared_ptr<void> get_blocker()
        {
          unique_lock<connection_body_base> local_lock(*this);
          shared_ptr<void> blocker = _weak_blocker.lock();
          if(blocker == shared_ptr<void>())
          {
            blocker.reset(this, &null_deleter);
            _weak_blocker = blocker;
          }
          return blocker;
        }
        bool blocked() const
        {
          return !_weak_blocker.expired();
        }
        bool nolock_nograb_blocked() const
        {
          return nolock_nograb_connected() == false || blocked();
        }
        bool nolock_nograb_connected() const {return _connected;}
        // expose part of Lockable concept of mutex
        virtual void lock() = 0;
        virtual void unlock() = 0;

      protected:

        mutable bool _connected;
        weak_ptr<void> _weak_blocker;
      };

      template<typename GroupKey, typename SlotType, typename Mutex>
      class connection_body: public connection_body_base
      {
      public:
        typedef Mutex mutex_type;
        connection_body(const SlotType &slot_in):
          slot(slot_in)
        {
        }
        virtual ~connection_body() {}
        virtual bool connected() const
        {
          unique_lock<mutex_type> local_lock(_mutex);
          nolock_grab_tracked_objects(detail::null_output_iterator());
          return nolock_nograb_connected();
        }
        const GroupKey& group_key() const {return _group_key;}
        void set_group_key(const GroupKey &key) {_group_key = key;}
        bool nolock_slot_expired() const
        {
          bool expired = slot.expired();
          if(expired == true)
          {
            _connected = false;
          }
          return expired;
        }
        template<typename OutputIterator>
          void nolock_grab_tracked_objects(OutputIterator inserter) const
        {
          slot_base::tracked_container_type::const_iterator it;
          for(it = slot.tracked_objects().begin();
            it != slot.tracked_objects().end();
            ++it)
          {
            void_shared_ptr_variant locked_object
            (
              apply_visitor
              (
                detail::lock_weak_ptr_visitor(),
                *it
              )
            );
            if(apply_visitor(detail::expired_weak_ptr_visitor(), *it))
            {
              _connected = false;
              return;
            }
            *inserter++ = locked_object;
          }
        }
        // expose Lockable concept of mutex
        virtual void lock()
        {
          _mutex.lock();
        }
        virtual void unlock()
        {
          _mutex.unlock();
        }
        SlotType slot;
      private:
        mutable mutex_type _mutex;
        GroupKey _group_key;
      };
    }

    class shared_connection_block;

    class connection
    {
    public:
      friend class shared_connection_block;

      connection() {}
      connection(const connection &other): _weak_connection_body(other._weak_connection_body)
      {}
      connection(const boost::weak_ptr<detail::connection_body_base> &connectionBody):
        _weak_connection_body(connectionBody)
      {}
      ~connection() {}
      void disconnect() const
      {
        boost::shared_ptr<detail::connection_body_base> connectionBody(_weak_connection_body.lock());
        if(connectionBody == 0) return;
        connectionBody->disconnect();
      }
      bool connected() const
      {
        boost::shared_ptr<detail::connection_body_base> connectionBody(_weak_connection_body.lock());
        if(connectionBody == 0) return false;
        return connectionBody->connected();
      }
      bool blocked() const
      {
        boost::shared_ptr<detail::connection_body_base> connectionBody(_weak_connection_body.lock());
        if(connectionBody == 0) return true;
        return connectionBody->blocked();
      }
      bool operator==(const connection& other) const
      {
        boost::shared_ptr<detail::connection_body_base> connectionBody(_weak_connection_body.lock());
        boost::shared_ptr<detail::connection_body_base> otherConnectionBody(other._weak_connection_body.lock());
        return connectionBody == otherConnectionBody;
      }
      bool operator!=(const connection& other) const
      {
        return !(*this == other);
      }
      bool operator<(const connection& other) const
      {
        boost::shared_ptr<detail::connection_body_base> connectionBody(_weak_connection_body.lock());
        boost::shared_ptr<detail::connection_body_base> otherConnectionBody(other._weak_connection_body.lock());
        return connectionBody < otherConnectionBody;
      }
      void swap(connection &other)
      {
        using std::swap;
        swap(_weak_connection_body, other._weak_connection_body);
      }
    protected:

      boost::weak_ptr<detail::connection_body_base> _weak_connection_body;
    };
    inline void swap(connection &conn1, connection &conn2)
    {
      conn1.swap(conn2);
    }

    class scoped_connection: public connection
    {
    public:
      scoped_connection() {}
      scoped_connection(const connection &other):
        connection(other)
      {}
      ~scoped_connection()
      {
        disconnect();
      }
      scoped_connection& operator=(const connection &rhs)
      {
        disconnect();
        connection::operator=(rhs);
        return *this;
      }
      connection release()
      {
        connection conn(_weak_connection_body);
        _weak_connection_body.reset();
        return conn;
      }
    private:
      scoped_connection(const scoped_connection &other);
      scoped_connection& operator=(const scoped_connection &rhs);
    };
    // Sun 5.9 compiler doesn't find the swap for base connection class when
    // arguments are scoped_connection, so we provide this explicitly.
    inline void swap(scoped_connection &conn1, scoped_connection &conn2)
    {
      conn1.swap(conn2);
    }
  }
}

#endif  // BOOST_SIGNALS2_CONNECTION_HPP

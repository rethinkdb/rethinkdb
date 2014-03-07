// Boost.Signals library

// Copyright Douglas Gregor 2001-2004. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#define BOOST_SIGNALS_SOURCE

#include <boost/signals/connection.hpp>
#include <cassert>

namespace boost {
  namespace BOOST_SIGNALS_NAMESPACE {

    connection::connection(const connection& other) :
      con(other.con), controlling_connection(other.controlling_connection)
    {
    }

    connection::~connection()
    {
      if (controlling_connection) {
        disconnect();
      }
    }

    void
    connection::reset(BOOST_SIGNALS_NAMESPACE::detail::basic_connection* new_con)
    {
      con.reset(new_con);
    }

    bool connection::operator==(const connection& other) const
    {
      return con.get() == other.con.get();
    }

    bool connection::operator<(const connection& other) const
    {
      return con.get() < other.con.get();
    }

    connection& connection::operator=(const connection& other)
    {
      connection(other).swap(*this);
      return *this;
    }

    void connection::swap(connection& other)
    {
      this->con.swap(other.con);
      std::swap(this->controlling_connection, other.controlling_connection);
    }

    void swap(connection& c1, connection& c2)
    {
      c1.swap(c2);
    }

    scoped_connection::scoped_connection(const connection& other) :
      connection(other),
      released(false)
    {
    }

    scoped_connection::scoped_connection(const scoped_connection& other) :
      connection(other),
      released(other.released)
    {
    }

    scoped_connection::~scoped_connection()
    {
      if (!released) {
        this->disconnect();
      }
    }

    connection scoped_connection::release()
    {
      released = true;
      return *this;
    }

    void scoped_connection::swap(scoped_connection& other)
    {
      this->connection::swap(other);
      bool other_released = other.released;
      other.released = this->released;
      this->released = other_released;
    }

    void swap(scoped_connection& c1, scoped_connection& c2)
    {
      c1.swap(c2);
    }

    scoped_connection&
    scoped_connection::operator=(const connection& other)
    {
      scoped_connection(other).swap(*this);
      return *this;
    }

    scoped_connection&
    scoped_connection::operator=(const scoped_connection& other)
    {
      scoped_connection(other).swap(*this);
      return *this;
    }

    void
    connection::add_bound_object(const BOOST_SIGNALS_NAMESPACE::detail::bound_object& b)
    {
      assert(con.get() != 0);
      con->bound_objects.push_back(b);
    }


    void connection::disconnect() const
    {
      if (this->connected()) {
        // Make sure we have a reference to the basic_connection object,
        // because 'this' may disappear
        shared_ptr<detail::basic_connection> local_con = con;

        void (*signal_disconnect)(void*, void*) = local_con->signal_disconnect;

        // Note that this connection no longer exists
        // Order is important here: we could get into an infinite loop if this
        // isn't cleared before we try the disconnect.
        local_con->signal_disconnect = 0;

        // Disconnect signal
        signal_disconnect(local_con->signal, local_con->signal_data);

        // Disconnect all bound objects
        typedef std::list<BOOST_SIGNALS_NAMESPACE::detail::bound_object>::iterator iterator;
        for (iterator i = local_con->bound_objects.begin();
             i != local_con->bound_objects.end(); ++i) {
          assert(i->disconnect != 0);
          i->disconnect(i->obj, i->data);
        }
      }
    }
  } // end namespace boost
} // end namespace boost

#ifndef BOOST_MSVC
// Explicit instantiations to keep everything in the library
template class std::list<boost::BOOST_SIGNALS_NAMESPACE::detail::bound_object>;
#endif

///////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
///////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_TEST_MOVABLE_INT_HEADER
#define BOOST_INTERPROCESS_TEST_MOVABLE_INT_HEADER

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/move.hpp>

namespace boost {
namespace interprocess {
namespace test {

template<class T>
struct is_copyable;

template<>
struct is_copyable<int>
{
   static const bool value = true;
};


class movable_int
{
   BOOST_MOVABLE_BUT_NOT_COPYABLE(movable_int)

   public:
   movable_int()
      :  m_int(0)
   {}

   explicit movable_int(int a)
      :  m_int(a)
   {}

   movable_int(BOOST_RV_REF(movable_int) mmi)
      :  m_int(mmi.m_int)
   {  mmi.m_int = 0; }

   movable_int & operator= (BOOST_RV_REF(movable_int) mmi)
   {  this->m_int = mmi.m_int;   mmi.m_int = 0;  return *this;  }

   movable_int & operator= (int i)
   {  this->m_int = i;  return *this;  }

   bool operator ==(const movable_int &mi) const
   {  return this->m_int == mi.m_int;   }

   bool operator !=(const movable_int &mi) const
   {  return this->m_int != mi.m_int;   }

   bool operator <(const movable_int &mi) const
   {  return this->m_int < mi.m_int;   }

   bool operator <=(const movable_int &mi) const
   {  return this->m_int <= mi.m_int;   }

   bool operator >=(const movable_int &mi) const
   {  return this->m_int >= mi.m_int;   }

   bool operator >(const movable_int &mi) const
   {  return this->m_int > mi.m_int;   }

   int get_int() const
   {  return m_int;  }

   friend bool operator==(const movable_int &l, int r)
   {  return l.get_int() == r;   }

   friend bool operator==(int l, const movable_int &r)
   {  return l == r.get_int();   }

   private:
   int m_int;
};

template<class E, class T>
std::basic_ostream<E, T> & operator<<
   (std::basic_ostream<E, T> & os, movable_int const & p)

{
    os << p.get_int();
    return os;
}


template<>
struct is_copyable<movable_int>
{
   static const bool value = false;
};

class movable_and_copyable_int
{
   BOOST_COPYABLE_AND_MOVABLE(movable_and_copyable_int)

   public:
   movable_and_copyable_int()
      :  m_int(0)
   {}

   explicit movable_and_copyable_int(int a)
      :  m_int(a)
   {}

   movable_and_copyable_int(const movable_and_copyable_int& mmi)
      :  m_int(mmi.m_int)
   {}
  
   movable_and_copyable_int(BOOST_RV_REF(movable_and_copyable_int) mmi)
      :  m_int(mmi.m_int)
   {  mmi.m_int = 0; }

   movable_and_copyable_int &operator= (BOOST_COPY_ASSIGN_REF(movable_and_copyable_int) mi)
   {  this->m_int = mi.m_int;    return *this;  }

   movable_and_copyable_int & operator= (BOOST_RV_REF(movable_and_copyable_int) mmi)
   {  this->m_int = mmi.m_int;   mmi.m_int = 0;    return *this;  }

   movable_and_copyable_int & operator= (int i)
   {  this->m_int = i;  return *this;  }

   bool operator ==(const movable_and_copyable_int &mi) const
   {  return this->m_int == mi.m_int;   }

   bool operator !=(const movable_and_copyable_int &mi) const
   {  return this->m_int != mi.m_int;   }

   bool operator <(const movable_and_copyable_int &mi) const
   {  return this->m_int < mi.m_int;   }

   bool operator <=(const movable_and_copyable_int &mi) const
   {  return this->m_int <= mi.m_int;   }

   bool operator >=(const movable_and_copyable_int &mi) const
   {  return this->m_int >= mi.m_int;   }

   bool operator >(const movable_and_copyable_int &mi) const
   {  return this->m_int > mi.m_int;   }

   int get_int() const
   {  return m_int;  }

   friend bool operator==(const movable_and_copyable_int &l, int r)
   {  return l.get_int() == r;   }

   friend bool operator==(int l, const movable_and_copyable_int &r)
   {  return l == r.get_int();   }

   private:
   int m_int;
};

template<class E, class T>
std::basic_ostream<E, T> & operator<<
   (std::basic_ostream<E, T> & os, movable_and_copyable_int const & p)

{
    os << p.get_int();
    return os;
}

template<>
struct is_copyable<movable_and_copyable_int>
{
   static const bool value = true;
};

class copyable_int
{
   public:
   copyable_int()
      :  m_int(0)
   {}

   explicit copyable_int(int a)
      :  m_int(a)
   {}

   copyable_int(const copyable_int& mmi)
      :  m_int(mmi.m_int)
   {}
  
   copyable_int & operator= (int i)
   {  this->m_int = i;  return *this;  }

   bool operator ==(const copyable_int &mi) const
   {  return this->m_int == mi.m_int;   }

   bool operator !=(const copyable_int &mi) const
   {  return this->m_int != mi.m_int;   }

   bool operator <(const copyable_int &mi) const
   {  return this->m_int < mi.m_int;   }

   bool operator <=(const copyable_int &mi) const
   {  return this->m_int <= mi.m_int;   }

   bool operator >=(const copyable_int &mi) const
   {  return this->m_int >= mi.m_int;   }

   bool operator >(const copyable_int &mi) const
   {  return this->m_int > mi.m_int;   }

   int get_int() const
   {  return m_int;  }

   friend bool operator==(const copyable_int &l, int r)
   {  return l.get_int() == r;   }

   friend bool operator==(int l, const copyable_int &r)
   {  return l == r.get_int();   }

   private:
   int m_int;
};

template<class E, class T>
std::basic_ostream<E, T> & operator<<
   (std::basic_ostream<E, T> & os, copyable_int const & p)

{
    os << p.get_int();
    return os;
}

template<>
struct is_copyable<copyable_int>
{
   static const bool value = true;
};

class non_copymovable_int
{
   non_copymovable_int(const non_copymovable_int& mmi);
   non_copymovable_int & operator= (const non_copymovable_int &mi);

   public:
   non_copymovable_int()
      :  m_int(0)
   {}

   explicit non_copymovable_int(int a)
      :  m_int(a)
   {}

   bool operator ==(const non_copymovable_int &mi) const
   {  return this->m_int == mi.m_int;   }

   bool operator !=(const non_copymovable_int &mi) const
   {  return this->m_int != mi.m_int;   }

   bool operator <(const non_copymovable_int &mi) const
   {  return this->m_int < mi.m_int;   }

   bool operator <=(const non_copymovable_int &mi) const
   {  return this->m_int <= mi.m_int;   }

   bool operator >=(const non_copymovable_int &mi) const
   {  return this->m_int >= mi.m_int;   }

   bool operator >(const non_copymovable_int &mi) const
   {  return this->m_int > mi.m_int;   }

   int get_int() const
   {  return m_int;  }

   friend bool operator==(const non_copymovable_int &l, int r)
   {  return l.get_int() == r;   }

   friend bool operator==(int l, const non_copymovable_int &r)
   {  return l == r.get_int();   }

   private:
   int m_int;
};

}  //namespace test {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_TEST_MOVABLE_INT_HEADER

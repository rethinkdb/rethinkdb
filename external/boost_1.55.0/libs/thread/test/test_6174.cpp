// Copyright (C) 2010 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#define BOOST_THREAD_VERSION 3

#include <boost/thread/thread_only.hpp>
#include <boost/thread/future.hpp>
#include <boost/config.hpp>

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
struct MovableButNonCopyable {
#if ! defined BOOST_NO_CXX11_DELETED_FUNCTIONS
      MovableButNonCopyable(MovableButNonCopyable const&) = delete;
      MovableButNonCopyable& operator=(MovableButNonCopyable const&) = delete;
#else
private:
    MovableButNonCopyable(MovableButNonCopyable const&);
    MovableButNonCopyable& operator=(MovableButNonCopyable const&);
#endif
public:
    MovableButNonCopyable() {};
    MovableButNonCopyable(MovableButNonCopyable&&) {};
    MovableButNonCopyable& operator=(MovableButNonCopyable&&)
    {
      return *this;
    };
};

MovableButNonCopyable construct()
{
  return MovableButNonCopyable();
}

int main()
{
    boost::packaged_task<MovableButNonCopyable> pt(construct);
    pt();
    return 0;
}
#else
int main()
{
    return 0;
}
#endif

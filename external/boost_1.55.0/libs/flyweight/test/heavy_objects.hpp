/* Classes for Boost.Flyweight key-value tests.
 *
 * Copyright 2006-2009 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#ifndef BOOST_FLYWEIGHT_TEST_HEAVY_OBJECTS_HPP
#define BOOST_FLYWEIGHT_TEST_HEAVY_OBJECTS_HPP

#if defined(_MSC_VER)&&(_MSC_VER>=1200)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/noncopyable.hpp>
#include <iosfwd>
#include <string>

struct texture
{
  texture(const std::string& str=""):str(str){}

  friend bool operator==(
    const texture& x,const texture& y){return x.str==y.str;}
  friend bool operator< (
    const texture& x,const texture& y){return x.str< y.str;}
  friend bool operator!=(
    const texture& x,const texture& y){return x.str!=y.str;}
  friend bool operator> (
    const texture& x,const texture& y){return x.str> y.str;}
  friend bool operator>=(
    const texture& x,const texture& y){return x.str>=y.str;}
  friend bool operator<=(
    const texture& x,const texture& y){return x.str<=y.str;}

  friend std::ostream& operator<<(std::ostream& os,const texture& x)
  {
    return os<<x.str;
  }

  friend std::istream& operator>>(std::istream& is,texture& x)
  {
    return is>>x.str;
  }

  std::string str;
};

struct from_texture_to_string
{
  const std::string& operator()(const texture& x)const{return x.str;}
};

struct factorization:private boost::noncopyable
{
  factorization(int n=0):n(n){}

  friend bool operator==(
    const factorization& x,const factorization& y){return x.n==y.n;}
  friend bool operator< (
    const factorization& x,const factorization& y){return x.n< y.n;}
  friend bool operator!=(
    const factorization& x,const factorization& y){return x.n!=y.n;}
  friend bool operator> (
    const factorization& x,const factorization& y){return x.n> y.n;}
  friend bool operator>=(
    const factorization& x,const factorization& y){return x.n>=y.n;}
  friend bool operator<=(
    const factorization& x,const factorization& y){return x.n<=y.n;}

  friend std::ostream& operator<<(std::ostream& os,const factorization& x)
  {
    return os<<x.n;
  }

  friend std::istream& operator>>(std::istream& is,factorization& x)
  {
    return is>>x.n;
  }

  int n;
};

#if !defined(BOOST_NO_EXCEPTIONS)
struct throwing_value_exception{};

struct throwing_value
{
  throwing_value():n(0){}
  throwing_value(const throwing_value&){throw throwing_value_exception();}
  throwing_value(int){throw throwing_value_exception();}

  int n;
};

struct from_throwing_value_to_int
{
  const int& operator()(const throwing_value& x)const{return x.n;}
};
#endif

#endif

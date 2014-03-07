//  (C) Copyright Terje Slettebo 2001.
//  (C) Copyright John Maddock 2001.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_NRVO
//  TITLE:         Named return value optimisation.
//  DESCRIPTION:   Named return value optimisation.


namespace boost_has_nrvo
{

class test_class
{
public:
  test_class() {}
  test_class(const test_class&)
  {
    ++copy_count;
  }

  static int copy_count;
};

int test_class::copy_count;

test_class f()
{
  test_class nrv;

  return nrv;
}

int test()
{
  test_class::copy_count=0;

  f();

  return test_class::copy_count;
}

} // namespace boost_has_nrvo











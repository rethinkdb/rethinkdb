//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//  Adaptation to Boost of the libcxx
//  Copyright 2010 Vicente J. Botet Escriba
//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#include <boost/ratio/ratio_io.hpp>

//typedef boost::ratio_string<boost::ratio_add<boost::ratio<1,2>, boost::ratio<1,3> >, char> R1;
typedef boost::ratio_string<int, char> R1;

void test() {

    std::string str = R1::symbol();
}

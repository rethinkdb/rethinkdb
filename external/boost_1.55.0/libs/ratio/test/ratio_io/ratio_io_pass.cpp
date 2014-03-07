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

// test ratio_add

#include <boost/ratio/ratio_io.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <climits>

int main()
{

    {
        BOOST_TEST((
                boost::ratio_string<boost::atto, char>::prefix() == "atto"
        ));
        BOOST_TEST((
                boost::ratio_string<boost::atto, char>::symbol() == "a"
        ));
    }
    {
        BOOST_TEST((
                boost::ratio_string<boost::femto, char>::prefix() == "femto"
        ));
        BOOST_TEST((
                boost::ratio_string<boost::femto, char>::symbol() == "f"
        ));
    }
    {
        BOOST_TEST((
                boost::ratio_string<boost::pico, char>::prefix() == "pico"
        ));
        BOOST_TEST((
                boost::ratio_string<boost::pico, char>::symbol() == "p"
        ));
    }
    {
        BOOST_TEST((
                boost::ratio_string<boost::nano, char>::prefix() == "nano"
        ));
        BOOST_TEST((
                boost::ratio_string<boost::nano, char>::symbol() == "n"
        ));
    }
    {
        BOOST_TEST((
                boost::ratio_string<boost::micro, char>::prefix() == "micro"
        ));
        BOOST_TEST((
                boost::ratio_string<boost::micro, char>::symbol() == "\xC2\xB5"
        ));
    }
    {
        BOOST_TEST((
                boost::ratio_string<boost::milli, char>::prefix() == "milli"
        ));
        BOOST_TEST((
                boost::ratio_string<boost::milli, char>::symbol() == "m"
        ));
    }
    {
        BOOST_TEST((
                boost::ratio_string<boost::centi, char>::prefix() == "centi"
        ));
        BOOST_TEST((
                boost::ratio_string<boost::centi, char>::symbol() == "c"
        ));
    }
    {
        BOOST_TEST((
                boost::ratio_string<boost::deci, char>::prefix() == "deci"
        ));
        BOOST_TEST((
                boost::ratio_string<boost::deci, char>::symbol() == "d"
        ));
    }
    {
        BOOST_TEST((
                boost::ratio_string<boost::deca, char>::prefix() == "deca"
        ));
        BOOST_TEST((
                boost::ratio_string<boost::deca, char>::symbol() == "da"
        ));
    }
    {
        BOOST_TEST((
                boost::ratio_string<boost::hecto, char>::prefix() == "hecto"
        ));
        BOOST_TEST((
                boost::ratio_string<boost::hecto, char>::symbol() == "h"
        ));
    }
    {
        BOOST_TEST((
                boost::ratio_string<boost::kilo, char>::prefix() == "kilo"
        ));
        BOOST_TEST((
                boost::ratio_string<boost::kilo, char>::symbol() == "k"
        ));
    }
    {
        BOOST_TEST((
                boost::ratio_string<boost::mega, char>::prefix() == "mega"
        ));
        BOOST_TEST((
                boost::ratio_string<boost::mega, char>::symbol() == "M"
        ));
    }
    {
        BOOST_TEST((
                boost::ratio_string<boost::giga, char>::prefix() == "giga"
        ));
        BOOST_TEST((
                boost::ratio_string<boost::giga, char>::symbol() == "G"
        ));
    }
    {
        BOOST_TEST((
                boost::ratio_string<boost::tera, char>::prefix() == "tera"
        ));
        BOOST_TEST((
                boost::ratio_string<boost::tera, char>::symbol() == "T"
        ));
    }
    {
        BOOST_TEST((
                boost::ratio_string<boost::peta, char>::prefix() == "peta"
        ));
        BOOST_TEST((
                boost::ratio_string<boost::peta, char>::symbol() == "P"
        ));
    }
    {
        BOOST_TEST((
                boost::ratio_string<boost::exa, char>::prefix() == "exa"
        ));
        BOOST_TEST((
                boost::ratio_string<boost::exa, char>::symbol() == "E"
        ));
    }
//    return 1;
    return boost::report_errors();
}



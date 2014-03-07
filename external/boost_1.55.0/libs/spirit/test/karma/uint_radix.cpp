//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2011 Jan Frederick Eick
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/karma_numeric.hpp>

#include <boost/cstdint.hpp>

#include "test.hpp"

///////////////////////////////////////////////////////////////////////////////
//
//  *** BEWARE PLATFORM DEPENDENT!!! ***
//  *** The following assumes 32 bit boost::uint32_tegers.
//  *** Modify these constants when appropriate.
//
///////////////////////////////////////////////////////////////////////////////

char const* max_unsigned_base2 =  "11111111111111111111111111111111";
char const* max_unsigned_base3 =  "102002022201221111210";
char const* max_unsigned_base4 =  "3333333333333333";
char const* max_unsigned_base5 =  "32244002423140";
char const* max_unsigned_base6 =  "1550104015503";
char const* max_unsigned_base7 =  "211301422353";
char const* max_unsigned_base8 =  "37777777777";
char const* max_unsigned_base9 =  "12068657453";
char const* max_unsigned_base11 = "1904440553";
char const* max_unsigned_base12 = "9ba461593";
char const* max_unsigned_base13 = "535a79888";
char const* max_unsigned_base14 = "2ca5b7463";
char const* max_unsigned_base15 = "1a20dcd80";
char const* max_unsigned_base16 = "ffffffff";
char const* max_unsigned_base17 = "a7ffda90";
char const* max_unsigned_base18 = "704he7g3";
char const* max_unsigned_base19 = "4f5aff65";
char const* max_unsigned_base20 = "3723ai4f";
char const* max_unsigned_base21 = "281d55i3";
char const* max_unsigned_base22 = "1fj8b183";
char const* max_unsigned_base23 = "1606k7ib";
char const* max_unsigned_base24 = "mb994af";
char const* max_unsigned_base25 = "hek2mgk";
char const* max_unsigned_base26 = "dnchbnl";
char const* max_unsigned_base27 = "b28jpdl";
char const* max_unsigned_base28 = "8pfgih3";
char const* max_unsigned_base29 = "76beigf";
char const* max_unsigned_base30 = "5qmcpqf";
char const* max_unsigned_base31 = "4q0jto3";
char const* max_unsigned_base32 = "3vvvvvv";
char const* max_unsigned_base33 = "3aokq93";
char const* max_unsigned_base34 = "2qhxjlh";
char const* max_unsigned_base35 = "2br45qa";
char const* max_unsigned_base36 = "1z141z3";

int 
main()
{
    using spirit_test::test;
    using boost::spirit::karma::uint_generator;

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 2)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 2> base2_generator;

        BOOST_TEST(test("1100111100100110010", base2_generator(424242)));
        BOOST_TEST(test("1100111100100110010", base2_generator, 424242));

        BOOST_TEST(test(max_unsigned_base2, base2_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base2, base2_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 3)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 3> base3_generator;

        BOOST_TEST(test("210112221200", base3_generator(424242)));
        BOOST_TEST(test("210112221200", base3_generator, 424242));

        BOOST_TEST(test(max_unsigned_base3, base3_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base3, base3_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 4)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 4> base4_generator;

        BOOST_TEST(test("1213210302", base4_generator(424242)));
        BOOST_TEST(test("1213210302", base4_generator, 424242));

        BOOST_TEST(test(max_unsigned_base4, base4_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base4, base4_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 5)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 5> base5_generator;

        BOOST_TEST(test("102033432", base5_generator(424242)));
        BOOST_TEST(test("102033432", base5_generator, 424242));

        BOOST_TEST(test(max_unsigned_base5, base5_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base5, base5_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 6)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 6> base6_generator;

        BOOST_TEST(test("13032030", base6_generator(424242)));
        BOOST_TEST(test("13032030", base6_generator, 424242));

        BOOST_TEST(test(max_unsigned_base6, base6_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base6, base6_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 7)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 7> base7_generator;

        BOOST_TEST(test("3414600", base7_generator(424242)));
        BOOST_TEST(test("3414600", base7_generator, 424242));

        BOOST_TEST(test(max_unsigned_base7, base7_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base7, base7_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 8)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 8> base8_generator;

        BOOST_TEST(test("1474462", base8_generator(424242)));
        BOOST_TEST(test("1474462", base8_generator, 424242));

        BOOST_TEST(test(max_unsigned_base8, base8_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base8, base8_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 9)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 9> base9_generator;

        BOOST_TEST(test("715850", base9_generator(424242)));
        BOOST_TEST(test("715850", base9_generator, 424242));

        BOOST_TEST(test(max_unsigned_base9, base9_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base9, base9_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 11)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 11> base11_generator;

        BOOST_TEST(test("26a815", base11_generator(424242)));
        BOOST_TEST(test("26a815", base11_generator, 424242));

        BOOST_TEST(test(max_unsigned_base11, base11_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base11, base11_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 12)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 12> base12_generator;

        BOOST_TEST(test("185616", base12_generator(424242)));
        BOOST_TEST(test("185616", base12_generator, 424242));

        BOOST_TEST(test(max_unsigned_base12, base12_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base12, base12_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 13)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 13> base13_generator;

        BOOST_TEST(test("11b140", base13_generator(424242)));
        BOOST_TEST(test("11b140", base13_generator, 424242));

        BOOST_TEST(test(max_unsigned_base13, base13_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base13, base13_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 14)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 14> base14_generator;

        BOOST_TEST(test("b0870", base14_generator(424242)));
        BOOST_TEST(test("b0870", base14_generator, 424242));

        BOOST_TEST(test(max_unsigned_base14, base14_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base14, base14_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 15)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 15> base15_generator;

        BOOST_TEST(test("85a7c", base15_generator(424242)));
        BOOST_TEST(test("85a7c", base15_generator, 424242));

        BOOST_TEST(test(max_unsigned_base15, base15_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base15, base15_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 16)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 16> base16_generator;

        BOOST_TEST(test("67932", base16_generator(424242)));
        BOOST_TEST(test("67932", base16_generator, 424242));

        BOOST_TEST(test(max_unsigned_base16, base16_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base16, base16_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 17)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 17> base17_generator;

        BOOST_TEST(test("515g7", base17_generator(424242)));
        BOOST_TEST(test("515g7", base17_generator, 424242));

        BOOST_TEST(test(max_unsigned_base17, base17_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base17, base17_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 18)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 18> base18_generator;

        BOOST_TEST(test("40d70", base18_generator(424242)));
        BOOST_TEST(test("40d70", base18_generator, 424242));

        BOOST_TEST(test(max_unsigned_base18, base18_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base18, base18_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 19)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 19> base19_generator;

        BOOST_TEST(test("34g3a", base19_generator(424242)));
        BOOST_TEST(test("34g3a", base19_generator, 424242));

        BOOST_TEST(test(max_unsigned_base19, base19_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base19, base19_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 20)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 20> base20_generator;

        BOOST_TEST(test("2d0c2", base20_generator(424242)));
        BOOST_TEST(test("2d0c2", base20_generator, 424242));

        BOOST_TEST(test(max_unsigned_base20, base20_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base20, base20_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 21)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 21> base21_generator;

        BOOST_TEST(test("23h00", base21_generator(424242)));
        BOOST_TEST(test("23h00", base21_generator, 424242));

        BOOST_TEST(test(max_unsigned_base21, base21_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base21, base21_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 22)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 22> base22_generator;

        BOOST_TEST(test("1hibg", base22_generator(424242)));
        BOOST_TEST(test("1hibg", base22_generator, 424242));

        BOOST_TEST(test(max_unsigned_base22, base22_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base22, base22_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 23)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 23> base23_generator;

        BOOST_TEST(test("1bjm7", base23_generator(424242)));
        BOOST_TEST(test("1bjm7", base23_generator, 424242));

        BOOST_TEST(test(max_unsigned_base23, base23_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base23, base23_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 24)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 24> base24_generator;

        BOOST_TEST(test("16gci", base24_generator(424242)));
        BOOST_TEST(test("16gci", base24_generator, 424242));

        BOOST_TEST(test(max_unsigned_base24, base24_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base24, base24_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 25)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 25> base25_generator;

        BOOST_TEST(test("123jh", base25_generator(424242)));
        BOOST_TEST(test("123jh", base25_generator, 424242));

        BOOST_TEST(test(max_unsigned_base25, base25_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base25, base25_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 26)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 26> base26_generator;

        BOOST_TEST(test("o3f0", base26_generator(424242)));
        BOOST_TEST(test("o3f0", base26_generator, 424242));

        BOOST_TEST(test(max_unsigned_base26, base26_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base26, base26_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 27)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 27> base27_generator;

        BOOST_TEST(test("lepi", base27_generator(424242)));
        BOOST_TEST(test("lepi", base27_generator, 424242));

        BOOST_TEST(test(max_unsigned_base27, base27_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base27, base27_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 28)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 28> base28_generator;

        BOOST_TEST(test("j93e", base28_generator(424242)));
        BOOST_TEST(test("j93e", base28_generator, 424242));

        BOOST_TEST(test(max_unsigned_base28, base28_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base28, base28_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 29)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 29> base29_generator;

        BOOST_TEST(test("hbd1", base29_generator(424242)));
        BOOST_TEST(test("hbd1", base29_generator, 424242));

        BOOST_TEST(test(max_unsigned_base29, base29_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base29, base29_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 30)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 30> base30_generator;

        BOOST_TEST(test("flbc", base30_generator(424242)));
        BOOST_TEST(test("flbc", base30_generator, 424242));

        BOOST_TEST(test(max_unsigned_base30, base30_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base30, base30_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 31)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 31> base31_generator;

        BOOST_TEST(test("e7e7", base31_generator(424242)));
        BOOST_TEST(test("e7e7", base31_generator, 424242));

        BOOST_TEST(test(max_unsigned_base31, base31_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base31, base31_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 32)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 32> base32_generator;

        BOOST_TEST(test("cu9i", base32_generator(424242)));
        BOOST_TEST(test("cu9i", base32_generator, 424242));

        BOOST_TEST(test(max_unsigned_base32, base32_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base32, base32_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 33)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 33> base33_generator;

        BOOST_TEST(test("bqir", base33_generator(424242)));
        BOOST_TEST(test("bqir", base33_generator, 424242));

        BOOST_TEST(test(max_unsigned_base33, base33_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base33, base33_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 34)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 34> base34_generator;

        BOOST_TEST(test("aqxo", base34_generator(424242)));
        BOOST_TEST(test("aqxo", base34_generator, 424242));

        BOOST_TEST(test(max_unsigned_base34, base34_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base34, base34_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 35)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 35> base35_generator;

        BOOST_TEST(test("9vb7", base35_generator(424242)));
        BOOST_TEST(test("9vb7", base35_generator, 424242));

        BOOST_TEST(test(max_unsigned_base35, base35_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base35, base35_generator, 0xffffffffu));
    }

    ///////////////////////////////////////////////////////////////////////////
    //  arbitrary radix test (base 36)
    ///////////////////////////////////////////////////////////////////////////
    {
        uint_generator<boost::uint32_t, 36> base36_generator;

        BOOST_TEST(test("93ci", base36_generator(424242)));
        BOOST_TEST(test("93ci", base36_generator, 424242));

        BOOST_TEST(test(max_unsigned_base36, base36_generator(0xffffffffu)));
        BOOST_TEST(test(max_unsigned_base36, base36_generator, 0xffffffffu));
    }

    return boost::report_errors();
}

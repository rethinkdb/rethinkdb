/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2001-2003 Daniel Nuffer
    Copyright (c) 2003      Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <iostream>
#include <boost/detail/lightweight_test.hpp>
#include <boost/detail/lightweight_test.hpp>
#include "impl/sstream.hpp"

#include <boost/spirit/include/classic_chset.hpp>

using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

namespace
{
    ///////////////////////////////////////////////////////////////////////////
    //
    //  chset tests
    //
    ///////////////////////////////////////////////////////////////////////////
    void
    DrawRuler(sstream_t& out, char const* str)
    {
        out << std::endl << std::endl;
        out << "\t_____________________________________________________________\n";
        out << "\t" << str << std::endl;
        out << "\t";
        for (char i = '!'; i < '^'; i++)
            out << i;
        out << "\n";
        out << "\t_____________________________________________________________\n\n";
    }

    //////////////////////////////////
    template <typename CharT>
    void
    Draw(sstream_t& out, chset<CharT> a, char const* str)
    {
        out << "\t";

        for (int i = '!'; i < '^'; i++)
            if (a.test(CharT(i)))
                out << '*';
            else
                out << " ";

        out << "\t" << str << std::endl;
    }

    //////////////////////////////////
    template <typename CharT>
    void
    chset_tests(sstream_t& out, CharT const* a_, CharT b1_, CharT b2_, CharT e1_)
    {
        chset<CharT>    a(a_);
        range<CharT>    b_(b1_, b2_);
        chset<CharT>    b(b_);
        chset<CharT>    c(~a);  // ~char_parser code must not interfere
                                // with chset
        negated_char_parser<range<CharT> > d_(~b_);
        chset<CharT>    d(d_);
        chlit<CharT>    e_(e1_);
        chset<CharT>    e(e_);
        negated_char_parser<chlit<CharT> > f_(e1_);
        chset<CharT>    f(f_);

        DrawRuler(out, "Initial");
        Draw(out, a, "a");
        Draw(out, b, "b");
        Draw(out, d, "d");
        Draw(out, e, "e");
        Draw(out, f, "f");

        DrawRuler(out, "Inverse");
        Draw(out, ~a, "~a");
        Draw(out, c, "chset<>(~a)");
        Draw(out, ~~a, "~~a");
        Draw(out, ~b, "~b");

        DrawRuler(out, "Union");
        Draw(out, a, "a");
        Draw(out, b, "b");
        Draw(out, d, "d");
        Draw(out, e, "e");
        Draw(out, f, "f");
        Draw(out, a | b, "a | b");
        Draw(out, a | b_, "a | b_");
        Draw(out, b_ | a, "b_ | a");
        Draw(out, a | anychar_p, "a | anychar_p");
        Draw(out, b | anychar_p, "b | anychar_p");
        Draw(out, a | d, "a | d");
        Draw(out, a | d_, "a | d_");
        Draw(out, d_ | a, "d_ | a");
        Draw(out, a | e_, "a | e_");
        Draw(out, e_ | b, "e_ | b");
        Draw(out, a | f_, "a | f_");
        Draw(out, f_ | b, "f_ | b");

        DrawRuler(out, "Intersection");
        Draw(out, a, "a");
        Draw(out, b, "b");
        Draw(out, d, "d");
        Draw(out, e, "e");
        Draw(out, f, "f");
        Draw(out, a & b, "a & b");
        Draw(out, a & b_, "a & b_");
        Draw(out, b_ & a, "b_ & a");
        Draw(out, a & d, "a & d");
        Draw(out, a & d_, "a & d_");
        Draw(out, d_ & a, "d_ & a");
        Draw(out, a & e_, "a & e_");
        Draw(out, e_ & b, "e_ & b");
        Draw(out, a & f_, "a & f_");
        Draw(out, f_ & b, "f_ & b");
        Draw(out, a & anychar_p, "a & anychar_p");
        Draw(out, b & anychar_p, "b & anychar_p");

        DrawRuler(out, "Difference");
        Draw(out, a, "a");
        Draw(out, b, "b");
        Draw(out, d, "d");
        Draw(out, e, "e");
        Draw(out, f, "f");
        Draw(out, a - b, "a - b");
        Draw(out, b - a, "b - a");
        Draw(out, a - b_, "a - b_");
        Draw(out, b_ - a, "b_ - a");
        Draw(out, a - d, "a - d");
        Draw(out, d - a, "d - a");
        Draw(out, a - d_, "a - d_");
        Draw(out, d_ - a, "d_ - a");
        Draw(out, a - e_, "a - e_");
        Draw(out, e_ - b, "e_ - b");
        Draw(out, a - f_, "a - f_");
        Draw(out, f_ - b, "f_ - b");
        Draw(out, a - anychar_p, "a - anychar_p");
        Draw(out, anychar_p - a, "anychar_p - a");
        Draw(out, b - anychar_p, "b - anychar_p");
        Draw(out, anychar_p - b, "anychar_p - b");

        DrawRuler(out, "Xor");
        Draw(out, a, "a");
        Draw(out, b, "b");
        Draw(out, d, "d");
        Draw(out, e, "e");
        Draw(out, f, "f");
        Draw(out, a ^ b, "a ^ b");
        Draw(out, a ^ b_, "a ^ b_");
        Draw(out, b_ ^ a, "b_ ^ a");
        Draw(out, a ^ d, "a ^ d");
        Draw(out, a ^ d_, "a ^ d_");
        Draw(out, d_ ^ a, "d_ ^ a");
        Draw(out, a ^ e_, "a ^ e_");
        Draw(out, e_ ^ b, "e_ ^ b");
        Draw(out, a ^ f_, "a ^ f_");
        Draw(out, f_ ^ b, "f_ ^ b");
        Draw(out, a ^ nothing_p, "a ^ nothing_p");
        Draw(out, a ^ anychar_p, "a ^ anychar_p");
        Draw(out, b ^ nothing_p, "b ^ nothing_p");
        Draw(out, b ^ anychar_p, "b ^ anychar_p");
    }

    char const* expected_output =
        "\n\n"
        "\t_____________________________________________________________\n"
        "\tInitial\n"
        "\t!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]\n"
        "\t_____________________________________________________________\n"
        "\n"
        "\t               **********       **************************   \ta\n"
        "\t                    **********************                   \tb\n"
        "\t********************                      *******************\td\n"
        "\t                 *                                           \te\n"
        "\t***************** *******************************************\tf\n"
        "\n"
        "\n"
        "\t_____________________________________________________________\n"
        "\tInverse\n"
        "\t!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]\n"
        "\t_____________________________________________________________\n"
        "\n"
        "\t***************          *******                          ***\t~a\n"
        "\t***************          *******                          ***\tchset<>(~a)\n"
        "\t               **********       **************************   \t~~a\n"
        "\t********************                      *******************\t~b\n"
        "\n"
        "\n"
        "\t_____________________________________________________________\n"
        "\tUnion\n"
        "\t!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]\n"
        "\t_____________________________________________________________\n"
        "\n"
        "\t               **********       **************************   \ta\n"
        "\t                    **********************                   \tb\n"
        "\t********************                      *******************\td\n"
        "\t                 *                                           \te\n"
        "\t***************** *******************************************\tf\n"
        "\t               *******************************************   \ta | b\n"
        "\t               *******************************************   \ta | b_\n"
        "\t               *******************************************   \tb_ | a\n"
        "\t*************************************************************\ta | anychar_p\n"
        "\t*************************************************************\tb | anychar_p\n"
        "\t*************************       *****************************\ta | d\n"
        "\t*************************       *****************************\ta | d_\n"
        "\t*************************       *****************************\td_ | a\n"
        "\t               **********       **************************   \ta | e_\n"
        "\t                 *  **********************                   \te_ | b\n"
        "\t*************************************************************\ta | f_\n"
        "\t***************** *******************************************\tf_ | b\n"
        "\n"
        "\n"
        "\t_____________________________________________________________\n"
        "\tIntersection\n"
        "\t!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]\n"
        "\t_____________________________________________________________\n"
        "\n"
        "\t               **********       **************************   \ta\n"
        "\t                    **********************                   \tb\n"
        "\t********************                      *******************\td\n"
        "\t                 *                                           \te\n"
        "\t***************** *******************************************\tf\n"
        "\t                    *****       **********                   \ta & b\n"
        "\t                    *****       **********                   \ta & b_\n"
        "\t                    *****       **********                   \tb_ & a\n"
        "\t               *****                      ****************   \ta & d\n"
        "\t               *****                      ****************   \ta & d_\n"
        "\t               *****                      ****************   \td_ & a\n"
        "\t                 *                                           \ta & e_\n"
        "\t                                                             \te_ & b\n"
        "\t               ** *******       **************************   \ta & f_\n"
        "\t                    **********************                   \tf_ & b\n"
        "\t               **********       **************************   \ta & anychar_p\n"
        "\t                    **********************                   \tb & anychar_p\n"
        "\n"
        "\n"
        "\t_____________________________________________________________\n"
        "\tDifference\n"
        "\t!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]\n"
        "\t_____________________________________________________________\n"
        "\n"
        "\t               **********       **************************   \ta\n"
        "\t                    **********************                   \tb\n"
        "\t********************                      *******************\td\n"
        "\t                 *                                           \te\n"
        "\t***************** *******************************************\tf\n"
        "\t               *****                      ****************   \ta - b\n"
        "\t                         *******                             \tb - a\n"
        "\t               *****                      ****************   \ta - b_\n"
        "\t                         *******                             \tb_ - a\n"
        "\t                    *****       **********                   \ta - d\n"
        "\t***************                                           ***\td - a\n"
        "\t                    *****       **********                   \ta - d_\n"
        "\t***************                                           ***\td_ - a\n"
        "\t               ** *******       **************************   \ta - e_\n"
        "\t                 *                                           \te_ - b\n"
        "\t                 *                                           \ta - f_\n"
        "\t***************** **                      *******************\tf_ - b\n"
        "\t                                                             \ta - anychar_p\n"
        "\t***************          *******                          ***\tanychar_p - a\n"
        "\t                                                             \tb - anychar_p\n"
        "\t********************                      *******************\tanychar_p - b\n"
        "\n"
        "\n"
        "\t_____________________________________________________________\n"
        "\tXor\n"
        "\t!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]\n"
        "\t_____________________________________________________________\n"
        "\n"
        "\t               **********       **************************   \ta\n"
        "\t                    **********************                   \tb\n"
        "\t********************                      *******************\td\n"
        "\t                 *                                           \te\n"
        "\t***************** *******************************************\tf\n"
        "\t               *****     *******          ****************   \ta ^ b\n"
        "\t               *****     *******          ****************   \ta ^ b_\n"
        "\t               *****     *******          ****************   \tb_ ^ a\n"
        "\t***************     *****       **********                ***\ta ^ d\n"
        "\t***************     *****       **********                ***\ta ^ d_\n"
        "\t***************     *****       **********                ***\td_ ^ a\n"
        "\t               ** *******       **************************   \ta ^ e_\n"
        "\t                 *  **********************                   \te_ ^ b\n"
        "\t***************  *       *******                          ***\ta ^ f_\n"
        "\t***************** **                      *******************\tf_ ^ b\n"
        "\t               **********       **************************   \ta ^ nothing_p\n"
        "\t***************          *******                          ***\ta ^ anychar_p\n"
        "\t                    **********************                   \tb ^ nothing_p\n"
        "\t********************                      *******************\tb ^ anychar_p\n"
    ;

    void chset_tests()
    {
        sstream_t tout, aout, bout;

        tout << expected_output;

        chset_tests(aout, "0-9A-Z", '5', 'J', '2');
        chset_tests(bout, L"0-9A-Z", L'5', L'J', L'2');

#define narrow_chset_works (getstring(aout) == getstring(tout))
#define wide_chset_works   (getstring(bout) == getstring(tout))

        if (!narrow_chset_works || !wide_chset_works)
        {
            std::cout << "EXPECTED:\n" <<
                getstring(tout);
            std::cout << "GOT:\n" <<
                getstring(aout);
            std::cout << "AND:\n" <<
                getstring(bout);
        }

        BOOST_TEST(narrow_chset_works);
        BOOST_TEST(wide_chset_works);
    }

} // namespace

int
main(int argc, char *argv[])
{
    chset_tests();
    return boost::report_errors();
}


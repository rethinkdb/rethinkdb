//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/locale/generator.hpp>
#include <boost/locale/info.hpp>
#include <boost/locale/message.hpp>
#include <iomanip>
#include "test_locale.hpp"


bool has_message(std::locale const &l)
{
    return std::has_facet<boost::locale::message_format<char> >(l);
}

struct test_facet : public std::locale::facet {
    test_facet() : std::locale::facet(0) {}
    static std::locale::id id;
};

std::locale::id test_facet::id;


int main()
{
    try {
        boost::locale::generator g;
        std::locale l=g("en_US.UTF-8");
        TEST(has_message(l));

        g.categories(g.categories() ^ boost::locale::message_facet);
        g.locale_cache_enabled(true);
        g("en_US.UTF-8");
        g.categories(g.categories() | boost::locale::message_facet);
        l=g("en_US.UTF-8");
        TEST(!has_message(l));
        g.clear_cache();
        g.locale_cache_enabled(false);
        l=g("en_US.UTF-8");
        TEST(has_message(l));
        g.characters(g.characters() ^ boost::locale::char_facet);
        l=g("en_US.UTF-8");
        TEST(!has_message(l));
        g.characters(g.characters() | boost::locale::char_facet);
        l=g("en_US.UTF-8");
        TEST(has_message(l));

        l=g("en_US.ISO8859-1");
        TEST(std::use_facet<boost::locale::info>(l).language()=="en");
        TEST(std::use_facet<boost::locale::info>(l).country()=="US");
        TEST(!std::use_facet<boost::locale::info>(l).utf8());
        TEST(std::use_facet<boost::locale::info>(l).encoding()=="iso8859-1");

        l=g("en_US.UTF-8");
        TEST(std::use_facet<boost::locale::info>(l).language()=="en");
        TEST(std::use_facet<boost::locale::info>(l).country()=="US");
        TEST(std::use_facet<boost::locale::info>(l).utf8());

        l=g("en_US.ISO8859-1");
        TEST(std::use_facet<boost::locale::info>(l).language()=="en");
        TEST(std::use_facet<boost::locale::info>(l).country()=="US");
        TEST(!std::use_facet<boost::locale::info>(l).utf8());
        TEST(std::use_facet<boost::locale::info>(l).encoding()=="iso8859-1");

        l=g("en_US.ISO8859-1");
        TEST(std::use_facet<boost::locale::info>(l).language()=="en");
        TEST(std::use_facet<boost::locale::info>(l).country()=="US");
        TEST(!std::use_facet<boost::locale::info>(l).utf8());
        TEST(std::use_facet<boost::locale::info>(l).encoding()=="iso8859-1");

        std::locale l_wt(std::locale::classic(),new test_facet);
        
        TEST(std::has_facet<test_facet>(g.generate(l_wt,"en_US.UTF-8")));
        TEST(std::has_facet<test_facet>(g.generate(l_wt,"en_US.ISO8859-1")));
        TEST(!std::has_facet<test_facet>(g("en_US.UTF-8")));
        TEST(!std::has_facet<test_facet>(g("en_US.ISO8859-1")));

        g.locale_cache_enabled(true);
        g.generate(l_wt,"en_US.UTF-8");
        g.generate(l_wt,"en_US.ISO8859-1");
        TEST(std::has_facet<test_facet>(g("en_US.UTF-8")));
        TEST(std::has_facet<test_facet>(g("en_US.ISO8859-1")));
        TEST(std::use_facet<boost::locale::info>(g("en_US.UTF-8")).utf8());
        TEST(!std::use_facet<boost::locale::info>(g("en_US.ISO8859-1")).utf8());

    }
    catch(std::exception const &e) {
        std::cerr << "Failed " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    FINALIZE();

}
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
// boostinspect:noascii 

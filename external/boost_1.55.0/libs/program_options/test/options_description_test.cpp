// Copyright Vladimir Prus 2002-2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <boost/program_options/options_description.hpp>
using namespace boost::program_options;

#include <boost/function.hpp>
using namespace boost;

#include <utility>
#include <string>
#include <sstream>
using namespace std;

#include "minitest.hpp"

void test_type()
{
    options_description desc;
    desc.add_options()
        ("foo", value<int>(), "")
        ("bar", value<string>(), "")
        ;
    
    const typed_value_base* b = dynamic_cast<const typed_value_base*>
        (desc.find("foo", false).semantic().get());
    BOOST_CHECK(b);
    BOOST_CHECK(b->value_type() == typeid(int));

    const typed_value_base* b2 = dynamic_cast<const typed_value_base*>
        (desc.find("bar", false).semantic().get());
    BOOST_CHECK(b2);
    BOOST_CHECK(b2->value_type() == typeid(string));
}

void test_approximation()
{
    options_description desc;
    desc.add_options()
        ("foo", new untyped_value())
        ("fee", new untyped_value())
        ("baz", new untyped_value())
        ("all-chroots", new untyped_value())
        ("all-sessions", new untyped_value())
        ("all", new untyped_value())
        ;

    BOOST_CHECK_EQUAL(desc.find("fo", true).long_name(), "foo");

    BOOST_CHECK_EQUAL(desc.find("all", true).long_name(), "all");
    BOOST_CHECK_EQUAL(desc.find("all-ch", true).long_name(), "all-chroots");

    options_description desc2;
    desc2.add_options()
        ("help", "display this message")
        ("config", value<string>(), "config file name")
        ("config-value", value<string>(), "single config value")
        ;

    BOOST_CHECK_EQUAL(desc2.find("config", true).long_name(), "config");
    BOOST_CHECK_EQUAL(desc2.find("config-value", true).long_name(), 
                      "config-value");


//    BOOST_CHECK(desc.count_approx("foo") == 1);
//    set<string> a = desc.approximations("f");
//    BOOST_CHECK(a.size() == 2);
//    BOOST_CHECK(*a.begin() == "fee");
//    BOOST_CHECK(*(++a.begin()) == "foo");
}

void test_formatting()
{
    // Long option descriptions used to crash on MSVC-8.0.
    options_description desc;
    desc.add_options()(
        "test", new untyped_value(),
        "foo foo foo foo foo foo foo foo foo foo foo foo foo foo"
        "foo foo foo foo foo foo foo foo foo foo foo foo foo foo"
        "foo foo foo foo foo foo foo foo foo foo foo foo foo foo"
        "foo foo foo foo foo foo foo foo foo foo foo foo foo foo")
      ("list", new untyped_value(),
         "a list:\n      \t"
             "item1, item2, item3, item4, item5, item6, item7, item8, item9, "
             "item10, item11, item12, item13, item14, item15, item16, item17, item18")
      ("well_formated", new untyped_value(), 
                        "As you can see this is a very well formatted option description.\n"
                        "You can do this for example:\n\n"
                        "Values:\n"
                        "  Value1: \tdoes this and that, bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla\n"
                        "  Value2: \tdoes something else, bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla\n\n"
                        "    This paragraph has a first line indent only, bla bla bla bla bla bla bla bla bla bla bla bla bla bla bla")
      ;

    stringstream ss;
    ss << desc;
    BOOST_CHECK_EQUAL(ss.str(),
"  --test arg            foo foo foo foo foo foo foo foo foo foo foo foo foo \n"
"                        foofoo foo foo foo foo foo foo foo foo foo foo foo foo \n"
"                        foofoo foo foo foo foo foo foo foo foo foo foo foo foo \n"
"                        foofoo foo foo foo foo foo foo foo foo foo foo foo foo \n"
"                        foo\n"
"  --list arg            a list:\n"
"                              item1, item2, item3, item4, item5, item6, item7, \n"
"                              item8, item9, item10, item11, item12, item13, \n"
"                              item14, item15, item16, item17, item18\n"
"  --well_formated arg   As you can see this is a very well formatted option \n"
"                        description.\n"
"                        You can do this for example:\n"
"                        \n"
"                        Values:\n"
"                          Value1: does this and that, bla bla bla bla bla bla \n"
"                                  bla bla bla bla bla bla bla bla bla\n"
"                          Value2: does something else, bla bla bla bla bla bla \n"
"                                  bla bla bla bla bla bla bla bla bla\n"
"                        \n"
"                            This paragraph has a first line indent only, bla \n"
"                        bla bla bla bla bla bla bla bla bla bla bla bla bla bla\n"
   );
}

void test_formatting_description_length()
{
    {
        options_description desc("",
                                 options_description::m_default_line_length,
                                 options_description::m_default_line_length / 2U);
        desc.add_options()
            ("an-option-that-sets-the-max", new untyped_value(), // > 40 available for desc
            "this description sits on the same line, but wrapping should still work correctly")
            ("a-long-option-that-would-leave-very-little-space-for-description", new untyped_value(),
            "the description of the long opt, but placed on the next line\n"
            "    \talso ensure that the tabulation works correctly when a"
            " description size has been set");

        stringstream ss;
        ss << desc;
        BOOST_CHECK_EQUAL(ss.str(),
        "  --an-option-that-sets-the-max arg     this description sits on the same line,\n"
        "                                        but wrapping should still work \n"
        "                                        correctly\n"
        "  --a-long-option-that-would-leave-very-little-space-for-description arg\n"
        "                                        the description of the long opt, but \n"
        "                                        placed on the next line\n"
        "                                            also ensure that the tabulation \n"
        "                                            works correctly when a description \n"
        "                                            size has been set\n");
    }
    {
        // the default behaviour reserves 23 (+1 space) characters for the
        // option column; this shows that the min_description_length does not
        // breach that.
        options_description desc("",
                                 options_description::m_default_line_length,
                                 options_description::m_default_line_length - 10U); // leaves < 23 (default option space)
        desc.add_options()
            ("an-option-that-encroaches-description", new untyped_value(),
            "this description should always be placed on the next line, and wrapping should continue as normal");

        stringstream ss;
        ss << desc;
        BOOST_CHECK_EQUAL(ss.str(),
        "  --an-option-that-encroaches-description arg\n"
       //123456789_123456789_
        "          this description should always be placed on the next line, and \n"
        "          wrapping should continue as normal\n");
    }
}

void test_long_default_value()
{
    options_description desc;
    desc.add_options()
        ("cfgfile,c",
         value<string>()->default_value("/usr/local/etc/myprogramXXXXXXXXX/configuration.conf"),
         "the configfile")
       ;

    stringstream ss;
    ss << desc;
    BOOST_CHECK_EQUAL(ss.str(),
"  -c [ --cfgfile ] arg (=/usr/local/etc/myprogramXXXXXXXXX/configuration.conf)\n"
"                                        the configfile\n"
   );
}

void test_word_wrapping()
{
   options_description desc("Supported options");
   desc.add_options()
      ("help",    "this is a sufficiently long text to require word-wrapping")
      ("prefix", value<string>()->default_value("/h/proj/tmp/dispatch"), "root path of the dispatch installation")
      ("opt1",    "this_is_a_sufficiently_long_text_to_require_word-wrapping_but_cannot_be_wrapped")
      ("opt2",    "this_is_a_sufficiently long_text_to_require_word-wrapping")
      ("opt3",    "this_is_a sufficiently_long_text_to_require_word-wrapping_but_will_not_be_wrapped")
      ;
    stringstream ss;
    ss << desc;    
    BOOST_CHECK_EQUAL(ss.str(),
"Supported options:\n"
"  --help                               this is a sufficiently long text to \n"
"                                       require word-wrapping\n"
"  --prefix arg (=/h/proj/tmp/dispatch) root path of the dispatch installation\n"
"  --opt1                               this_is_a_sufficiently_long_text_to_requ\n"
"                                       ire_word-wrapping_but_cannot_be_wrapped\n"
"  --opt2                               this_is_a_sufficiently \n"
"                                       long_text_to_require_word-wrapping\n"
"  --opt3                               this_is_a sufficiently_long_text_to_requ\n"
"                                       ire_word-wrapping_but_will_not_be_wrappe\n"
"                                       d\n"
   );
}

void test_default_values()
{
   options_description desc("Supported options");
   desc.add_options()
      ("maxlength", value<double>()->default_value(.1, "0.1"), "Maximum edge length to keep.")
      ;
   stringstream ss;
   ss << desc;    
   BOOST_CHECK_EQUAL(ss.str(),
"Supported options:\n"
"  --maxlength arg (=0.1) Maximum edge length to keep.\n"
   );   
}

void test_value_name()
{
    options_description desc("Supported options");
    desc.add_options()
        ("include", value<string>()->value_name("directory"), "Search for headers in 'directory'.")
        ;

    stringstream ss;
    ss << desc;
   BOOST_CHECK_EQUAL(ss.str(),
"Supported options:\n"
"  --include directory   Search for headers in 'directory'.\n"
   );
}


int main(int, char* [])
{
    test_type();
    test_approximation();
    test_formatting();
    test_formatting_description_length();
    test_long_default_value();
    test_word_wrapping();
    test_default_values();
    test_value_name();
    return 0;
}


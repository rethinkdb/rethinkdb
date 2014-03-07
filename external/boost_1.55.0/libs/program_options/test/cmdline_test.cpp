// Copyright Vladimir Prus 2002-2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/detail/cmdline.hpp>
using namespace boost::program_options;
using boost::program_options::detail::cmdline;

#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>
using namespace std;

#include "minitest.hpp"

/* To facilitate testing, declare a number of error codes. Otherwise,
   we'd have to specify the type of exception that should be thrown.
*/

const int s_success = 0;
const int s_unknown_option = 1;
const int s_ambiguous_option = 2;
const int s_long_not_allowed = 3;
const int s_long_adjacent_not_allowed = 4;
const int s_short_adjacent_not_allowed = 5;
const int s_empty_adjacent_parameter = 6;
const int s_missing_parameter = 7;
const int s_extra_parameter = 8;
const int s_unrecognized_line = 9;

int translate_syntax_error_kind(invalid_command_line_syntax::kind_t k)
{
    invalid_command_line_syntax::kind_t table[] = {
        invalid_command_line_syntax::long_not_allowed,
        invalid_command_line_syntax::long_adjacent_not_allowed,
        invalid_command_line_syntax::short_adjacent_not_allowed,
        invalid_command_line_syntax::empty_adjacent_parameter,
        invalid_command_line_syntax::missing_parameter,
        invalid_command_line_syntax::extra_parameter,
        invalid_command_line_syntax::unrecognized_line 
    };
    invalid_command_line_syntax::kind_t *b, *e, *i;
    b = table;
    e = table + sizeof(table)/sizeof(table[0]);
    i = std::find(b, e, k);
    assert(i != e);
    return std::distance(b, i) + 3;
}

struct test_case {
    const char* input;
    int expected_status;
    const char* expected_result;
};


/* Parses the syntax description in 'syntax' and initialized
   'cmd' accordingly' 
   The "boost::program_options" in parameter type is needed because CW9 
   has std::detail and it causes an ambiguity.
*/
void apply_syntax(options_description& desc, 
                  const char* syntax)
{
   
    string s;
    stringstream ss;
    ss << syntax;
    while(ss >> s) {
        value_semantic* v = 0;
        
        if (*(s.end()-1) == '=') {
            v = value<string>();
            s.resize(s.size()-1);
        } else if (*(s.end()-1) == '?') {
            //v = value<string>()->implicit();
            v = value<string>();
            s.resize(s.size()-1);
        } else if (*(s.end()-1) == '*') {
            v = value<vector<string> >()->multitoken();
            s.resize(s.size()-1);
        } else if (*(s.end()-1) == '+') {
            v = value<vector<string> >()->multitoken();
            s.resize(s.size()-1);
        }
        if (v) {
            desc.add_options()
                (s.c_str(), v, "");
        } else {
            desc.add_options()
                (s.c_str(), "");
        }
    }
}

void test_cmdline(const char* syntax, 
                  command_line_style::style_t style,
                  const test_case* cases)
{
    for (int i = 0; cases[i].input; ++i) {
        // Parse input
        vector<string> xinput;
        {
            string s;
            stringstream ss;
            ss << cases[i].input;
            while (ss >> s) {
                xinput.push_back(s);
            }
        }
        options_description desc;
        apply_syntax(desc, syntax);

        cmdline cmd(xinput);
        cmd.style(style);
        cmd.set_options_description(desc);


        string result;
        int status = 0;

        try {
            vector<option> options = cmd.run();

            for(unsigned i = 0; i < options.size(); ++i)
            {
                option opt = options[i];

                if (opt.position_key != -1) {
                    if (!result.empty())
                        result += " ";
                    result += opt.value[0];
                } else {
                    if (!result.empty())
                        result += " ";
                    result += opt.string_key + ":";
                    for (size_t j = 0; j < opt.value.size(); ++j) {
                        if (j != 0)
                            result += "-";
                        result += opt.value[j];
                    }                    
                }
            }
        }
        catch(unknown_option& e) {
            status = s_unknown_option;
        }
        catch(ambiguous_option& e) {
            status = s_ambiguous_option;
        }
        catch(invalid_command_line_syntax& e) {
            status = translate_syntax_error_kind(e.kind());
        }
        BOOST_CHECK_EQUAL(status, cases[i].expected_status);
        BOOST_CHECK_EQUAL(result, cases[i].expected_result);
    }
}

void test_long_options()
{
    using namespace command_line_style;
    cmdline::style_t style = cmdline::style_t(
        allow_long | long_allow_adjacent);

    test_case test_cases1[] = {
        // Test that long options are recognized and everything else
        // is treated like arguments
        {"--foo foo -123 /asd", s_success, "foo: foo -123 /asd"},

        // Unknown option
        {"--unk", s_unknown_option, ""},

        // Test that abbreviated names do not work
        {"--fo", s_unknown_option, ""},

        // Test for disallowed parameter
        {"--foo=13", s_extra_parameter, ""},

        // Test option with required parameter
        {"--bar=", s_empty_adjacent_parameter, ""},
        {"--bar", s_missing_parameter, ""},

        {"--bar=123", s_success, "bar:123"},
        {0, 0, 0}
    };
    test_cmdline("foo bar=", style, test_cases1);


    style = cmdline::style_t(
        allow_long | long_allow_next);

    test_case test_cases2[] = {
        {"--bar 10", s_success, "bar:10"},
        {"--bar", s_missing_parameter,  ""},
        // Since --bar accepts a parameter, --foo is
        // considered a value, even though it looks like
        // an option.
        {"--bar --foo", s_success, "bar:--foo"},
        {0, 0, 0}
    };
    test_cmdline("foo bar=", style, test_cases2);
    style = cmdline::style_t(
        allow_long | long_allow_adjacent
        | long_allow_next);

    test_case test_cases3[] = {
        {"--bar=10", s_success, "bar:10"},
        {"--bar 11", s_success, "bar:11"},
        {0, 0, 0}
    };
    test_cmdline("foo bar=", style, test_cases3);

    style = cmdline::style_t(
        allow_long | long_allow_adjacent
        | long_allow_next | case_insensitive);

    // Test case insensitive style.
    // Note that option names are normalized to lower case.
    test_case test_cases4[] = {
        {"--foo", s_success, "foo:"},
        {"--Foo", s_success, "foo:"},
        {"--bar=Ab", s_success, "bar:Ab"},
        {"--Bar=ab", s_success, "bar:ab"},
        {"--giz", s_success, "Giz:"},
        {0, 0, 0}
    };
    test_cmdline("foo bar= baz? Giz", style, test_cases4);
}

void test_short_options()
{
    using namespace command_line_style;
    cmdline::style_t style;

    style = cmdline::style_t(
        allow_short | allow_dash_for_short 
        | short_allow_adjacent);

    test_case test_cases1[] = {
        {"-d d /bar", s_success, "-d: d /bar"},
        // This is treated as error when long options are disabled
        {"--foo", s_success, "--foo"},
        {"-d13", s_extra_parameter, ""},
        {"-f14", s_success, "-f:14"},
        {"-g -f1", s_success, "-g: -f:1"},
        {"-f", s_missing_parameter, ""},
        {0, 0, 0}
    };
    test_cmdline(",d ,f= ,g", style, test_cases1);

    style = cmdline::style_t(
        allow_short | allow_dash_for_short
        | short_allow_next);

    test_case test_cases2[] = {
        {"-f 13", s_success, "-f:13"},
        {"-f -13", s_success, "-f:-13"},
        {"-f", s_missing_parameter, ""},
        {"-f /foo", s_success, "-f:/foo"},
        {"-f -d", s_missing_parameter, ""},
        {0, 0, 0}
    };
    test_cmdline(",d ,f=", style, test_cases2);

    style = cmdline::style_t(
        allow_short | short_allow_next
        | allow_dash_for_short | short_allow_adjacent);

    test_case test_cases3[] = {
        {"-f10", s_success, "-f:10"},
        {"-f 10", s_success, "-f:10"},
        {"-f -d", s_missing_parameter, ""},
        {0, 0, 0}
    };
    test_cmdline(",d ,f=", style, test_cases3);

    style = cmdline::style_t(
        allow_short | short_allow_next
        | allow_dash_for_short
        | short_allow_adjacent | allow_sticky);

    test_case test_cases4[] = {
        {"-de", s_success, "-d: -e:"},
        {"-df10", s_success, "-d: -f:10"},
        // FIXME: review
        //{"-d12", s_extra_parameter, ""},
        {"-f12", s_success, "-f:12"},
        {"-fe", s_success, "-f:e"},
        {0, 0, 0}
    };
    test_cmdline(",d ,f= ,e", style, test_cases4);

}


void test_dos_options()
{
    using namespace command_line_style;
    cmdline::style_t style;

    style = cmdline::style_t(
        allow_short
        | allow_slash_for_short | short_allow_adjacent);

    test_case test_cases1[] = {
        {"/d d -bar", s_success, "-d: d -bar"},
        {"--foo", s_success, "--foo"},
        {"/d13", s_extra_parameter, ""},
        {"/f14", s_success, "-f:14"},
        {"/f", s_missing_parameter, ""},
        {0, 0, 0}
    };
    test_cmdline(",d ,f=", style, test_cases1);

    style = cmdline::style_t(
        allow_short 
        | allow_slash_for_short | short_allow_next
        | short_allow_adjacent | allow_sticky);

    test_case test_cases2[] = {
        {"/de", s_extra_parameter, ""},
        {"/fe", s_success, "-f:e"},
        {0, 0, 0}
    };
    test_cmdline(",d ,f= ,e", style, test_cases2);

}


void test_disguised_long()
{
    using namespace command_line_style;
    cmdline::style_t style;

    style = cmdline::style_t(
        allow_short | short_allow_adjacent
        | allow_dash_for_short
        | short_allow_next | allow_long_disguise
        | long_allow_adjacent);

    test_case test_cases1[] = {
        {"-foo -f", s_success, "foo: foo:"},
        {"-goo=x -gy", s_success, "goo:x goo:y"},
        {"-bee=x -by", s_success, "bee:x bee:y"},
        {0, 0, 0}
    };
    test_cmdline("foo,f goo,g= bee,b?", style, test_cases1);

    style = cmdline::style_t(style | allow_slash_for_short);
    test_case test_cases2[] = {
        {"/foo -f", s_success, "foo: foo:"},
        {"/goo=x", s_success, "goo:x"},
        {0, 0, 0}
    };
    test_cmdline("foo,f goo,g= bee,b?", style, test_cases2);
}

void test_guessing()
{
    using namespace command_line_style;
    cmdline::style_t style;

    style = cmdline::style_t(
        allow_short | short_allow_adjacent
        | allow_dash_for_short        
        | allow_long | long_allow_adjacent
        | allow_guessing | allow_long_disguise);

    test_case test_cases1[] = {
        {"--opt1", s_success, "opt123:"},
        {"--opt", s_ambiguous_option, ""},
        {"--f=1", s_success, "foo:1"},
        {"-far", s_success, "foo:ar"},
        {0, 0, 0}
    };
    test_cmdline("opt123 opt56 foo,f=", style, test_cases1);

    test_case test_cases2[] = { 
        {"--fname file --fname2 file2", s_success, "fname: file fname2: file2"},
        {"--fnam file --fnam file2", s_ambiguous_option, ""},
        {"--fnam file --fname2 file2", s_ambiguous_option, ""},
        {"--fname2 file2 --fnam file", s_ambiguous_option, ""},
        {0, 0, 0} 
    };
    test_cmdline("fname fname2", style, test_cases2);
}

void test_arguments()
{
    using namespace command_line_style;
    cmdline::style_t style;

    style = cmdline::style_t(
        allow_short | allow_long
        | allow_dash_for_short
        | short_allow_adjacent | long_allow_adjacent);

    test_case test_cases1[] = {
        {"-f file -gx file2", s_success, "-f: file -g:x file2"},
        {"-f - -gx - -- -e", s_success, "-f: - -g:x - -e"},
        {0, 0, 0}
    };
    test_cmdline(",f ,g= ,e", style, test_cases1);

    // "--" should stop options regardless of whether long options are
    // allowed or not.

    style = cmdline::style_t(
        allow_short | short_allow_adjacent
        | allow_dash_for_short);

    test_case test_cases2[] = {
        {"-f - -gx - -- -e", s_success, "-f: - -g:x - -e"},
        {0, 0, 0}
    };
    test_cmdline(",f ,g= ,e", style, test_cases2);
}

void test_prefix()
{
    using namespace command_line_style;
    cmdline::style_t style;

    style = cmdline::style_t(
        allow_short | allow_long
        | allow_dash_for_short
        | short_allow_adjacent | long_allow_adjacent
        );

    test_case test_cases1[] = {
        {"--foo.bar=12", s_success, "foo.bar:12"},
        {0, 0, 0}
    };

    test_cmdline("foo*=", style, test_cases1);
}


pair<string, string> at_option_parser(string const&s)
{
    if ('@' == s[0])
        return std::make_pair(string("response-file"), s.substr(1));
    else
        return pair<string, string>();
}

pair<string, string> at_option_parser_broken(string const&s)
{
    if ('@' == s[0])
        return std::make_pair(string("some garbage"), s.substr(1));
    else
        return pair<string, string>();
}



void test_additional_parser()
{
    options_description desc;
    desc.add_options()
        ("response-file", value<string>(), "response file")
        ("foo", value<int>(), "foo")
        ;

    vector<string> input;
    input.push_back("@config");
    input.push_back("--foo=1");

    cmdline cmd(input);
    cmd.set_options_description(desc);
    cmd.set_additional_parser(at_option_parser);

    vector<option> result = cmd.run();

    BOOST_REQUIRE(result.size() == 2);
    BOOST_CHECK_EQUAL(result[0].string_key, "response-file");
    BOOST_CHECK_EQUAL(result[0].value[0], "config");
    BOOST_CHECK_EQUAL(result[1].string_key, "foo");
    BOOST_CHECK_EQUAL(result[1].value[0], "1");    

    // Test that invalid options returned by additional style
    // parser are detected.
    cmdline cmd2(input);
    cmd2.set_options_description(desc);
    cmd2.set_additional_parser(at_option_parser_broken);

    BOOST_CHECK_THROW(cmd2.run(), unknown_option);

}

vector<option> at_option_parser2(vector<string>& args)
{
    vector<option> result;
    if ('@' == args[0][0]) {
        // Simulate reading the response file.
        result.push_back(option("foo", vector<string>(1, "1")));
        result.push_back(option("bar", vector<string>(1, "1")));
        args.erase(args.begin());
    }
    return result;
}


void test_style_parser()
{
    options_description desc;
    desc.add_options()
        ("foo", value<int>(), "foo")
        ("bar", value<int>(), "bar")
        ;

    vector<string> input;
    input.push_back("@config");

    cmdline cmd(input);
    cmd.set_options_description(desc);
    cmd.extra_style_parser(at_option_parser2);

    vector<option> result = cmd.run();

    BOOST_REQUIRE(result.size() == 2);
    BOOST_CHECK_EQUAL(result[0].string_key, "foo");
    BOOST_CHECK_EQUAL(result[0].value[0], "1");    
    BOOST_CHECK_EQUAL(result[1].string_key, "bar");
    BOOST_CHECK_EQUAL(result[1].value[0], "1");    
}

void test_unregistered()
{
    // Check unregisted option when no options are registed at all.
    options_description desc;

    vector<string> input;
    input.push_back("--foo=1");
    input.push_back("--bar");
    input.push_back("1");
    input.push_back("-b");
    input.push_back("-biz");

    cmdline cmd(input);
    cmd.set_options_description(desc);
    cmd.allow_unregistered();
    
    vector<option> result = cmd.run();
    BOOST_REQUIRE(result.size() == 5);
    // --foo=1
    BOOST_CHECK_EQUAL(result[0].string_key, "foo");
    BOOST_CHECK_EQUAL(result[0].unregistered, true);
    BOOST_CHECK_EQUAL(result[0].value[0], "1");
    // --bar
    BOOST_CHECK_EQUAL(result[1].string_key, "bar");
    BOOST_CHECK_EQUAL(result[1].unregistered, true);
    BOOST_CHECK(result[1].value.empty());
    // '1' is considered a positional option, not a value to
    // --bar
    BOOST_CHECK(result[2].string_key.empty());
    BOOST_CHECK(result[2].position_key == 0);
    BOOST_CHECK_EQUAL(result[2].unregistered, false);
    BOOST_CHECK_EQUAL(result[2].value[0], "1");
    // -b
    BOOST_CHECK_EQUAL(result[3].string_key, "-b");
    BOOST_CHECK_EQUAL(result[3].unregistered, true);
    BOOST_CHECK(result[3].value.empty());
    // -biz
    BOOST_CHECK_EQUAL(result[4].string_key, "-b");
    BOOST_CHECK_EQUAL(result[4].unregistered, true);
    BOOST_CHECK_EQUAL(result[4].value[0], "iz");

    // Check sticky short options together with unregisted options.
    
    desc.add_options()
        ("help,h", "")
        ("magic,m", value<string>(), "")
        ;

    input.clear();
    input.push_back("-hc");
    input.push_back("-mc");


    cmdline cmd2(input);
    cmd2.set_options_description(desc);
    cmd2.allow_unregistered();
    
    result = cmd2.run();

    BOOST_REQUIRE(result.size() == 3);
    BOOST_CHECK_EQUAL(result[0].string_key, "help");
    BOOST_CHECK_EQUAL(result[0].unregistered, false);
    BOOST_CHECK(result[0].value.empty());
    BOOST_CHECK_EQUAL(result[1].string_key, "-c");
    BOOST_CHECK_EQUAL(result[1].unregistered, true);
    BOOST_CHECK(result[1].value.empty());
    BOOST_CHECK_EQUAL(result[2].string_key, "magic");
    BOOST_CHECK_EQUAL(result[2].unregistered, false);
    BOOST_CHECK_EQUAL(result[2].value[0], "c");

    // CONSIDER:
    // There's a corner case:
    //   -foo
    // when 'allow_long_disguise' is set. Should this be considered
    // disguised long option 'foo' or short option '-f' with value 'oo'?
    // It's not clear yet, so I'm leaving the decision till later.
}

int main(int /*ac*/, char** /*av*/)
{
    test_long_options();
    test_short_options();
    test_dos_options();
    test_disguised_long();
    test_guessing();
    test_arguments();
    test_prefix();
    test_additional_parser();
    test_style_parser();
    test_unregistered();

    return 0;
}

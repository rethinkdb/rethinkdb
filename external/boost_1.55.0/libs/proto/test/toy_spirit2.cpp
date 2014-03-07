///////////////////////////////////////////////////////////////////////////////
// toy_spirit3.cpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <cctype>
#include <string>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <boost/version.hpp>
#include <boost/assert.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/transform.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/fold.hpp>
#include <boost/fusion/include/cons.hpp>
#include <boost/fusion/include/any.hpp>
#include <boost/test/unit_test.hpp>

namespace boost
{
    // global tags
    struct char_tag {};
    struct space_tag {};

    // global primitives
    proto::terminal<char_tag>::type const char_ = {{}};
    proto::terminal<space_tag>::type const space = {{}};

    using proto::lit;
    using proto::literal;
}

namespace boost { namespace spirit2
{
    namespace utility
    {
        inline bool char_icmp(char ch, char lo, char hi)
        {
            return ch == lo || ch == hi;
        }

        template<typename FwdIter>
        inline bool string_cmp(char const *sz, FwdIter &begin, FwdIter end)
        {
            FwdIter tmp = begin;
            for(; *sz; ++tmp, ++sz)
                if(tmp == end || *tmp != *sz)
                    return false;
            begin = tmp;
            return true;
        }

        template<typename FwdIter>
        inline bool string_icmp(std::string const &str, FwdIter &begin, FwdIter end)
        {
            BOOST_ASSERT(0 == str.size() % 2);
            FwdIter tmp = begin;
            std::string::const_iterator istr = str.begin(), estr = str.end();
            for(; istr != estr; ++tmp, istr += 2)
                if(tmp == end || (*tmp != *istr && *tmp != *(istr+1)))
                    return false;
            begin = tmp;
            return true;
        }

        inline bool in_range(char ch, char lo, char hi)
        {
            return ch >= lo && ch <= hi;
        }

        inline bool in_irange(char ch, char lo, char hi)
        {
            return in_range(ch, lo, hi)
                || in_range(std::tolower(ch), lo, hi)
                || in_range(std::toupper(ch), lo, hi);
        }

        inline std::string to_istr(char const *sz)
        {
            std::string res;
            res.reserve(std::strlen(sz) * 2);
            for(; *sz; ++sz)
            {
                res.push_back(std::tolower(*sz));
                res.push_back(std::toupper(*sz));
            }
            return res;
        }
    } // namespace utility

    template<typename List>
    struct alternate
    {
        explicit alternate(List const &list)
          : elems(list)
        {}
        List elems;
    };

    template<typename List>
    struct sequence
    {
        explicit sequence(List const &list)
          : elems(list)
        {}
        List elems;
    };

    struct char_range
      : std::pair<char, char>
    {
        char_range(char from, char to)
          : std::pair<char, char>(from, to)
        {}
    };

    struct ichar
    {
        ichar(char ch)
          : lo_(std::tolower(ch))
          , hi_(std::toupper(ch))
        {}

        char lo_, hi_;
    };

    struct istr
    {
        istr(char const *sz)
          : str_(utility::to_istr(sz))
        {}

        std::string str_;
    };

    struct ichar_range
      : std::pair<char, char>
    {
        ichar_range(char from, char to)
          : std::pair<char, char>(from, to)
        {}
    };

    // The no-case directive
    struct no_case_tag {};

    struct True : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////////
    /// Begin Spirit grammar here
    ///////////////////////////////////////////////////////////////////////////////
    namespace grammar
    {
        using namespace proto;
        using namespace fusion;

        struct SpiritExpr;

        struct AnyChar
          : terminal<char_tag>
        {};

        struct CharLiteral
          : terminal<char>
        {};

        struct NTBSLiteral
          : terminal<char const *>
        {};

        struct CharParser
          : proto::function<AnyChar, CharLiteral>
        {};

        struct CharRangeParser
          : proto::function<AnyChar, CharLiteral, CharLiteral>
        {};

        struct NoCase
          : terminal<no_case_tag>
        {};

        // The data determines the case-sensitivity of the terminals
        typedef _data _icase;

        // Ugh, would be nice to find a work-around for this:
        #if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
        #define _value(x) call<_value(x)>
        #define True() make<True()>
        #endif

        // Extract the child from terminals
        struct SpiritTerminal
          : or_<
                when< AnyChar,          _value >
              , when< CharLiteral,      if_<_icase, ichar(_value), _value> >
              , when< CharParser,       if_<_icase, ichar(_value(_child1)), _value(_child1)> >  // char_('a')
              , when< NTBSLiteral,      if_<_icase, istr(_value), char const*(_value)> >
              , when< CharRangeParser,  if_<_icase
                                            , ichar_range(_value(_child1), _value(_child2))
                                            , char_range(_value(_child1), _value(_child2))> >   // char_('a','z')
            >
        {};

        struct FoldToList
          : reverse_fold_tree<_, nil(), cons<SpiritExpr, _state>(SpiritExpr, _state)>
        {};

        // sequence rule folds all >>'s together into a list
        // and wraps the result in a sequence<> wrapper
        struct SpiritSequence
          : when< shift_right<SpiritExpr, SpiritExpr>,  sequence<FoldToList>(FoldToList)  >
        {};

        // alternate rule folds all |'s together into a list
        // and wraps the result in a alternate<> wrapper
        struct SpiritAlternate
          : when< bitwise_or<SpiritExpr, SpiritExpr>,   alternate<FoldToList>(FoldToList) >
        {};

        // Directives such as no_case are handled here
        struct SpiritDirective
          : when< subscript<NoCase, SpiritExpr>, SpiritExpr(_right, _state, True()) >
        {};

        // A SpiritExpr is an alternate, a sequence, a directive or a terminal
        struct SpiritExpr
          : or_<
                SpiritSequence
              , SpiritAlternate
              , SpiritDirective
              , SpiritTerminal
            >
        {};

    } // namespace grammar

    using grammar::SpiritExpr;
    using grammar::NoCase;

    ///////////////////////////////////////////////////////////////////////////////
    /// End SpiritExpr
    ///////////////////////////////////////////////////////////////////////////////

    // Globals
    NoCase::type const no_case = {{}};

    template<typename Iterator>
    struct parser;

    template<typename Iterator>
    struct fold_alternate
    {
        parser<Iterator> const &parse;

        explicit fold_alternate(parser<Iterator> const &p)
          : parse(p)
        {}

        template<typename T>
        bool operator ()(T const &t) const
        {
            Iterator tmp = this->parse.first;
            if(this->parse(t))
                return true;
            this->parse.first = tmp;
            return false;
        }
    };

    template<typename Iterator>
    struct fold_sequence
    {
        parser<Iterator> const &parse;

        explicit fold_sequence(parser<Iterator> const &p)
          : parse(p)
        {}

        typedef bool result_type;

        #if BOOST_VERSION >= 104200
        template<typename T>
        bool operator ()(bool success, T const &t) const
        {
            return success && this->parse(t);
        }
        #else
        template<typename T>
        bool operator ()(T const &t, bool success) const
        {
            return success && this->parse(t);
        }
        #endif
    };

    template<typename Iterator>
    struct parser
    {
        mutable Iterator first;
        Iterator second;

        parser(Iterator begin, Iterator end)
          : first(begin)
          , second(end)
        {}

        bool done() const
        {
            return this->first == this->second;
        }

        template<typename List>
        bool operator ()(alternate<List> const &alternates) const
        {
            return fusion::any(alternates.elems, fold_alternate<Iterator>(*this));
        }

        template<typename List>
        bool operator ()(sequence<List> const &sequence) const
        {
            return fusion::fold(sequence.elems, true, fold_sequence<Iterator>(*this));
        }

        bool operator ()(char_tag ch) const
        {
            if(this->done())
                return false;
            ++this->first;
            return true;
        }

        bool operator ()(char ch) const
        {
            if(this->done() || ch != *this->first)
                return false;
            ++this->first;
            return true;
        }

        bool operator ()(ichar ich) const
        {
            if(this->done() || !utility::char_icmp(*this->first, ich.lo_, ich.hi_))
                return false;
            ++this->first;
            return true;
        }

        bool operator ()(char const *sz) const
        {
            return utility::string_cmp(sz, this->first, this->second);
        }

        bool operator ()(istr const &s) const
        {
            return utility::string_icmp(s.str_, this->first, this->second);
        }

        bool operator ()(char_range rng) const
        {
            if(this->done() || !utility::in_range(*this->first, rng.first, rng.second))
                return false;
            ++this->first;
            return true;
        }

        bool operator ()(ichar_range rng) const
        {
            if(this->done() || !utility::in_irange(*this->first, rng.first, rng.second))
                return false;
            ++this->first;
            return true;
        }
    };

    template<typename Rule, typename Iterator>
    typename enable_if<proto::matches< Rule, SpiritExpr >, bool >::type
    parse_impl(Rule const &rule, Iterator begin, Iterator end)
    {
        mpl::false_ is_case_sensitive;
        parser<Iterator> parse_fun(begin, end);
        return parse_fun(SpiritExpr()(rule, proto::ignore(), is_case_sensitive));
    }

    // 2nd overload provides a short error message for invalid rules
    template<typename Rule, typename Iterator>
    typename disable_if<proto::matches< Rule, SpiritExpr >, bool >::type
    parse_impl(Rule const &rule, Iterator begin, Iterator end)
    {
        BOOST_MPL_ASSERT((proto::matches<Rule, SpiritExpr>));
        return false;
    }

    // parse() converts rule literals to proto expressions if necessary
    // and dispatches to parse_impl
    template<typename Rule, typename Iterator>
    bool parse(Rule const &rule, Iterator begin, Iterator end)
    {
        return parse_impl(proto::as_expr(rule), begin, end);
    }

}}

void test_toy_spirit3()
{
    using boost::spirit2::no_case;
    using boost::char_;
    std::string hello("abcd");

    BOOST_CHECK(
        boost::spirit2::parse(
            "abcd"
          , hello.begin()
          , hello.end()
        )
    );

    BOOST_CHECK(
        boost::spirit2::parse(
            char_ >> char_('b') >> 'c' >> char_
          , hello.begin()
          , hello.end()
        )
    );

    BOOST_CHECK(
       !boost::spirit2::parse(
            char_ >> char_('b') >> 'c' >> 'D'
          , hello.begin()
          , hello.end()
        )
    );

    BOOST_CHECK(
        boost::spirit2::parse(
            char_ >> char_('b') >> 'c' >> 'e'
          | char_ >> no_case[char_('B') >> "C" >> char_('D','Z')]
          , hello.begin()
          , hello.end()
        )
    );

    std::string nest_alt_input("abd");
    BOOST_CHECK(
        boost::spirit2::parse(
            char_('a')
          >> ( char_('b')
             | char_('c')
             )
          >>  char_('d')
          , nest_alt_input.begin()
          , nest_alt_input.end()
        )
    );
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test proto, grammars and tree transforms");

    test->add(BOOST_TEST_CASE(&test_toy_spirit3));

    return test;
}

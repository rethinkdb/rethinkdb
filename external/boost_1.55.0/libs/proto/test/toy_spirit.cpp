///////////////////////////////////////////////////////////////////////////////
// toy_spirit.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <cctype>
#include <string>
#include <cstring>
#include <iostream>
#include <boost/assert.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
#include <boost/test/unit_test.hpp>

namespace boost
{
    // global tags
    struct char_tag {};
    struct ichar_tag {};
    struct istring_tag {};
    struct ichar_range_tag {};
    struct never_tag {};
    struct always_tag {};
    struct space_tag {};

    // global primitives
    proto::terminal<char_tag>::type const char_ = {{}};
    proto::terminal<space_tag>::type const space = {{}};

    using proto::lit;
    using proto::literal;
}

namespace boost { namespace spirit2
{

    // handy typedefs
    typedef proto::terminal<char_tag>::type anychar_p;
    typedef proto::terminal<ichar_tag>::type ianychar_p;
    typedef proto::terminal<istring_tag>::type ianystr_p;
    typedef proto::terminal<ichar_range_tag>::type ianychar_range_p;
    typedef proto::terminal<never_tag>::type never_p;
    typedef proto::terminal<space_tag>::type space_p;

    struct SpiritGrammar;
    struct SkipperGrammar;
    struct SpiritPrimitives;
    template<typename Grammar>
    struct SpiritComposites;

    struct CharLiteral
      : proto::terminal<char>
    {};

    struct NTBSLiteral
      : proto::terminal<char const *>
    {};

    struct StdStringLiteral
      : proto::terminal<std::string>
    {};

    struct CharParser
      : proto::function<anychar_p, CharLiteral>
    {};

    struct ICharParser
      : proto::function<ianychar_p, CharLiteral, CharLiteral>
    {};

    struct CharRangeParser
      : proto::function<anychar_p, CharLiteral, CharLiteral>
    {};

    struct IStrParser
      : proto::function<ianystr_p, StdStringLiteral>
    {};

    struct ICharRangeParser
      : proto::function<ianychar_range_p, CharLiteral, CharLiteral>
    {};

    ianychar_p const ichar_ = {{}};
    ianystr_p const istr_ = {{}};
    ianychar_range_p const ichar_range_ = {{}};

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

    template<typename FwdIter, typename Skipper = never_p>
    struct spirit_context
      : std::pair<FwdIter, FwdIter>
      , proto::callable_context<spirit_context<FwdIter, Skipper> >
    {
        typedef bool result_type;
        typedef FwdIter iterator;

        spirit_context(FwdIter first, FwdIter second, Skipper const &skip = Skipper())
          : std::pair<FwdIter, FwdIter>(first, second)
          , skip_(skip)
          , in_skip_(false)
        {}

        // parse function for anychar_p
        bool operator()(proto::tag::terminal, char_tag)
        {
            this->skip();
            if(this->first == this->second)
                return false;
            ++this->first;
            return true;
        }

        // parse function for char_('a')
        template<typename Expr>
        bool operator()(proto::tag::function, anychar_p, Expr const &expr)
        {
            this->skip();
            return proto::eval(expr, *this);
        }

        // parse function for space_p
        bool operator()(proto::tag::terminal, space_tag)
        {
            this->skip();
            if(this->first == this->second || !std::isspace(*this->first))
                return false;
            ++this->first;
            return true;
        }

        // parse function for bare character literals
        bool operator()(proto::tag::terminal, char ch)
        {
            this->skip();
            if(this->first == this->second || *this->first != ch)
                return false;
            ++this->first;
            return true;
        }

        // case-insensitive character parser
        template<typename Arg1, typename Arg2>
        bool operator()(proto::tag::function, ianychar_p, Arg1 const &arg1, Arg2 const &arg2)
        {
            this->skip();
            if(this->first == this->second
              || !utility::char_icmp(*this->first, proto::value(arg1), proto::value(arg2)))
                return false;
            ++this->first;
            return true;
        }

        // parse function for NTBS literals
        bool operator()(proto::tag::terminal, char const *sz)
        {
            this->skip();
            return utility::string_cmp(sz, this->first, this->second);
        }

        // parse function for istr_("hello")
        template<typename Expr>
        bool operator()(proto::tag::function, ianystr_p, Expr const &expr)
        {
            this->skip();
            return utility::string_icmp(proto::value(expr), this->first, this->second);
        }

        // parse function for char_('a','z')
        template<typename Arg1, typename Arg2>
        bool operator()(proto::tag::function, anychar_p, Arg1 const &arg1, Arg2 const &arg2)
        {
            BOOST_ASSERT(proto::value(arg1) <= proto::value(arg2));
            this->skip();
            if(this->first == this->second
              || !utility::in_range(*this->first, proto::value(arg1), proto::value(arg2)))
                return false;
            ++this->first;
            return true;
        }

        // parse function for ichar_range_('a','z')
        template<typename Arg1, typename Arg2>
        bool operator()(proto::tag::function, ianychar_range_p, Arg1 const &arg1, Arg2 const &arg2)
        {
            BOOST_ASSERT(proto::value(arg1) <= proto::value(arg2));
            this->skip();
            if(this->first == this->second
              || !utility::in_irange(*this->first, proto::value(arg1), proto::value(arg2)))
                return false;
            ++this->first;
            return true;
        }

        // parse function for complemented thingies (where thingies are assumed
        // to be 1 character wide).
        template<typename Expr>
        bool operator()(proto::tag::complement, Expr const &expr)
        {
            this->skip();
            iterator where = this->first;
            if(proto::eval(expr, *this))
                return this->first = where, false;
            this->first = ++where;
            return true;
        }

        // never_p parse function always returns false.
        bool operator()(proto::tag::terminal, never_tag)
        {
            return false;
        }

        // for A >> B, succeeds if A and B matches.
        template<typename Left, typename Right>
        bool operator()(proto::tag::shift_right, Left const &left, Right const &right)
        {
            return proto::eval(left, *this) && proto::eval(right, *this);
        }

        // for A | B, succeeds if either A or B matches at this point.
        template<typename Left, typename Right>
        bool operator()(proto::tag::bitwise_or, Left const &left, Right const &right)
        {
            iterator where = this->first;
            return proto::eval(left, *this) || proto::eval(right, this->reset(where));
        }

        // for *A, greedily match A as many times as possible.
        template<typename Expr>
        bool operator()(proto::tag::dereference, Expr const &expr)
        {
            iterator where = this->first;
            while(proto::eval(expr, *this))
                where = this->first;
            // make sure that when we return true, the iterator is at the correct position!
            this->first = where;
            return true;
        }

        // for +A, greedily match A one or more times.
        template<typename Expr>
        bool operator()(proto::tag::unary_plus, Expr const &expr)
        {
            return proto::eval(expr, *this) && proto::eval(*expr, *this);
        }

        // for !A, optionally match A.
        template<typename Expr>
        bool operator()(proto::tag::logical_not, Expr const &expr)
        {
            iterator where = this->first;
            if(!proto::eval(expr, *this))
                this->first = where;
            return true;
        }

        // for (A - B), matches when A but not B matches.
        template<typename Left, typename Right>
        bool operator()(proto::tag::minus, Left const &left, Right const &right)
        {
            iterator where = this->first;
            return !proto::eval(right, *this) && proto::eval(left, this->reset(where));
        }
    private:
        spirit_context &reset(iterator where)
        {
            this->first = where;
            return *this;
        }

        void skip()
        {
            if(!this->in_skip_)
            {
                this->in_skip_ = true;
                while(proto::eval(this->skip_, *this))
                {}
                this->in_skip_ = false;
            }
        }

        Skipper skip_;
        bool in_skip_;
    };

    struct as_ichar_parser : proto::callable
    {
        typedef proto::function<
            ianychar_p
          , proto::terminal<char>::type
          , proto::terminal<char>::type
        >::type result_type;

        template<typename Expr>
        result_type operator()(Expr const &expr) const
        {
            char lo = std::tolower(proto::value(proto::child_c<1>(expr)));
            char hi = std::toupper(proto::value(proto::child_c<1>(expr)));
            result_type that = {ichar_, {lo}, {hi}};
            return that;
        }
    };

    struct as_ichar_range_parser : proto::callable
    {
        typedef proto::function<
            ianychar_range_p
          , proto::terminal<char>::type
          , proto::terminal<char>::type
        >::type result_type;

        template<typename Expr>
        result_type operator()(Expr const &expr) const
        {
            char lo = proto::value(proto::child_c<1>(expr));
            char hi = proto::value(proto::child_c<2>(expr));
            result_type that = {ichar_range_, {lo}, {hi}};
            return that;
        }
    };

    struct as_ichar_literal : proto::callable
    {
        typedef proto::function<
            ianychar_p
          , proto::terminal<char>::type
          , proto::terminal<char>::type
        >::type result_type;

        template<typename Expr>
        result_type operator()(Expr const &expr) const
        {
            char lo = std::tolower(proto::value(expr));
            char hi = std::toupper(proto::value(expr));
            result_type that = {ichar_, {lo}, {hi}};
            return that;
        }
    };

    struct as_intbs_literal : proto::callable
    {
        typedef proto::function<
            ianystr_p
          , proto::terminal<std::string>::type
        >::type result_type;

        template<typename Expr>
        result_type operator()(Expr const &expr) const
        {
            result_type that = {istr_, {utility::to_istr(proto::value(expr))}};
            return that;
        }
    };

    struct as_istdstring_literal : proto::callable
    {
        typedef proto::function<
            ianystr_p
          , proto::terminal<std::string>::type
        >::type result_type;

        template<typename Expr>
        result_type operator()(Expr const &expr) const
        {
            result_type that = {istr_, {utility::to_istr(proto::value(expr).c_str())}};
            return that;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Transforms
    ///////////////////////////////////////////////////////////////////////////

    struct skip_primitives : proto::transform<skip_primitives>
    {
        template<typename Expr, typename State, typename Data>
        struct impl : proto::transform_impl<Expr, State, Data>
        {
            typedef
                typename proto::shift_right<
                    typename proto::dereference<State>::type
                  , Expr
                >::type
            result_type;

            result_type operator ()(
                typename impl::expr_param expr
              , typename impl::state_param state
              , typename impl::data_param data
            ) const
            {
                result_type that = {{state}, expr};
                return that;
            }
        };
    };

    ///////////////////////////////////////////////////////////////////////////
    // Grammar
    ///////////////////////////////////////////////////////////////////////////
    using proto::_;

    struct SpiritGrammar;

    struct SpiritCaseSensitivePrimitives
      : proto::or_<
            proto::when<CharParser, as_ichar_parser(_)>
          , proto::when<CharLiteral, as_ichar_literal(_)>
          , proto::when<NTBSLiteral, as_intbs_literal(_)>
          , proto::when<CharRangeParser, as_ichar_range_parser(_)>
          , proto::when<StdStringLiteral, as_istdstring_literal(_)>
        >
    {};

    struct SpiritCaseInsensitivePrimitives
      : proto::or_<
            anychar_p
          , IStrParser
          , ICharParser
          , ICharRangeParser
          , proto::complement<SpiritPrimitives>
        >
    {};

    struct SpiritPrimitives
      : proto::or_<
            SpiritCaseSensitivePrimitives
          , SpiritCaseInsensitivePrimitives
        >
    {};

    template<typename Grammar>
    struct SpiritComposites
      : proto::or_<
            proto::bitwise_or< Grammar, Grammar >
          , proto::shift_right< Grammar, Grammar >
          , proto::minus< Grammar, Grammar >
          , proto::dereference< Grammar >
          , proto::unary_plus< Grammar >
          , proto::logical_not< Grammar >
        >
    {};

    // Regular Spirit grammar, has no-case transforms
    struct SpiritGrammar
      : proto::or_<
            SpiritComposites<SpiritGrammar>
          , SpiritPrimitives
        >
    {};

    // Spirit grammar with the skipper transform
    struct SkipperGrammar
      : proto::or_<
            SpiritComposites<SkipperGrammar>
          , proto::when<SpiritPrimitives, skip_primitives>
        >
    {};

    ///////////////////////////////////////////////////////////////////////////
    // Directives
    ///////////////////////////////////////////////////////////////////////////

    struct no_case_directive
    {
        template<typename Expr>
        typename boost::result_of<SpiritGrammar(Expr const &)>::type const
        operator [](Expr const &expr) const
        {
            return SpiritGrammar()(expr);
        }
    };

    // no_case
    no_case_directive const no_case = {};

    template<typename Skipper>
    struct skip_directive
    {
        skip_directive(Skipper const &skip)
          : skip_(skip)
        {}

        template<typename Expr>
        typename boost::result_of<SkipperGrammar(Expr const &, Skipper const &)>::type const
        operator [](Expr const &expr) const
        {
            return SkipperGrammar()(expr, this->skip_);
        }
    private:
        Skipper skip_;
    };

    // skip
    template<typename Skipper>
    skip_directive<Skipper> skip(Skipper const &skip)
    {
        return skip_directive<Skipper>(skip);
    }

    ///////////////////////////////////////////////////////////////////////////
    // parse
    ///////////////////////////////////////////////////////////////////////////

    template<typename FwdIter, typename Rule>
    bool parse(FwdIter begin, FwdIter end, Rule const &rule)
    {
        // make sure the rule corresponds to the Spirit grammar:
        BOOST_MPL_ASSERT((proto::matches<Rule, SpiritGrammar>));

        spirit_context<FwdIter> ctx(begin, end);
        return proto::eval(rule, ctx);
    }

    // parse with a skip parser can be implemented in one of two ways:
    // Method 1)
    //      The skip parser is passed to all the parsers which invoke it
    //      before they invoke themselves. This is how Spirit-1 does it,
    //      and it is the cause of the Scanner Business. However, it has
    //      the advantage of not needing a parser transformation phase.
    // Method 2)
    //      Transform the expression template to insert the skip parser
    //      in between all sequenced parsers. That is, transform (A >> B)
    //      to (*skip >> A >> *skip >> B). This has the advantage of making
    //      it unnecessary to pass the scanner to all the parsers, which
    //      means its type doesn't show up in function signatures, avoiding
    //      the Scanner Business.
    // Recommendation:
    //      Both methods should be supported. Method 1 should be preferred
    //      when calling parse with parsers defined inline. Method 2 should
    //      be preferred when a parser expression is assigned to a rule<>,
    //      thereby making the type of the rule<> independent of the skip
    //      parser used. I imagine a syntax like:
    //          rule<> r = skip(space)[A >> B >> C]
    template<typename FwdIter, typename Rule, typename Skipper>
    bool parse(FwdIter begin, FwdIter end, Rule const &rule, Skipper const &skipper)
    {
        // make sure the rule corresponds to the Spirit grammar:
        BOOST_MPL_ASSERT((proto::matches<Rule, SpiritGrammar>));

        //// Method 1: pass skip parser in the context structure.
        //spirit_context<FwdIter, Skipper> ctx(begin, end, skipper);
        //return proto::eval(rule, ctx);

        // Method 2: Embed skip parser via tree transformation.
        spirit_context<FwdIter> ctx(begin, end);
        return proto::eval(spirit2::skip(skipper)[rule], ctx);
    }

}}

using namespace boost;
using namespace spirit2;

void test_toy_spirit()
{
    std::string str("abcd");

    // This will fail:
    BOOST_CHECK(!spirit2::parse(str.begin(), str.end()
                   , char_ >> char_('a')));

    // This will succeed:
    BOOST_CHECK(spirit2::parse(str.begin(), str.end()
                   , char_ >> char_('b') >> char_ >> 'd'));

    // This will succeed:
    BOOST_CHECK(spirit2::parse(str.begin(), str.end()
                   , 'a' >> ('c' >> char_ | 'b' >> char_('d') | 'b' >> char_('c')) >> 'd'));

    // This will succeed:
    BOOST_CHECK(spirit2::parse(str.begin(), str.end()
                   , *(char_ - 'd')));

    // This will succeed:
    BOOST_CHECK(spirit2::parse(str.begin(), str.end()
                   , no_case[char_('A') >> 'B' >> "CD"]));

    // This will succeed:
    BOOST_CHECK(spirit2::parse(str.begin(), str.end()
                   , no_case[*char_('A','Z')]));

    literal<char> a = lit('a');
    literal<char const *> bcd = lit("bcd");

    // This will succeed:
    BOOST_CHECK(spirit2::parse(str.begin(), str.end()
                   , +~~a >> no_case[bcd]));

    // Scanner Business: R.I.P. :-)
    str = "a  b    cd";
    BOOST_CHECK(spirit2::parse(str.begin(), str.end()
                   , char_('a') >> 'b' >> 'c' >> 'd', space >> space));

}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test proto and and toy spirit-2");

    test->add(BOOST_TEST_CASE(&test_toy_spirit));

    return test;
}

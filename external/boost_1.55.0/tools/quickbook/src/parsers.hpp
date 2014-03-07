/*=============================================================================
    Copyright (c) 2010-2011 Daniel James
    Copyright (c) 2003 Martin Wille
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 =============================================================================*/

// Some custom parsers for use in quickbook.
 
#ifndef BOOST_QUICKBOOK_PARSERS_HPP
#define BOOST_QUICKBOOK_PARSERS_HPP

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_nil.hpp>
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_tuples.hpp>
#include <boost/spirit/include/phoenix1_binders.hpp>
#include "fwd.hpp"

namespace quickbook {
    namespace cl = boost::spirit::classic;
    
    ///////////////////////////////////////////////////////////////////////////
    //
    // scoped_parser<Impl>
    //
    // Impl is a struct with the methods:
    //
    // void start();
    // void success(parse_iterator, parse_iterator);
    // void failure();
    // void cleanup();
    //
    ///////////////////////////////////////////////////////////////////////////

    template <typename Impl, typename Arguments, typename ParserT>
    struct scoped_parser_impl
        : public cl::unary< ParserT, cl::parser< scoped_parser_impl<Impl, Arguments, ParserT> > >
    {
        typedef scoped_parser_impl<Impl, Arguments, ParserT> self_t;
        typedef cl::unary< ParserT, cl::parser< scoped_parser_impl<Impl, Arguments, ParserT> > > base_t;

        template <typename ScannerT>
        struct result { typedef cl::match<> type; };

        scoped_parser_impl(
                Impl const& impl,
                Arguments const& arguments,
                ParserT const &p)
            : base_t(p)
            , impl_(impl)
            , arguments_(arguments)
        {}

        struct scoped
        {
            explicit scoped(Impl const& impl)
                : impl_(impl)
                , in_progress_(false)
            {}
            
            typedef phoenix::tuple_index<0> t0;
            typedef phoenix::tuple_index<1> t1;
            
            bool start(phoenix::tuple<> const&)
            {
                in_progress_ = impl_.start();
                return in_progress_;
            }
            
            template <typename Arg1>
            bool start(phoenix::tuple<Arg1> const& x)
            {
                in_progress_ = phoenix::bind(&Impl::start)(phoenix::var(impl_), x[t0()])();
                return in_progress_;
            }

            template <typename Arg1, typename Arg2>
            bool start(phoenix::tuple<Arg1, Arg2> const& x)
            {
                in_progress_ = phoenix::bind(&Impl::start)(phoenix::var(impl_), x[t0()], x[t1()])();
                return in_progress_;
            }
            
            void success(parse_iterator f, parse_iterator l)
            {
                in_progress_ = false;
                impl_.success(f, l);
            }

            void failure()
            {
                in_progress_ = false;
                impl_.failure();
            }

            ~scoped()
            {
                if (in_progress_) impl_.failure();
                impl_.cleanup();
            }
            
            Impl impl_;
            bool in_progress_;
        };
    
        template <typename ScannerT>
        typename result<ScannerT>::type parse(ScannerT const &scan) const
        {
            typedef typename ScannerT::iterator_t iterator_t;
            iterator_t save = scan.first;

            scoped scope(impl_);
            if (!scope.start(arguments_))
                return scan.no_match();

            typename cl::parser_result<ParserT, ScannerT>::type result
                = this->subject().parse(scan);

            bool success = scope.impl_.result(result, scan);

            if (success) {
                scope.success(save, scan.first);

                if (result) {
                    return scan.create_match(result.length(), cl::nil_t(), save, scan.first);
                }
                else {
                    return scan.create_match(scan.first.base() - save.base(), cl::nil_t(), save, scan.first);
                }
            }
            else {
                scope.failure();
                return scan.no_match();
            }
        }
        
        Impl impl_;
        Arguments arguments_;
    };

    template <typename Impl, typename Arguments>
    struct scoped_parser_gen
    {    
        explicit scoped_parser_gen(Impl impl, Arguments const& arguments)
            : impl_(impl), arguments_(arguments) {}

        template<typename ParserT>
        scoped_parser_impl
        <
            Impl,
            Arguments,
            typename cl::as_parser<ParserT>::type
        >
        operator[](ParserT const &p) const
        {
            typedef cl::as_parser<ParserT> as_parser_t;
            typedef typename as_parser_t::type parser_t;

            return scoped_parser_impl<Impl, Arguments, parser_t>
                (impl_, arguments_, p);
        }

        Impl impl_;
        Arguments arguments_;
    };

    template <typename Impl>
    struct scoped_parser
    {
        scoped_parser(Impl const& impl)
            : impl_(impl) {}
    
        scoped_parser_gen<Impl, phoenix::tuple<> >
            operator()() const
        {
            typedef phoenix::tuple<> tuple;
            return scoped_parser_gen<Impl, tuple>(impl_, tuple());
        }

        template <typename Arg1>
        scoped_parser_gen<Impl, phoenix::tuple<Arg1> >
            operator()(Arg1 x1) const
        {
            typedef phoenix::tuple<Arg1> tuple;
            return scoped_parser_gen<Impl, tuple>(impl_, tuple(x1));
        }
    
        template <typename Arg1, typename Arg2>
        scoped_parser_gen<Impl, phoenix::tuple<Arg1, Arg2> >
            operator()(Arg1 x1, Arg2 x2) const
        {
            typedef phoenix::tuple<Arg1, Arg2> tuple;
            return scoped_parser_gen<Impl, tuple>(impl_, tuple(x1, x2));
        }
        
        Impl impl_;
    };
    
    ///////////////////////////////////////////////////////////////////////////
    //
    // Lookback parser
    //
    // usage: lookback[body]
    //
    // Requires that iterator has typedef 'lookback_range' and function
    // 'lookback' returning a 'lookback_range'.
    //
    ///////////////////////////////////////////////////////////////////////////

    template <typename ParserT>
    struct lookback_parser
        : public cl::unary< ParserT, cl::parser< lookback_parser<ParserT> > >
    {
        typedef lookback_parser<ParserT> self_t;
        typedef cl::unary< ParserT, cl::parser< lookback_parser<ParserT> > > base_t;

        template <typename ScannerT>
        struct result
        {
            typedef typename cl::parser_result<ParserT, ScannerT>::type type;
        };

        lookback_parser(ParserT const& p)
            : base_t(p)
        {}

        template <typename ScannerT>
        typename result<ScannerT>::type parse(ScannerT const &scan) const
        {
            typedef typename ScannerT::iterator_t::lookback_range::iterator iterator_t;
            typedef cl::scanner<iterator_t, typename ScannerT::policies_t> scanner_t;

            iterator_t begin = scan.first.lookback().begin();
            scanner_t lookback_scan(begin, scan.first.lookback().end(), scan);
            
            if (this->subject().parse(lookback_scan))
                return scan.empty_match();
            else
                return scan.no_match();
        }
    };
    
    struct lookback_gen
    {
        template <typename ParserT>
        lookback_parser<ParserT> operator[](ParserT const& p) const
        {
            return lookback_parser<ParserT>(p);
        }
    };
    
    lookback_gen const lookback = lookback_gen();
 
    ///////////////////////////////////////////////////////////////////////////
    //
    // UTF-8 code point
    //
    // Very crude, it doesn't check that the code point is in any way valid.
    // Just looks for the beginning of the next character. This is just for
    // implementing some crude fixes, rather than full unicode support. I'm
    // sure experts would be appalled.
    //
    ///////////////////////////////////////////////////////////////////////////

    struct u8_codepoint_parser : public cl::parser<u8_codepoint_parser>
    {
        typedef u8_codepoint_parser self_t;

        template <typename Scanner>
        struct result
        {
            typedef cl::match<> type;
        };

        template <typename Scanner>
        typename result<Scanner>::type parse(Scanner const& scan) const
        {
            typedef typename Scanner::iterator_t iterator_t;

            if (scan.at_end()) return scan.no_match();

            iterator_t save(scan.first);

            do {
                ++scan.first;
            } while (!scan.at_end() &&
                    ((unsigned char) *scan.first & 0xc0) == 0x80);

            return scan.create_match(scan.first.base() - save.base(),
                    cl::nil_t(), save, scan.first);
        }
    };
  
    u8_codepoint_parser const u8_codepoint_p = u8_codepoint_parser();
}

#endif // BOOST_QUICKBOOK_SCOPED_BLOCK_HPP

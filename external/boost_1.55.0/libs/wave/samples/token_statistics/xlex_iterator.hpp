/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Definition of the lexer iterator for the xpressive lexer
    
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(XLEX_ITERATOR_HPP)
#define XLEX_ITERATOR_HPP

#include <string>
#include <iostream>

#include <boost/assert.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>

#include <boost/wave/language_support.hpp>
#include <boost/wave/util/file_position.hpp>
#include <boost/wave/util/functor_input.hpp>

#include "xlex_interface.hpp"

#if 0 != __COMO_VERSION__ || !BOOST_WORKAROUND(BOOST_MSVC, <= 1310)
#define BOOST_WAVE_EOF_PREFIX static
#else
#define BOOST_WAVE_EOF_PREFIX 
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace cpplexer {
namespace xlex {
namespace impl {

///////////////////////////////////////////////////////////////////////////////
//  
//  lex_iterator_functor_shim
//
///////////////////////////////////////////////////////////////////////////////

template <typename TokenT> 
class xlex_iterator_functor_shim 
{
    typedef typename TokenT::position_type  position_type;

public:
    xlex_iterator_functor_shim()
#if /*0 != __DECCXX_VER || */defined(__PGI)
      : eof()
#endif // 0 != __DECCXX_VER
    {}

// interface to the boost::spirit::classic::iterator_policies::functor_input 
// policy
    typedef TokenT result_type;

    BOOST_WAVE_EOF_PREFIX result_type const eof;
    typedef xlex_iterator_functor_shim unique;
    typedef lex_input_interface<TokenT>* shared;

    template <typename MultiPass>
    static result_type& get_next(MultiPass& mp, result_type& result)
    { 
        return mp.shared()->ftor->get(result); 
    }

    // this will be called whenever the last reference to a multi_pass will
    // be released
    template <typename MultiPass>
    static void destroy(MultiPass& mp)
    { 
        delete mp.shared()->ftor; 
    }

    template <typename MultiPass>
    static void set_position(MultiPass& mp, position_type const &pos)
    {
        mp.shared()->ftor->set_position(pos);
    }

#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
    template <typename MultiPass>
    static bool has_include_guards(MultiPass& mp, std::string& guard_name) 
    {
        return mp.shared()->ftor->has_include_guards(guard_name);
    }
#endif    

private:
    boost::shared_ptr<lex_input_interface<TokenT> > functor_ptr;
};

#if 0 != __COMO_VERSION__ || !BOOST_WORKAROUND(BOOST_MSVC, <= 1310)
///////////////////////////////////////////////////////////////////////////////
//  eof token
template <typename TokenT>
typename xlex_iterator_functor_shim<TokenT>::result_type const
   xlex_iterator_functor_shim<TokenT>::eof = 
       typename xlex_iterator_functor_shim<TokenT>::result_type();
#endif // 0 != __COMO_VERSION__

///////////////////////////////////////////////////////////////////////////////
}   // namespace impl

///////////////////////////////////////////////////////////////////////////////
//  
//  xlex_iterator
//
//      A generic C++ lexer interface class, which allows to plug in different
//      lexer implementations (template parameter LexT). The following 
//      requirement apply:
//
//          - the lexer type should have a function implemented, which returns
//            the next lexed token from the input stream:
//                typename LexT::token_type get();
//          - at the end of the input stream this function should return the
//            eof token equivalent
//          - the lexer should implement a constructor taking two iterators
//            pointing to the beginning and the end of the input stream and
//            a third parameter containing the name of the parsed input file,
//            the 4th parameter contains the information about the mode the 
//            preprocessor is used in (C99/C++ mode etc.)
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  Divide the given functor type into its components (unique and shared) 
//  and build a std::pair from these parts
template <typename FunctorData>
struct make_multi_pass
{
    typedef  
        std::pair<typename FunctorData::unique, typename FunctorData::shared> 
    functor_data_type;
    typedef typename FunctorData::result_type result_type;

    typedef boost::spirit::iterator_policies::split_functor_input input_policy;
    typedef boost::spirit::iterator_policies::ref_counted ownership_policy;
#if defined(BOOST_WAVE_DEBUG)
    typedef boost::spirit::iterator_policies::buf_id_check check_policy;
#else
    typedef boost::spirit::iterator_policies::no_check check_policy;
#endif
    typedef boost::spirit::iterator_policies::split_std_deque storage_policy;

    typedef boost::spirit::iterator_policies::default_policy<
            ownership_policy, check_policy, input_policy, storage_policy>
        policy_type;
    typedef boost::spirit::multi_pass<functor_data_type, policy_type> type;
};

///////////////////////////////////////////////////////////////////////////////
template <typename TokenT>
class xlex_iterator 
:   public make_multi_pass<impl::xlex_iterator_functor_shim<TokenT> >::type
{
    typedef impl::xlex_iterator_functor_shim<TokenT> input_policy_type;

    typedef typename make_multi_pass<input_policy_type>::type base_type;
    typedef typename make_multi_pass<input_policy_type>::functor_data_type 
        functor_data_type;

    typedef typename input_policy_type::unique unique_functor_type;
    typedef typename input_policy_type::shared shared_functor_type;

public:
    typedef TokenT token_type;

    xlex_iterator()
    {}

    template <typename IteratorT>
    xlex_iterator(IteratorT const &first, IteratorT const &last, 
            typename TokenT::position_type const &pos, 
            boost::wave::language_support language)
    :   base_type(
            functor_data_type(
                unique_functor_type(),
                xlex_input_interface<TokenT>
                    ::new_lexer(first, last, pos, language)
            )
        )
    {}

    void set_position(typename TokenT::position_type const &pos)
    {
        typedef typename token_type::position_type position_type;

    // set the new position in the current token
    token_type const& currtoken = this->base_type::dereference(*this);
    position_type currpos = currtoken.get_position();

        currpos.set_file(pos.get_file());
        currpos.set_line(pos.get_line());
        const_cast<token_type&>(currtoken).set_position(currpos);

    // set the new position for future tokens as well
        if (token_type::string_type::npos != 
            currtoken.get_value().find_first_of('\n'))
        {
            currpos.set_line(pos.get_line() + 1);
        }
        unique_functor_type::set_position(*this, currpos);
    }

#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
    // return, whether the current file has include guards
    // this function returns meaningful results only if the file was scanned 
    // completely
    bool has_include_guards(std::string& guard_name) const
    {
        return unique_functor_type::has_include_guards(*this, guard_name);
    }
#endif
};

///////////////////////////////////////////////////////////////////////////////
}   // namespace xlex
}   // namespace cpplexer
}   // namespace wave
}   // namespace boost

#undef BOOST_WAVE_EOF_PREFIX

#endif // !defined(XLEX_ITERATOR_HPP)

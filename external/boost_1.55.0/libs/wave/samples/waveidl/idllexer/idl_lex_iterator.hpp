/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Sample: Re2C based IDL lexer
            Definition of the lexer iterator 
    
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(IDL_LEX_ITERATOR_HPP_7926F865_E02F_4950_9EB5_5F453C9FF953_INCLUDED)
#define IDL_LEX_ITERATOR_HPP_7926F865_E02F_4950_9EB5_5F453C9FF953_INCLUDED

#include <string>
#include <iostream>

#include <boost/assert.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>

#include <boost/wave/language_support.hpp>
#include <boost/wave/util/file_position.hpp>
#include <boost/wave/util/functor_input.hpp>

#include "idl_lex_interface.hpp"

#if 0 != __COMO_VERSION__ || !BOOST_WORKAROUND(BOOST_MSVC, <= 1310)
#define BOOST_WAVE_EOF_PREFIX static
#else
#define BOOST_WAVE_EOF_PREFIX 
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace idllexer {
namespace impl {

///////////////////////////////////////////////////////////////////////////////
//  
//  lex_iterator_functor_shim
//
///////////////////////////////////////////////////////////////////////////////

template <typename TokenT> 
class lex_iterator_functor_shim 
{
    typedef typename TokenT::position_type  position_type;

public:
    lex_iterator_functor_shim()
#if /*0 != __DECCXX_VER || */defined(__PGI)
      : eof()
#endif // 0 != __DECCXX_VER
    {}

// interface to the boost::spirit::classic::iterator_policies::functor_input 
// policy
    typedef TokenT result_type;

    BOOST_WAVE_EOF_PREFIX result_type const eof;
    typedef lex_iterator_functor_shim unique;
    typedef cpplexer::lex_input_interface<TokenT>* shared;

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

private:
    boost::shared_ptr<cpplexer::lex_input_interface<TokenT> > functor_ptr;
};

#if 0 != __COMO_VERSION__ || !BOOST_WORKAROUND(BOOST_MSVC, <= 1310)
///////////////////////////////////////////////////////////////////////////////
//  eof token
template <typename TokenT>
typename lex_iterator_functor_shim<TokenT>::result_type const
    lex_iterator_functor_shim<TokenT>::eof =
        typename lex_iterator_functor_shim<TokenT>::result_type();
#endif // 0 != __COMO_VERSION__

///////////////////////////////////////////////////////////////////////////////
}   // namespace impl

///////////////////////////////////////////////////////////////////////////////
//  
//  lex_iterator
//
//      A generic C++ lexer interface class, which allows to plug in different
//      lexer implementations (template parameter LexT). The following 
//      requirement apply:
//
//          - the lexer type should have a function implemented, which returnes
//            the next lexed token from the input stream:
//                typename LexT::token_type get();
//          - at the end of the input stream this function should return the
//            eof token equivalent
//          - the lexer should implement a constructor taking two iterators
//            pointing to the beginning and the end of the input stream and
//            a third parameter containing the name of the parsed input file 
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
class lex_iterator 
:   public make_multi_pass<impl::lex_iterator_functor_shim<TokenT> >::type
{
    typedef impl::lex_iterator_functor_shim<TokenT> input_policy_type;

    typedef typename make_multi_pass<input_policy_type>::type base_type;
    typedef typename make_multi_pass<input_policy_type>::functor_data_type 
        functor_data_type;

    typedef typename input_policy_type::unique unique_functor_type;
    typedef typename input_policy_type::shared shared_functor_type;

public:
    typedef TokenT token_type;

    lex_iterator()
    {}

    template <typename IteratorT>
    lex_iterator(IteratorT const &first, IteratorT const &last, 
            typename TokenT::position_type const &pos, 
            boost::wave::language_support language)
    :   base_type(
            functor_data_type(
                unique_functor_type(),
                cpplexer::lex_input_interface_generator<TokenT>
                    ::new_lexer(first, last, pos, language)
            )
        )
    {}

    void set_position(typename TokenT::position_type const &pos)
    {
        typedef typename TokenT::position_type position_type;

        // set the new position in the current token
        token_type const& currtoken = this->base_type::dereference(*this);
        position_type currpos = currtoken.get_position();
        currpos.set_file(pos.get_file());
        currpos.set_line(pos.get_line());
        const_cast<token_type&>(currtoken).set_position(currpos);

        // set the new position for future tokens as well
        unique_functor_type::set_position(*this, currpos);
    }

#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
    // this sample does no include guard detection
    bool has_include_guards(std::string&) const { return false; }
#endif    
};

///////////////////////////////////////////////////////////////////////////////
}   // namespace idllexer
}   // namespace wave
}   // namespace boost

#undef BOOST_WAVE_EOF_PREFIX

#endif // !defined(IDL_LEX_ITERATOR_HPP_7926F865_E02F_4950_9EB5_5F453C9FF953_INCLUDED)

//  Copyright (c) 2010 Larry Evans
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//Purpose:
//  Demonstrate error in non-classic multi_pass iterator compilation.
//

#include <boost/spirit/home/qi.hpp>
#include <boost/spirit/home/support.hpp>
#include <boost/spirit/home/support/multi_pass.hpp>
#include <boost/spirit/home/support/iterators/detail/functor_input_policy.hpp>

#include <fstream>

//[iterate_a2m:
// copied from:
//    http://www.boost.org/doc/libs/1_41_0/libs/spirit/doc/html/spirit/support/multi_pass.html

// define the function object
template<typename CharT=char>
class istreambuf_functor
{
public:
        typedef 
      std::istreambuf_iterator<CharT> 
    buf_iterator_type;
        typedef 
      typename buf_iterator_type::int_type
    result_type;
        static 
      result_type 
    eof;

    istreambuf_functor(void)
      : current_chr(eof)
    {}

    istreambuf_functor(std::ifstream& input) 
      : my_first(input)
      , current_chr(eof)
    {}

    result_type operator()()
    {
        buf_iterator_type last;
        if (my_first == last)
        {
            return eof;
        }
        current_chr=*my_first;
        ++my_first;
        return current_chr;
    }

private:
    buf_iterator_type my_first;
    result_type current_chr;
};

template<typename CharT>
  typename istreambuf_functor<CharT>::result_type 
  istreambuf_functor<CharT>::
eof
( istreambuf_functor<CharT>::buf_iterator_type::traits_type::eof()
)
;

//]iterate_a2m:

typedef istreambuf_functor<char> base_iterator_type;

typedef
  boost::spirit::multi_pass
  < base_iterator_type
  , boost::spirit::iterator_policies::default_policy
    < boost::spirit::iterator_policies::first_owner
    , boost::spirit::iterator_policies::no_check
    , boost::spirit::iterator_policies::functor_input
    , boost::spirit::iterator_policies::split_std_deque
    >
  > 
chr_iterator_type;

// ======================================================================       
// Main                                                                         
int main() 
{
    std::ifstream in("multi_pass.txt");

    unsigned num_toks=0;
    unsigned const max_toks=10;

    base_iterator_type base_first(in);
    chr_iterator_type chr_first(base_first);
    chr_iterator_type chr_last;
    for
      (
      ; (chr_first != chr_last && ++num_toks < max_toks)
      ; ++chr_first
      )
    {
        std::cout<<":num_toks="<<num_toks<<":chr="<<*chr_first<<"\n";
    }
    return 0;
}    

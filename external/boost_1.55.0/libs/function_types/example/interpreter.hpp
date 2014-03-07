
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------
//
// This example implements a simple batch-style interpreter that is capable of
// calling functions previously registered with it. The parameter types of the
// functions are used to control the parsing of the input.
//
// Implementation description
// ==========================
//
// When a function is registered, an 'invoker' template is instantiated with
// the function's type. The 'invoker' fetches a value from the 'token_parser'
// for each parameter of the function into a tuple and finally invokes the the
// function with these values as arguments. The invoker's entrypoint, which
// is a function of the callable builtin that describes the function to call and
// a reference to the 'token_parser', is partially bound to the registered
// function and put into a map so it can be found by name during parsing.

#include <map>
#include <string>
#include <stdexcept>

#include <boost/token_iterator.hpp>
#include <boost/token_functions.hpp>

#include <boost/lexical_cast.hpp>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>

#include <boost/fusion/include/push_back.hpp>
#include <boost/fusion/include/cons.hpp>
#include <boost/fusion/include/invoke.hpp>

#include <boost/mpl/begin.hpp>
#include <boost/mpl/end.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/deref.hpp>

#include <boost/utility/enable_if.hpp>

#include <boost/function_types/is_nonmember_callable_builtin.hpp>
#include <boost/function_types/parameter_types.hpp>

namespace example
{
  namespace fusion = boost::fusion;
  namespace ft = boost::function_types;
  namespace mpl = boost::mpl;

  class interpreter
  {
    class token_parser;
    typedef boost::function<void(token_parser &)> invoker_function;
    typedef std::map<std::string, invoker_function> dictionary;

    dictionary map_invokers;
  public:
    // Registers a function with the interpreter.
    template<typename Function>
    typename boost::enable_if< ft::is_nonmember_callable_builtin<Function>
    >::type register_function(std::string const & name, Function f);

    // Parse input for functions to call.
    void parse_input(std::string const & text) const;

  private:
    template< typename Function
    , class From = typename mpl::begin< ft::parameter_types<Function> >::type
    , class To   = typename mpl::end< ft::parameter_types<Function> >::type
    >
    struct invoker;
  };

  class interpreter::token_parser
  {
    typedef boost::token_iterator_generator<
        boost::char_separator<char> >::type token_iterator;

    token_iterator itr_at, itr_to;
  public:

    token_parser(token_iterator from, token_iterator to)
      : itr_at(from), itr_to(to)
    { }

  private:
    template<typename T>
    struct remove_cv_ref
      : boost::remove_cv< typename boost::remove_reference<T>::type >
    { };
  public:
    // Returns a token of given type.
    // We just apply boost::lexical_cast to whitespace separated string tokens
    // for simplicity.
    template<typename RequestedType>
    typename remove_cv_ref<RequestedType>::type get()
    {
      if (! this->has_more_tokens())
        throw std::runtime_error("unexpected end of input");

      try
      {
        typedef typename remove_cv_ref<RequestedType>::type result_type;
        result_type result = boost::lexical_cast
            <typename remove_cv_ref<result_type>::type>(*this->itr_at);
        ++this->itr_at;
        return result;
      }

      catch (boost::bad_lexical_cast &)
      { throw std::runtime_error("invalid argument: " + *this->itr_at); }
    }

    // Any more tokens?
    bool has_more_tokens() const { return this->itr_at != this->itr_to; }
  };

  template<typename Function, class From, class To>
  struct interpreter::invoker
  {
    // add an argument to a Fusion cons-list for each parameter type
    template<typename Args>
    static inline
    void apply(Function func, token_parser & parser, Args const & args)
    {
      typedef typename mpl::deref<From>::type arg_type;
      typedef typename mpl::next<From>::type next_iter_type;

      interpreter::invoker<Function, next_iter_type, To>::apply
          ( func, parser, fusion::push_back(args, parser.get<arg_type>()) );
    }
  };

  template<typename Function, class To>
  struct interpreter::invoker<Function,To,To>
  {
    // the argument list is complete, now call the function
    template<typename Args>
    static inline
    void apply(Function func, token_parser &, Args const & args)
    {
      fusion::invoke(func,args);
    }
  };

  template<typename Function>
  typename boost::enable_if< ft::is_nonmember_callable_builtin<Function> >::type
  interpreter::register_function(std::string const & name, Function f)
  {
    // instantiate and store the invoker by name
    this->map_invokers[name] = boost::bind(
        & invoker<Function>::template apply<fusion::nil>, f,_1,fusion::nil() );
  }


  void interpreter::parse_input(std::string const & text) const
  {
    boost::char_separator<char> s(" \t\n\r");

    token_parser parser
      ( boost::make_token_iterator<std::string>(text.begin(), text.end(), s)
      , boost::make_token_iterator<std::string>(text.end()  , text.end(), s) );

    while (parser.has_more_tokens())
    {
      // read function name
      std::string func_name = parser.get<std::string>();

      // look up function
      dictionary::const_iterator entry = map_invokers.find( func_name );
      if (entry == map_invokers.end())
        throw std::runtime_error("unknown function: " + func_name);

      // call the invoker which controls argument parsing
      entry->second(parser);
    }
  }

}


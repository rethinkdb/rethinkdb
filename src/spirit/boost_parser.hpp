#ifndef BOOST_PARSER_HPP_
#define BOOST_PARSER_HPP_

//#define BOOST_SPIRIT_DEBUG_PRINT_SOME 1

#include "errors.hpp"
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/classic_debug.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>

#include "arch/types.hpp"

namespace qi = boost::spirit::qi;
namespace phoenix = boost::phoenix;
namespace fusion = boost::fusion;
namespace ascii = boost::spirit::ascii;

#endif  // BOOST_PARSER_HPP_

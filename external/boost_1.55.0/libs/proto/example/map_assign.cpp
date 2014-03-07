//[ MapAssign
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This is a port of map_list_of() from the Boost.Assign library.
// It has the advantage of being more efficient at runtime by not
// building any temporary container that requires dynamic allocation.

#include <map>
#include <string>
#include <iostream>
#include <boost/proto/core.hpp>
#include <boost/proto/transform.hpp>
#include <boost/type_traits/add_reference.hpp>
namespace proto = boost::proto;
using proto::_;

struct map_list_of_tag
{};

// A simple callable function object that inserts a
// (key,value) pair into a map.
struct insert
  : proto::callable
{
    template<typename Sig>
    struct result;

    template<typename This, typename Map, typename Key, typename Value>
    struct result<This(Map, Key, Value)>
      : boost::add_reference<Map>
    {};

    template<typename Map, typename Key, typename Value>
    Map &operator()(Map &map, Key const &key, Value const &value) const
    {
        map.insert(typename Map::value_type(key, value));
        return map;
    }
};

// Work-arounds for Microsoft Visual C++ 7.1
#if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
#define MapListOf(x) proto::call<MapListOf(x)>
#define _value(x) call<proto::_value(x)>
#endif

// The grammar for valid map-list expressions, and a
// transform that populates the map.
struct MapListOf
  : proto::or_<
        proto::when<
            // map_list_of(a,b)
            proto::function<
                proto::terminal<map_list_of_tag>
              , proto::terminal<_>
              , proto::terminal<_>
            >
          , insert(
                proto::_data
              , proto::_value(proto::_child1)
              , proto::_value(proto::_child2)
            )
        >
      , proto::when<
            // map_list_of(a,b)(c,d)...
            proto::function<
                MapListOf
              , proto::terminal<_>
              , proto::terminal<_>
            >
          , insert(
                MapListOf(proto::_child0)
              , proto::_value(proto::_child1)
              , proto::_value(proto::_child2)
            )
        >
    >
{};

#if BOOST_WORKAROUND(BOOST_MSVC, == 1310)
#undef MapListOf
#undef _value
#endif

template<typename Expr>
struct map_list_of_expr;

struct map_list_of_dom
  : proto::domain<proto::pod_generator<map_list_of_expr>, MapListOf>
{};

// An expression wrapper that provides a conversion to a
// map that uses the MapListOf
template<typename Expr>
struct map_list_of_expr
{
    BOOST_PROTO_BASIC_EXTENDS(Expr, map_list_of_expr, map_list_of_dom)
    BOOST_PROTO_EXTENDS_FUNCTION()

    template<typename Key, typename Value, typename Cmp, typename Al>
    operator std::map<Key, Value, Cmp, Al> () const
    {
        BOOST_MPL_ASSERT((proto::matches<Expr, MapListOf>));
        std::map<Key, Value, Cmp, Al> map;
        return MapListOf()(*this, 0, map);
    }
};

map_list_of_expr<proto::terminal<map_list_of_tag>::type> const map_list_of = {{{}}};

int main()
{
    // Initialize a map:
    std::map<std::string, int> op =
        map_list_of
            ("<", 1)
            ("<=",2)
            (">", 3)
            (">=",4)
            ("=", 5)
            ("<>",6)
        ;

    std::cout << "\"<\"  --> " << op["<"] << std::endl;
    std::cout << "\"<=\" --> " << op["<="] << std::endl;
    std::cout << "\">\"  --> " << op[">"] << std::endl;
    std::cout << "\">=\" --> " << op[">="] << std::endl;
    std::cout << "\"=\"  --> " << op["="] << std::endl;
    std::cout << "\"<>\" --> " << op["<>"] << std::endl;

    return 0;
}
//]


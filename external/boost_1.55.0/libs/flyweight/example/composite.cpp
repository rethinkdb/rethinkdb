/* Boost.Flyweight example of a composite design.
 *
 * Copyright 2006-2010 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#include <boost/flyweight.hpp>
#include <boost/functional/hash.hpp>
#include <boost/tokenizer.hpp>
#include <boost/variant.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace boost::flyweights;

/* A node of a lisp-like list can be modeled as a boost::variant of
 *   1. A string (primitive node)
 *   2. A vector of nodes (embedded list)
 * To save space, 2 is stored as a vector of flyweights.
 * As is usual with recursive data structures, a node can be thought
 * of also as a list. To close the flyweight circle, the final
 * type list is a flyweight wrapper, so that the final structure can
 * be described as follows in BNF-like style:
 *
 *   list      ::= flyweight<list_impl>
 *   list_impl ::= std::string | std::vector<list>
 */

struct list_elems;

typedef boost::variant<
  std::string,
  boost::recursive_wrapper<list_elems>  
> list_impl;

struct list_elems:std::vector<flyweight<list_impl> >{};

typedef flyweight<list_impl> list;

/* list_impl must be hashable to be used by flyweight: If a
 * node is a std::string, its hash resolves to that of the string;
 * if it is a vector of nodes, we compute the hash by combining
 * the *addresses* of the stored flyweights' associated values: this is
 * consistent because flyweight equality implies equality of reference.
 * Using this trick instead of hashing the node values themselves
 * allow us to do the computation without recursively descending down
 * through the entire data structure.
 */

struct list_hasher:boost::static_visitor<std::size_t>
{
  std::size_t operator()(const std::string& str)const
  {
    boost::hash<std::string> h;
    return h(str);
  }

  std::size_t operator()(const list_elems& elms)const
  {
    std::size_t res=0;
    for(list_elems::const_iterator it=elms.begin(),it_end=elms.end();
        it!=it_end;++it){
      const list_impl* p=&it->get();
      boost::hash_combine(res,p);
    }
    return res;
  }
};

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
namespace boost{
#endif

std::size_t hash_value(const list_impl& limpl)
{
  return boost::apply_visitor(list_hasher(),limpl);
}

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
} /* namespace boost */
#endif

/* basic pretty printer with indentation according to the nesting level */

struct list_pretty_printer:boost::static_visitor<>
{
  list_pretty_printer():nest(0){}

  void operator()(const std::string& str)
  {
    indent();
    std::cout<<str<<"\n";
  }

  void operator()(const boost::recursive_wrapper<list_elems>& elmsw)
  {
    indent();
    std::cout<<"(\n";
    ++nest;
    const list_elems& elms=elmsw.get();
    for(list_elems::const_iterator it=elms.begin(),it_end=elms.end();
        it!=it_end;++it){
      boost::apply_visitor(*this,it->get());
    }
    --nest;
    indent();
    std::cout<<")\n";
  }

private:
  void indent()const
  {
    for(int i=nest;i--;)std::cout<<"  ";
  }

  int nest;
};

void pretty_print(const list& l)
{
  list_pretty_printer pp;
  boost::apply_visitor(pp,l.get());
}

/* list parser */

template<typename InputIterator>
list parse_list(InputIterator& first,InputIterator last,int nest)
{
  list_elems elms;
  while(first!=last){
    std::string str=*first++;
    if(str=="("){
      elms.push_back(parse_list(first,last,nest+1));   
    }
    else if(str==")"){
      if(nest==0)throw std::runtime_error("unmatched )");
      return list(elms);
    }
    else{
      elms.push_back(list(str)); 
    }
  }
  if(nest!=0)throw std::runtime_error("unmatched (");
  return list(elms);
}

list parse_list(const std::string str)
{
  typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
  tokenizer tok(str,boost::char_separator<char>(" ","()"));
  tokenizer::iterator begin=tok.begin();
  return parse_list(begin,tok.end(),0);
}

int main()
{
  std::cout<<"enter list: ";
  std::string str;
  std::getline(std::cin,str);
  try{
    pretty_print(parse_list(str));
  }
  catch(const std::exception& e){
    std::cout<<"error: "<<e.what()<<"\n";
  }

  return 0;
}

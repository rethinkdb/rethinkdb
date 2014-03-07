/* Boost.Flyweight example of flyweight-based formatted text processing.
 *
 * Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#include <boost/flyweight.hpp>
#include <boost/functional/hash.hpp>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{using ::exit;using ::tolower;}
#endif

using namespace boost::flyweights;

/* See the portability section of Boost.Hash at
 *   http://boost.org/doc/html/hash/portability.html
 * for an explanation of the ADL-related workarounds.
 */

namespace boost{
#if !defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
namespace flyweights{
#endif

/* We hash the various flyweight types used in the program hashing
 * a *pointer* to their contents: this is consistent as equality of
 * flyweights implies equality of references.
 */

template<typename T>
std::size_t hash_value(const flyweight<T>& x)
{
  boost::hash<const T*> h;
  return h(&x.get());
}

#if !defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
} /* namespace flyweights */
#endif
} /* namespace boost */

/* An HTML tag consists of a name and optional properties of the form
 * name1=value1 ... namen=valuen. We do not need to parse the properties
 * for the purposes of the program, hence they are all stored in
 * html_tag_data::properties in raw form.
 */

struct html_tag_data
{
  std::string name;
  std::string properties;
};

bool operator==(const html_tag_data& x,const html_tag_data& y)
{
  return x.name==y.name&&x.properties==y.properties;
}

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
namespace boost{
#endif

std::size_t hash_value(const html_tag_data& x)
{
  std::size_t res=0;
  boost::hash_combine(res,x.name);
  boost::hash_combine(res,x.properties);
  return res;
}

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
} /* namespace boost */
#endif

typedef flyweight<html_tag_data> html_tag;

/* parse_tag is passed an iterator positioned at the first char of
 * the tag after the opening '<' and returns, if succesful, a parsed tag
 * and whether it is opening (<xx>) or closing (</xx>).
 */

enum tag_type{opening,closing,failure};

struct parse_tag_res
{
  parse_tag_res(tag_type type_,const html_tag_data& tag_=html_tag_data()):
    type(type_),tag(tag_){}
  parse_tag_res(const parse_tag_res& x):type(x.type),tag(x.tag){}

  tag_type type;
  html_tag tag;
};

template<typename ForwardIterator>
parse_tag_res parse_tag(ForwardIterator& first,ForwardIterator last)
{
  html_tag_data  tag;
  std::string    buf;
  bool           in_quote=false;
  for(ForwardIterator it=first;it!=last;){
    char ch=*it++;
    if(ch=='>'&&!in_quote){             /* ignore '>'s if inside quotes */
      tag_type type;
      std::string::size_type
        bname=buf.find_first_not_of("\t\n\r "),
        ename=bname==std::string::npos?
          std::string::npos:
          buf.find_first_of("\t\n\r ",bname),
        bprop=ename==std::string::npos?
          std::string::npos:
          buf.find_first_not_of("\t\n\r ",ename);
      if(bname==ename){                 /* null name */
        return parse_tag_res(failure);
      }
      else if(buf[bname]=='/'){         /* closing tag */
        type=closing;
        ++bname;
      }
      else type=opening;
      tag.name=buf.substr(bname,ename-bname);      
      std::transform(                   /* normalize tag name to lower case */
        tag.name.begin(),tag.name.end(),tag.name.begin(),
        (int(*)(int))std::tolower);
      if(bprop!=std::string::npos){
        tag.properties=buf.substr(bprop,buf.size());
      }
      first=it;                         /* result good, consume the chars */
      return parse_tag_res(type,tag);      
    }
    else{
      if(ch=='"')in_quote=!in_quote;
      buf+=ch;
    }
  }
  return parse_tag_res(failure);        /* end reached and found no '>' */
}

/* A character context is just a vector containing the tags enclosing the
 * character, from the outermost level to the innermost.
 */

typedef std::vector<html_tag>        html_context_data;
typedef flyweight<html_context_data> html_context;

/* A character is a char code plus its context.
 */

struct character_data
{
  character_data(char code_=0,html_context context_=html_context()):
    code(code_),context(context_){}
  character_data(const character_data& x):code(x.code),context(x.context){}
    
  char         code;
  html_context context;
};

bool operator==(const character_data& x,const character_data& y)
{
  return x.code==y.code&&x.context==y.context;
}

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
namespace boost{
#endif

std::size_t hash_value(const character_data& x)
{
  std::size_t res=0;
  boost::hash_combine(res,x.code);
  boost::hash_combine(res,x.context);
  return res;
}

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
} /* namespace boost */
#endif

typedef flyweight<character_data> character;

/* scan_html converts HTML code into a stream of contextualized characters.
 */

template<typename ForwardIterator,typename OutputIterator>
void scan_html(ForwardIterator first,ForwardIterator last,OutputIterator out)
{
  html_context_data context;
  while(first!=last){
    if(*first=='<'){                                 /* tag found */
      ++first;
      parse_tag_res res=parse_tag(first,last);
      if(res.type==opening){                         /* add to contex */
        context.push_back(res.tag);
        continue;
      }
      else if(res.type==closing){                    /* remove from context */
        /* Pop all tags from the innermost to the matching one; this takes
         * care of missing </xx>s like vg. in <ul><li>hello</ul>.
         */

        for(html_context_data::reverse_iterator rit=context.rbegin();
            rit!=context.rend();++rit){
          if(rit->get().name==res.tag.get().name){
            context.erase(rit.base()-1,context.end());
            break;
          }
        }
        continue;
      }
    }
    *out++=character(*first++,html_context(context));
  }
}

/* HTML-producing utilities */

void print_opening_tag(std::ostream& os,const html_tag_data& x)
{
  os<<"<"<<x.name;
  if(!x.properties.empty())os<<" "<<x.properties;
  os<<">";
}

void print_closing_tag(std::ostream& os,const html_tag_data& x)
{
  /* SGML declarations (beginning with '!') are not closed */

  if(x.name[0]!='!')os<<"</"<<x.name<<">";
}

/* change_context takes contexts from and to with tags
 *
 *   from<- c1 ... cn fn+1 ... fm
 *   to  <- c1 ... cn tn+1 ... tk
 *
 * (that is, they share the first n tags, n might be 0), and
 * produces code closing fm ... fn+1 and opening tn+1 ... tk.
 */

template<typename OutputIterator>
void change_context(
  const html_context_data& from,const html_context_data& to,
  OutputIterator out)
{
  std::ostringstream oss;
  html_context_data::const_iterator
    it0=from.begin(),
    it0_end=from.end(),
    it1=to.begin(),
    it1_end=to.end();
  for(;it0!=it0_end&&it1!=it1_end&&*it0==*it1;++it0,++it1);
  while(it0_end!=it0)print_closing_tag(oss,*--it0_end);
  while(it1!=it1_end)print_opening_tag(oss,*it1++);
  std::string str=oss.str();
  std::copy(str.begin(),str.end(),out);
}

/* produce_html is passed a bunch of contextualized characters and emits
 * the corresponding HTML. The algorithm is simple: tags are opened and closed
 * as a result of the context from one character to the following changing.
 */

template<typename ForwardIterator,typename OutputIterator>
void produce_html(ForwardIterator first,ForwardIterator last,OutputIterator out)
{
  html_context context;
  while(first!=last){
    if(first->get().context!=context){
      change_context(context,first->get().context,out);
      context=first->get().context;
    }
    *out++=(first++)->get().code;
  }
  change_context(context,html_context(),out); /* close remaining context */
}

/* Without these explicit instantiations, MSVC++ 6.5/7.0 does not
 * find some friend operators in certain contexts.
 */      

character dummy1;
html_tag  dummy2;

int main()
{
  std::cout<<"input html file: ";
  std::string in;
  std::getline(std::cin,in);
  std::ifstream ifs(in.c_str());
  if(!ifs){
    std::cout<<"can't open "<<in<<std::endl;
    std::exit(EXIT_FAILURE);
  }
  typedef std::istreambuf_iterator<char> istrbuf_iterator;
  std::vector<char> html_source;
  std::copy(
    istrbuf_iterator(ifs),istrbuf_iterator(),
    std::back_inserter(html_source));

  /* parse the HTML */
  
  std::vector<character> scanned_html;
  scan_html(
    html_source.begin(),html_source.end(),std::back_inserter(scanned_html));

  /* Now that we have the text as a vector of contextualized characters,
   * we can shuffle it around and manipulate in almost any way we please.
   * For instance, the following reverses the central portion of the doc.
   */

  std::reverse(
    scanned_html.begin()+scanned_html.size()/4,
    scanned_html.begin()+3*(scanned_html.size()/4));

  /* emit the resulting HTML */

  std::cout<<"output html file: ";
  std::string out;
  std::getline(std::cin,out);
  std::ofstream ofs(out.c_str());
  if(!ofs){
    std::cout<<"can't open "<<out<<std::endl;
    std::exit(EXIT_FAILURE);
  }
  typedef std::ostreambuf_iterator<char> ostrbuf_iterator;
  produce_html(scanned_html.begin(),scanned_html.end(),ostrbuf_iterator(ofs));

  return 0;
}

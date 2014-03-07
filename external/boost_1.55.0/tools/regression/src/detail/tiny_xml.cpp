//  tiny XML sub-set tools implementation  -----------------------------------//

//  (C) Copyright Beman Dawes 2002.  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "tiny_xml.hpp"
#include <cassert>
#include <cstring>

namespace
{

  void eat_whitespace( char & c, std::istream & in )
  {
    while ( c == ' ' || c == '\r' || c == '\n' || c == '\t' )
      in.get( c );
  }

  std::string get_name( char & c, std::istream & in )
  {
    std::string result;
    eat_whitespace( c, in );
    while ( std::strchr(
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.", c )
      != 0 )
    {
      result += c;
      if(!in.get( c ))
        throw std::string("xml: unexpected eof");
    }
    return result;
  }

  void eat_delim( char & c, std::istream & in,
                  char delim, const std::string & msg )
  {
    eat_whitespace( c, in );
    if ( c != delim )
      throw std::string("xml syntax error, expected ") + delim
       + " (" + msg + ")";
    in.get( c );
  }

  std::string get_value( char & c, std::istream & in )
  {
    std::string result;
    while ( c != '\"' )
    {
      result += c;
      in.get( c );
    }
    in.get( c );
    return result;
  }

}

namespace boost
{
  namespace tiny_xml
  {

  //  parse  -----------------------------------------------------------------//

    element_ptr parse( std::istream & in, const std::string & msg )
    {
      char c = 0;  // current character
      element_ptr e( new element );

      if(!in.get( c ))
        throw std::string("xml: unexpected eof");
      if ( c == '<' )
        if(!in.get( c ))
          throw std::string("xml: unexpected eof");

      e->name = get_name( c, in );
      eat_whitespace( c, in );

      // attributes
      while ( c != '>' )
      {
        attribute a;
        a.name = get_name( c, in );

        eat_delim( c, in, '=', msg );
        eat_delim( c, in, '\"', msg );

        a.value = get_value( c, in );

        e->attributes.push_back( a );
        eat_whitespace( c, in );
      }
      if(!in.get( c )) // next after '>'
        throw std::string("xml: unexpected eof");

      eat_whitespace( c, in );

      // sub-elements
      while ( c == '<' )
      {
        if ( in.peek() == '/' ) break;
        e->elements.push_back( parse( in, msg ) );
        in.get( c ); // next after '>'
        eat_whitespace( c, in );
      }

      // content
      if ( c != '<' )
      {
        e->content += '\n';
        while ( c != '<' )
        {
          e->content += c;
          if(!in.get( c ))
            throw std::string("xml: unexpected eof");
        }
      }

      assert( c == '<' );
      if(!in.get( c )) // next after '<'
        throw std::string("xml: unexpected eof");

      eat_delim( c, in, '/', msg );
      std::string end_name( get_name( c, in ) );
      if ( e->name != end_name )
        throw std::string("xml syntax error: beginning name ")
          + e->name + " did not match end name " + end_name
          + " (" + msg + ")";

      eat_delim( c, in, '>', msg );
      return e;
    }

    //  write  ---------------------------------------------------------------//

    void write( const element & e, std::ostream & out )
    {
      out << "<" << e.name;
      if ( !e.attributes.empty() )
      {
        for( attribute_list::const_iterator itr = e.attributes.begin();
             itr != e.attributes.end(); ++itr )
        {
          out << " " << itr->name << "=\"" << itr->value << "\"";
        }
      }
      out << ">";
      if ( !e.elements.empty() )
      {
        out << "\n";
        for( element_list::const_iterator itr = e.elements.begin();
             itr != e.elements.end(); ++itr )
        {
          write( **itr, out );
        }
      }
      if ( !e.content.empty() )
      {
        out << e.content;
      }
      out << "</" << e.name << ">\n";
    }

  } // namespace tiny_xml
} // namespace boost


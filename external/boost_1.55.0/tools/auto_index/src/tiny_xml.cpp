//  tiny XML sub-set tools implementation  -----------------------------------//

//  (C) Copyright Beman Dawes 2002.  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "tiny_xml.hpp"
#include <cassert>
#include <cstring>

namespace
{

   inline void eat_whitespace( char & c, std::istream & in )
   {
      while ( c == ' ' || c == '\r' || c == '\n' || c == '\t' )
         in.get( c );
   }

   void eat_comment( char & c, std::istream & in )
   {
      in.get(c);
      if(c != '-')
         throw std::string("Invalid comment in XML");
      in.get(c);
      if(c != '-')
         throw std::string("Invalid comment in XML");
      do{
         while(in.get(c) && (c != '-'));
         in.get(c);
         if(c != '-') 
            continue;
         in.get(c);
         if(c != '>') 
            continue;
         else
            break;
      }
      while(true);
   }

   std::string get_name( char & c, std::istream & in )
   {
      std::string result;
      eat_whitespace( c, in );
      while ( std::strchr(
         "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.:", c )
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

         if(c == '!')
         {
            eat_comment(c, in);
            return e;
         }
         if(c == '?')
         {
            // XML processing instruction.
            e->name += c;
            if(!in.get( c )) // next char
               throw std::string("xml: unexpected eof");
            e->name += get_name(c, in);
            in >> std::ws;
            if(!in.get( c )) // next char
               throw std::string("xml: unexpected eof");
            while(c != '?')
            {
               e->content += c;
               if(!in.get( c )) // next char
                  throw std::string("xml: unexpected eof");
            }
            if(!in.get( c )) // next char
               throw std::string("xml: unexpected eof");
            if(c != '>')
               throw std::string("Invalid XML processing instruction.");
            return e;
         }

         e->name = get_name( c, in );
         eat_whitespace( c, in );

         // attributes
         while ( (c != '>') && (c != '/') )
         {
            attribute a;
            a.name = get_name( c, in );

            eat_delim( c, in, '=', msg );
            eat_delim( c, in, '\"', msg );

            a.value = get_value( c, in );

            e->attributes.push_back( a );
            eat_whitespace( c, in );
         }
         if(c == '/')
         {
            if(!in.get( c )) // next after '/'
               throw std::string("xml: unexpected eof");
            eat_whitespace( c, in );
            if(c != '>')
               throw std::string("xml: unexpected /");
            return e;
         }
         if(!in.get( c )) // next after '>'
            throw std::string("xml: unexpected eof");

         //eat_whitespace( c, in );

         do{
            // sub-elements
            while ( c == '<' )
            {
               if ( in.peek() == '/' ) 
                  break;
               element_ptr child(parse( in, msg ));
               child->parent = e;
               e->elements.push_back(child);
               in.get( c ); // next after '>'
               //eat_whitespace( c, in );
            }
            if (( in.peek() == '/' ) && (c == '<'))
               break;

            // content
            if ( (c != '<') )
            {
               element_ptr sub( new element );
               while ( c != '<' )
               {
                  sub->content += c;
                  if(!in.get( c ))
                     throw std::string("xml: unexpected eof");
               }
               sub->parent = e;
               e->elements.push_back( sub );
            }

            assert( c == '<' );
            if( in.peek() == '/' )
               break;
         }while(true);

         in.get(c);
         eat_delim( c, in, '/', msg );
         std::string end_name( get_name( c, in ) );
         if ( e->name != end_name )
            throw std::string("xml syntax error: beginning name ")
            + e->name + " did not match end name " + end_name
            + " (" + msg + ")";

         eat_delim( c, in, '>', msg );
         if(c != '>')
         {
            // we've eaten one character past the >, put it back:
            if(!in.putback(c))
               throw std::string("Unable to put back character");
         }
         return e;
      }

      //  write  ---------------------------------------------------------------//

      void write( const element & e, std::ostream & out )
      {
         if(e.name.size())
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
            if(e.name[0] == '?')
            {
               out << " " << e.content << "?>";
               return;
            }
            if(e.elements.empty() && e.content.empty())
            {
               out << "/>";
               return;
            }
            out << ">";
         }
         if ( !e.elements.empty() )
         {
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
         if(e.name.size() && (e.name[0] != '?'))
         {
            out << "</" << e.name << ">";
         }
      }

   } // namespace tiny_xml
} // namespace boost


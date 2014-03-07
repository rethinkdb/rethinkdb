//  tiny XML sub-set tools  --------------------------------------------------//

//  (C) Copyright Beman Dawes 2002.  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  Provides self-contained tools for this XML sub-set:
//
//    element ::= { "<" name { name "=" "\"" value "\"" } ">"
//                  {element} [contents] "</" name ">" }
//
//  The point of "self-contained" is to minimize tool-chain dependencies.

#ifndef BOOST_TINY_XML_H
#define BOOST_TINY_XML_H

#include "boost/smart_ptr.hpp" // for shared_ptr
#include "boost/utility.hpp"   // for noncopyable
#include <list>
#include <iostream>
#include <string>

namespace boost
{
   namespace tiny_xml
   {
      class element;
      struct attribute
      {
         std::string name;
         std::string value;

         attribute(){}
         attribute( const std::string & name, const std::string & value )
            : name(name), value(value) {}
      };
      typedef boost::shared_ptr< element >  element_ptr;
      typedef boost::weak_ptr< element >    weak_element_ptr;
      typedef std::list< element_ptr  >     element_list;
      typedef std::list< attribute >        attribute_list;

      class element
         : private boost::noncopyable  // because deep copy semantics would be required
      {
      public:
         // The name of the XML element, or "" if this is inline content,
         // or begins with '?' if this is a processing instruction.
         std::string       name;
         // List of attributes applied to this element
         attribute_list    attributes;
         // List of sub-elements, this will be empty if the name is empty
         // or starts with a '?'.  Plain text content will be inside
         // anonymous elements in this list - this preserves the order of
         // plain text mixed with true XML <elements>.
         element_list      elements;
         // The plain text content of this element, only present if the name is ""
         // or if this is a processing instruction in which case it is the content 
         // after the name of the instruction.
         std::string       content;
         // Pointer to our parent
         weak_element_ptr  parent;

         element() {}
         explicit element( const std::string & name ) : name(name) {}
      };

      // Precondition: stream positioned at either the initial "<"
      // or the first character after the initial "<".
      // Postcondition: stream positioned at the first character after final
      //  ">" (or eof).
      // Returns: an element_ptr to an element representing the parsed stream.
      // Throws: std::string on syntax error. msg appended to what() string.
      element_ptr parse( std::istream & in, const std::string & msg );

      void write( const element & e, std::ostream & out );

   }
}

#endif  // BOOST_TINY_XML_H




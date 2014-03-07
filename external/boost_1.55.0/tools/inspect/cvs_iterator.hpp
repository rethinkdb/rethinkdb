//  cvs_iterator  ------------------------------------------------------------//

//  Copyright Beman Dawes 2003.
//
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  WARNING: This is just a quick hack. It doesn't conform to C++ Standard
//  Library iterator requirements.

#ifndef BOOST_CVS_ITERATOR_HPP
#define BOOST_CVS_ITERATOR_HPP

#include <string>
#include <assert.h>

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/fstream.hpp"
#include "boost/noncopyable.hpp"

namespace hack
{
  class cvs_iterator : boost::noncopyable
  {
    boost::filesystem::ifstream  entries_file;
    boost::filesystem::path      dir_path;
    boost::filesystem::path      value_path;
  public:

    cvs_iterator(){} // end iterator

    ~cvs_iterator() { if ( !!entries_file ) entries_file.close(); }

    cvs_iterator( const boost::filesystem::path & dir_path ) : dir_path(dir_path)
    {
      boost::filesystem::path entries_file_path( dir_path / "CVS/Entries" );
      entries_file.open( entries_file_path );
      if ( !entries_file )
        throw std::string( "could not open: " ) + entries_file_path.string();
      ++*this;
    }

    const boost::filesystem::path & operator*() const { return value_path; }

    cvs_iterator & operator++()
    {
      assert( !!entries_file );
      std::string contents;
      do
      {
        do
        {
          std::getline( entries_file, contents );
          if ( entries_file.eof() )
          {
            entries_file.close();
            value_path = "";
            return *this;
          }
        } while ( contents == "D" );
        if ( contents[0] == 'D' ) contents.erase( 0, 1 );
        value_path = dir_path
          / boost::filesystem::path( contents.substr( 1, contents.find( '/', 1 ) ) );

      // in case entries file is mistaken, do until value_path actually found
      } while ( !boost::filesystem::exists( value_path ) );
      return *this;
    }

    bool operator==( const cvs_iterator & rhs )
      { return value_path.string() == rhs.value_path.string(); }

    bool operator!=( const cvs_iterator & rhs )
      { return value_path.string() != rhs.value_path.string(); }

  };
}

#endif // include guard

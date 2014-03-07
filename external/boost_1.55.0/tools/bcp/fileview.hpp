/*
 *
 * Copyright (c) 2003 Dr John Maddock
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#if defined(BOOST_FILESYSTEM_VERSION) && (BOOST_FILESYSTEM_VERSION != 3)
# error "This library must be built with Boost.Filesystem version 3"
#else
#define BOOST_FILESYSTEM_VERSION 3
#endif

#include <boost/shared_ptr.hpp>
#include <boost/filesystem/path.hpp>

class fileview
{
public:
   // types:
   typedef const char&                               reference;
   typedef reference                                 const_reference;
   typedef const char*                               iterator;       // See _lib.container.requirements_
   typedef iterator                                  const_iterator; // See _lib.container.requirements_
   typedef std::size_t                               size_type;
   typedef std::ptrdiff_t                            difference_type;
   typedef char                                      value_type;
   typedef const char*                               pointer;
   typedef pointer                                   const_pointer;
#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
   typedef std::reverse_iterator<iterator>           reverse_iterator;
   typedef std::reverse_iterator<const_iterator>     const_reverse_iterator;
#endif

   // construct:
   fileview();
   fileview(const boost::filesystem::path& p);
   ~fileview();
   fileview(const fileview& that);
   fileview& operator=(const fileview& that);
   void close();
   void open(const boost::filesystem::path& p);

   // iterators:
   const_iterator         begin() const;
   const_iterator         end() const;
#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
   const_reverse_iterator rbegin() const;
   const_reverse_iterator rend() const;
#endif

   // capacity:
   size_type size() const;
   size_type max_size() const;
   bool      empty() const;

   // element access:
   const_reference operator[](size_type n) const;
   const_reference at(size_type n) const;
   const_reference front() const;
   const_reference back() const;
   void            swap(fileview& that);

private:
   void cow();
   struct implementation;
   boost::shared_ptr<implementation> pimpl;
};



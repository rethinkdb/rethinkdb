/*
 *
 * Copyright (c) 2003 Dr John Maddock
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * This file implements the fileview class
 */

#include "fileview.hpp"
#include <boost/filesystem/fstream.hpp>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <istream>
#include <stdexcept>

struct fileview::implementation
{
   std::vector<char> m_data;
};


// construct:
fileview::fileview()
{
   pimpl.reset(new implementation());
}

fileview::fileview(const boost::filesystem::path& p)
{
   pimpl.reset(new implementation());
   open(p);
}

fileview::~fileview()
{
}

fileview::fileview(const fileview& )
{
}

fileview& fileview::operator=(const fileview& that)
{
   pimpl = that.pimpl;
   return *this;
}

void fileview::close()
{
   cow();
   pimpl->m_data.clear();
}

void fileview::open(const boost::filesystem::path& p)
{
   cow();
   boost::filesystem::ifstream is(p);
   if(!is)
   {
      std::string msg("Bad file name: ");
      msg += p.string();
      std::runtime_error e(msg);
      boost::throw_exception(e);
   }
   std::istreambuf_iterator<char> in(is);
   std::istreambuf_iterator<char> end;
   std::copy(in, end, std::back_inserter(pimpl->m_data));
}

// iterators:
fileview::const_iterator         fileview::begin() const
{
   return pimpl->m_data.size() ? &(pimpl->m_data[0]) : 0;
}

fileview::const_iterator         fileview::end() const
{
   return begin() + pimpl->m_data.size();
}

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
fileview::const_reverse_iterator fileview::rbegin() const
{
   return const_reverse_iterator(end());
}

fileview::const_reverse_iterator fileview::rend() const
{
   return const_reverse_iterator(begin());
}
#endif

// capacity:
fileview::size_type fileview::size() const
{
   return pimpl->m_data.size();
}

fileview::size_type fileview::max_size() const
{
   return pimpl->m_data.max_size();
}

bool      fileview::empty() const
{
   return pimpl->m_data.empty();
}

// element access:
fileview::const_reference fileview::operator[](fileview::size_type n) const
{
   return pimpl->m_data[n];
}

fileview::const_reference fileview::at(size_type n) const
{
   return pimpl->m_data.at(n);
}

fileview::const_reference fileview::front() const
{
   return pimpl->m_data.front();
}

fileview::const_reference fileview::back() const
{
   return pimpl->m_data.back();
}

void fileview::swap(fileview& that)
{
   pimpl.swap(that.pimpl);
}

void fileview::cow()
{
   if(!pimpl.unique())
      pimpl.reset(new implementation(*pimpl));
}

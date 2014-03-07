/*
 *
 * Copyright (c) 1998-2002
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#ifdef _MSC_VER
#pragma warning(disable: 4996 4127)
#endif

#include <string>
#include <algorithm>
#include <deque>
#include <iterator>

#ifdef BOOST_RE_OLD_IOSTREAM
#include <iostream.h>
#include <fstream.h>
#else
#include <iostream>
#include <fstream>
using std::cout;
using std::cin;
using std::cerr;
using std::istream;
using std::ostream;
using std::endl;
using std::ifstream;
using std::streambuf;
using std::getline;
#endif

#include <boost/config.hpp>
#include <boost/regex.hpp>
#include <boost/cregex.hpp>
#include <boost/timer.hpp> 
#include <boost/smart_ptr.hpp>

#if defined(_WIN32) && defined(BOOST_REGEX_USE_WIN32_LOCALE)
#include <windows.h>
#endif

#if (defined(_MSC_VER) && (_MSC_VER <= 1300)) || defined(__sgi)
// maybe no Koenig lookup, use using declaration instead:
using namespace boost;
#endif

#ifndef BOOST_NO_WREGEX
ostream& operator << (ostream& os, const std::wstring& s)
{
   std::wstring::const_iterator i, j;
   i = s.begin();
   j = s.end();
   while(i != j)
   {
      os.put(static_cast<char>(*i));
      ++i;
   }
   return os;
}
#endif

template <class S>
class string_out_iterator 
#ifndef BOOST_NO_STD_ITERATOR
  : public std::iterator<std::output_iterator_tag, void, void, void, void>
#endif // ndef BOOST_NO_STD_ITERATOR
{
#ifdef BOOST_NO_STD_ITERATOR
   typedef std::output_iterator_tag iterator_category;
   typedef void value_type;
   typedef void difference_type;
   typedef void pointer;
   typedef void reference;
#endif // BOOST_NO_STD_ITERATOR

   S* out;
public:
   string_out_iterator(S& s) : out(&s) {}
   string_out_iterator& operator++() { return *this; }
   string_out_iterator& operator++(int) { return *this; }
   string_out_iterator& operator*() { return *this; }
   string_out_iterator& operator=(typename S::value_type v) 
   { 
      out->append(1, v); 
      return *this; 
   }
};

namespace boost{
#if defined(BOOST_MSVC) || (defined(__BORLANDC__) && (__BORLANDC__ == 0x550)) || defined(__SGI_STL_PORT)
//
// problem with std::getline under MSVC6sp3
// and C++ Builder 5.5, is this really that hard?
istream& getline(istream& is, std::string& s)
{
   s.erase();
   char c = static_cast<char>(is.get());
   while(c != '\n')
   {
      BOOST_ASSERT(is.good());
      s.append(1, c);
      c = static_cast<char>(is.get());
   }
   return is;
}
#else
istream& getline(istream& is, std::string& s)
{
   std::getline(is, s);
   if(s.size() && (s[s.size() -1] == '\r'))
      s.erase(s.size() - 1);
   return is;
}
#endif
}


int main(int argc, char**argv)
{
   ifstream ifs;
   std::istream* p_in = &std::cin;
   if(argc == 2)
   {
      ifs.open(argv[1]);
      ifs.peek();
      if(!ifs.good())
      {
         cout << "Bad filename: " << argv[1] << endl;
         return -1;
      }
      p_in = &ifs;
   }
   
   boost::regex ex;
   boost::match_results<std::string::const_iterator> sm;
#ifndef BOOST_NO_WREGEX
   std::wstring ws1, ws2;
   boost::wregex wex;
   boost::match_results<std::wstring::const_iterator> wsm;
#endif
   boost::match_results<std::deque<char>::iterator> dm;
   std::string s1, s2, ts;
   std::deque<char> ds;
   boost::regex_tA r;
   boost::scoped_array<boost::regmatch_t> matches;
   std::size_t nsubs;
   boost::timer t;
   double tim;
   int result = 0;
   unsigned iters = 100;
   double wait_time = (std::min)(t.elapsed_min() * 1000, 0.5);

   while(true)
   {
      cout << "Enter expression (or \"quit\" to exit): ";
      boost::getline(*p_in, s1);
      if(argc == 2)
         cout << endl << s1 << endl;
      if(s1 == "quit")
         break;
#ifndef BOOST_NO_WREGEX
      ws1.erase();
      std::copy(s1.begin(), s1.end(), string_out_iterator<std::wstring>(ws1));
#endif
      try{
         ex.assign(s1);
#ifndef BOOST_NO_WREGEX
         wex.assign(ws1);
#endif
      }
      catch(std::exception& e)
      {
         cout << "Error in expression: \"" << e.what() << "\"" << endl;
         continue;
      }
      int code = regcompA(&r, s1.c_str(), boost::REG_PERL);
      if(code != 0)
      {
         char buf[256];
         regerrorA(code, &r, buf, 256);
         cout << "regcompA error: \"" << buf << "\"" << endl;
         continue;
      }
      nsubs = r.re_nsub + 1;
      matches.reset(new boost::regmatch_t[nsubs]);

      while(true)
      {
         cout << "Enter string to search (or \"quit\" to exit): ";
         boost::getline(*p_in, s2);
         if(argc == 2)
            cout << endl << s2 << endl;
         if(s2 == "quit")
            break;

#ifndef BOOST_NO_WREGEX
         ws2.erase();
         std::copy(s2.begin(), s2.end(), string_out_iterator<std::wstring>(ws2));
#endif
         ds.erase(ds.begin(), ds.end());
         std::copy(s2.begin(), s2.end(), std::back_inserter(ds));

         unsigned i;
         iters = 10;
         tim = 1.1;

#if defined(_WIN32) && defined(BOOST_REGEX_USE_WIN32_LOCALE)
         MSG msg;
         PeekMessage(&msg, 0, 0, 0, 0);
         Sleep(0);
#endif

         // cache load:
         regex_search(s2, sm, ex);

         // measure time interval for basic_regex<char>
         do{
            iters *= static_cast<unsigned>((tim > 0.001) ? (1.1/tim) : 100);
            t.restart();
            for(i =0; i < iters; ++i)
            {
               result = regex_search(s2, sm, ex);
            }
            tim = t.elapsed();
         }while(tim < wait_time);

         cout << "regex time: " << (tim * 1000000 / iters) << "us" << endl;
         if(result)
         {
            for(i = 0; i < sm.size(); ++i)
            {
               ts = sm[i];
               cout << "\tmatch " << i << ": \"";
               cout << ts;
               cout << "\" (matched=" << sm[i].matched << ")" << endl;
            }
            cout << "\tmatch $`: \"";
            cout << std::string(sm[-1]);
            cout << "\" (matched=" << sm[-1].matched << ")" << endl;
            cout << "\tmatch $': \"";
            cout << std::string(sm[-2]);
            cout << "\" (matched=" << sm[-2].matched << ")" << endl << endl;
         }

#ifndef BOOST_NO_WREGEX
         // measure time interval for boost::wregex
         iters = 10;
         tim = 1.1;
         // cache load:
         regex_search(ws2, wsm, wex);
         do{
            iters *= static_cast<unsigned>((tim > 0.001) ? (1.1/tim) : 100);
            t.restart();
            for(i = 0; i < iters; ++i)
            {
               result = regex_search(ws2, wsm, wex);
            }
            tim = t.elapsed();
         }while(tim < wait_time);
         cout << "wregex time: " << (tim * 1000000 / iters) << "us" << endl;
         if(result)
         {
            std::wstring tw;
            for(i = 0; i < wsm.size(); ++i)
            {
               tw.erase();
               std::copy(wsm[i].first, wsm[i].second, string_out_iterator<std::wstring>(tw));
               cout << "\tmatch " << i << ": \"" << tw;
               cout << "\" (matched=" << sm[i].matched << ")" << endl;
            }
            cout << "\tmatch $`: \"";
            tw.erase();
            std::copy(wsm[-1].first, wsm[-1].second, string_out_iterator<std::wstring>(tw));
            cout << tw;
            cout << "\" (matched=" << sm[-1].matched << ")" << endl;
            cout << "\tmatch $': \"";
            tw.erase();
            std::copy(wsm[-2].first, wsm[-2].second, string_out_iterator<std::wstring>(tw));
            cout << tw;
            cout << "\" (matched=" << sm[-2].matched << ")" << endl << endl;
         }
#endif
        
         // measure time interval for basic_regex<char> using a deque
         iters = 10;
         tim = 1.1;
         // cache load:
         regex_search(ds.begin(), ds.end(), dm, ex);
         do{
            iters *= static_cast<unsigned>((tim > 0.001) ? (1.1/tim) : 100);
            t.restart();
            for(i = 0; i < iters; ++i)
            {
               result = regex_search(ds.begin(), ds.end(), dm, ex);
            }
            tim = t.elapsed();
         }while(tim < wait_time);
         cout << "regex time (search over std::deque<char>): " << (tim * 1000000 / iters) << "us" << endl;

         if(result)
         {
            for(i = 0; i < dm.size(); ++i)
            {
               ts.erase();
               std::copy(dm[i].first, dm[i].second, string_out_iterator<std::string>(ts));
               cout << "\tmatch " << i << ": \"" << ts;
               cout << "\" (matched=" << sm[i].matched << ")" << endl;
            }
            cout << "\tmatch $`: \"";
            ts.erase();
            std::copy(dm[-1].first, dm[-1].second, string_out_iterator<std::string>(ts));
            cout << ts;
            cout << "\" (matched=" << sm[-1].matched << ")" << endl;
            cout << "\tmatch $': \"";
            ts.erase();
            std::copy(dm[-2].first, dm[-2].second, string_out_iterator<std::string>(ts));
            cout << ts;
            cout << "\" (matched=" << sm[-2].matched << ")" << endl << endl;
         }
         
         // measure time interval for POSIX matcher:
         iters = 10;
         tim = 1.1;
         // cache load:
         regexecA(&r, s2.c_str(), nsubs, matches.get(), 0);
         do{
            iters *= static_cast<unsigned>((tim > 0.001) ? (1.1/tim) : 100);
            t.restart();
            for(i = 0; i < iters; ++i)
            {
               result = regexecA(&r, s2.c_str(), nsubs, matches.get(), 0);
            }
            tim = t.elapsed();
         }while(tim < wait_time);
         cout << "POSIX regexecA time: " << (tim * 1000000 / iters) << "us" << endl;

         if(result == 0)
         {
            for(i = 0; i < nsubs; ++i)
            {
               if(matches[i].rm_so >= 0)
               {
                  ts.assign(s2.begin() + matches[i].rm_so, s2.begin() + matches[i].rm_eo);
                  cout << "\tmatch " << i << ": \"" << ts << "\" (matched=" << (matches[i].rm_so != -1) << ")"<< endl;
               }
               else
                  cout << "\tmatch " << i << ": \"\" (matched=" << (matches[i].rm_so != -1) << ")" << endl;   // no match
            }
            cout << "\tmatch $`: \"";
            ts.erase();
            ts.assign(s2.begin(), s2.begin() + matches[0].rm_so);
            cout << ts;
            cout << "\" (matched=" << (matches[0].rm_so != 0) << ")" << endl;
            cout << "\tmatch $': \"";
            ts.erase();
            ts.assign(s2.begin() + matches[0].rm_eo, s2.end());
            cout << ts;
            cout << "\" (matched=" << (matches[0].rm_eo != (int)s2.size()) << ")" << endl << endl;
         }
      }
      regfreeA(&r);
   }

   return 0;
}

#if defined(_WIN32) && defined(BOOST_REGEX_USE_WIN32_LOCALE) && !defined(UNDER_CE)
#pragma comment(lib, "user32.lib")
#endif












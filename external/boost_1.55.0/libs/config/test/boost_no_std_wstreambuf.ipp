//  (C) Copyright John Maddock 2001.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_STD_WSTREAMBUF
//  TITLE:         std::basic_streambuf<wchar_t>
//  DESCRIPTION:   The standard library lacks std::basic_streambuf<wchar_t>.

#include <iostream>
#include <streambuf>
#include <string>

namespace boost_no_std_wstreambuf{

template <class charT,
          class traits = ::std::char_traits<charT> >
class parser_buf : public ::std::basic_streambuf<charT, traits>
{
   typedef ::std::basic_streambuf<charT, traits> base_type;
   typedef typename base_type::int_type int_type;
   typedef typename base_type::char_type char_type;
   typedef typename base_type::pos_type pos_type;
   typedef ::std::streamsize streamsize;
   typedef typename base_type::off_type off_type;
public:
   parser_buf() : base_type() { setbuf(0, 0); }
   const charT* getnext() { return this->gptr(); }
protected:
   std::basic_streambuf<charT, traits>* setbuf(char_type* s, streamsize n);
   typename parser_buf<charT, traits>::pos_type seekpos(pos_type sp, ::std::ios_base::openmode which);
   typename parser_buf<charT, traits>::pos_type seekoff(off_type off, ::std::ios_base::seekdir way, ::std::ios_base::openmode which);
private:
   parser_buf& operator=(const parser_buf&)
   { return *this; };
   parser_buf(const parser_buf&);
};

template<class charT, class traits>
std::basic_streambuf<charT, traits>*
parser_buf<charT, traits>::setbuf(char_type* s, streamsize n)
{
   this->setg(s, s, s + n);
   return this;
}

template<class charT, class traits>
typename parser_buf<charT, traits>::pos_type
parser_buf<charT, traits>::seekoff(off_type off, ::std::ios_base::seekdir way, ::std::ios_base::openmode which)
{
   typedef typename parser_buf<charT, traits>::pos_type pos_type;
   if(which & ::std::ios_base::out)
      return pos_type(off_type(-1));
   int size = this->egptr() - this->eback();
   int pos = this->gptr() - this->eback();
   charT* g = this->eback();
   switch((int)way)
   {
   case ::std::ios_base::beg:
      if((off < 0) || (off > size))
         return pos_type(off_type(-1));
      else
         this->setg(g, g + off, g + size);
   case ::std::ios_base::end:
      if((off < 0) || (off > size))
         return pos_type(off_type(-1));
      else
         this->setg(g, g + size - off, g + size);
   case ::std::ios_base::cur:
   {
      int newpos = pos + off;
      if((newpos < 0) || (newpos > size))
         return pos_type(off_type(-1));
      else
         this->setg(g, g + newpos, g + size);
   }
   }
   return static_cast<pos_type>(this->gptr() - this->eback());
}

template<class charT, class traits>
typename parser_buf<charT, traits>::pos_type
parser_buf<charT, traits>::seekpos(pos_type sp, ::std::ios_base::openmode which)
{
   if(which & ::std::ios_base::out)
      return pos_type(off_type(-1));
   int size = this->egptr() - this->eback();
   charT* g = this->eback();
   if(off_type(sp) <= size)
   {
      this->setg(g, g + off_type(sp), g + size);
   }
   return pos_type(off_type(-1));
}


int test()
{
   return 0;
}

template class parser_buf<char>;
template class parser_buf<wchar_t>;

}


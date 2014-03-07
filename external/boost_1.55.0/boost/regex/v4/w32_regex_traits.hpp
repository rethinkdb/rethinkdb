/*
 *
 * Copyright (c) 2004
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */
 
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         w32_regex_traits.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares regular expression traits class w32_regex_traits.
  */

#ifndef BOOST_W32_REGEX_TRAITS_HPP_INCLUDED
#define BOOST_W32_REGEX_TRAITS_HPP_INCLUDED

#ifndef BOOST_RE_PAT_EXCEPT_HPP
#include <boost/regex/pattern_except.hpp>
#endif
#ifndef BOOST_REGEX_TRAITS_DEFAULTS_HPP_INCLUDED
#include <boost/regex/v4/regex_traits_defaults.hpp>
#endif
#ifdef BOOST_HAS_THREADS
#include <boost/regex/pending/static_mutex.hpp>
#endif
#ifndef BOOST_REGEX_PRIMARY_TRANSFORM
#include <boost/regex/v4/primary_transform.hpp>
#endif
#ifndef BOOST_REGEX_OBJECT_CACHE_HPP
#include <boost/regex/pending/object_cache.hpp>
#endif

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable: 4103)
#endif
#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4786)
#pragma warning(disable:4800)
#endif

namespace boost{ 

//
// forward declaration is needed by some compilers:
//
template <class charT>
class w32_regex_traits;
   
namespace re_detail{

//
// start by typedeffing the types we'll need:
//
typedef ::boost::uint32_t lcid_type;   // placeholder for LCID.
typedef ::boost::shared_ptr<void> cat_type; // placeholder for dll HANDLE.

//
// then add wrappers around the actual Win32 API's (ie implementation hiding):
//
BOOST_REGEX_DECL lcid_type BOOST_REGEX_CALL w32_get_default_locale();
BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is_lower(char, lcid_type);
#ifndef BOOST_NO_WREGEX
BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is_lower(wchar_t, lcid_type);
#ifdef BOOST_REGEX_HAS_OTHER_WCHAR_T
BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is_lower(unsigned short ca, lcid_type state_id);
#endif
#endif
BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is_upper(char, lcid_type);
#ifndef BOOST_NO_WREGEX
BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is_upper(wchar_t, lcid_type);
#ifdef BOOST_REGEX_HAS_OTHER_WCHAR_T
BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is_upper(unsigned short ca, lcid_type state_id);
#endif
#endif
BOOST_REGEX_DECL cat_type BOOST_REGEX_CALL w32_cat_open(const std::string& name);
BOOST_REGEX_DECL std::string BOOST_REGEX_CALL w32_cat_get(const cat_type& cat, lcid_type state_id, int i, const std::string& def);
#ifndef BOOST_NO_WREGEX
BOOST_REGEX_DECL std::wstring BOOST_REGEX_CALL w32_cat_get(const cat_type& cat, lcid_type state_id, int i, const std::wstring& def);
#ifdef BOOST_REGEX_HAS_OTHER_WCHAR_T
BOOST_REGEX_DECL std::basic_string<unsigned short> BOOST_REGEX_CALL w32_cat_get(const cat_type& cat, lcid_type, int i, const std::basic_string<unsigned short>& def);
#endif
#endif
BOOST_REGEX_DECL std::string BOOST_REGEX_CALL w32_transform(lcid_type state_id, const char* p1, const char* p2);
#ifndef BOOST_NO_WREGEX
BOOST_REGEX_DECL std::wstring BOOST_REGEX_CALL w32_transform(lcid_type state_id, const wchar_t* p1, const wchar_t* p2);
#ifdef BOOST_REGEX_HAS_OTHER_WCHAR_T
BOOST_REGEX_DECL std::basic_string<unsigned short> BOOST_REGEX_CALL w32_transform(lcid_type state_id, const unsigned short* p1, const unsigned short* p2);
#endif
#endif
BOOST_REGEX_DECL char BOOST_REGEX_CALL w32_tolower(char c, lcid_type);
#ifndef BOOST_NO_WREGEX
BOOST_REGEX_DECL wchar_t BOOST_REGEX_CALL w32_tolower(wchar_t c, lcid_type);
#ifdef BOOST_REGEX_HAS_OTHER_WCHAR_T
BOOST_REGEX_DECL unsigned short BOOST_REGEX_CALL w32_tolower(unsigned short c, lcid_type state_id);
#endif
#endif
BOOST_REGEX_DECL char BOOST_REGEX_CALL w32_toupper(char c, lcid_type);
#ifndef BOOST_NO_WREGEX
BOOST_REGEX_DECL wchar_t BOOST_REGEX_CALL w32_toupper(wchar_t c, lcid_type);
#endif
BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is(lcid_type, boost::uint32_t mask, char c);
#ifndef BOOST_NO_WREGEX
BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is(lcid_type, boost::uint32_t mask, wchar_t c);
#ifdef BOOST_REGEX_HAS_OTHER_WCHAR_T
BOOST_REGEX_DECL bool BOOST_REGEX_CALL w32_is(lcid_type state_id, boost::uint32_t m, unsigned short c);
#endif
#endif
//
// class w32_regex_traits_base:
// acts as a container for locale and the facets we are using.
//
template <class charT>
struct w32_regex_traits_base
{
   w32_regex_traits_base(lcid_type l)
   { imbue(l); }
   lcid_type imbue(lcid_type l);

   lcid_type m_locale;
};

template <class charT>
inline lcid_type w32_regex_traits_base<charT>::imbue(lcid_type l)
{
   lcid_type result(m_locale);
   m_locale = l;
   return result;
}

//
// class w32_regex_traits_char_layer:
// implements methods that require specialisation for narrow characters:
//
template <class charT>
class w32_regex_traits_char_layer : public w32_regex_traits_base<charT>
{
   typedef std::basic_string<charT> string_type;
   typedef std::map<charT, regex_constants::syntax_type> map_type;
   typedef typename map_type::const_iterator map_iterator_type;
public:
   w32_regex_traits_char_layer(const lcid_type l);

   regex_constants::syntax_type syntax_type(charT c)const
   {
      map_iterator_type i = m_char_map.find(c);
      return ((i == m_char_map.end()) ? 0 : i->second);
   }
   regex_constants::escape_syntax_type escape_syntax_type(charT c) const
   {
      map_iterator_type i = m_char_map.find(c);
      if(i == m_char_map.end())
      {
         if(::boost::re_detail::w32_is_lower(c, this->m_locale)) return regex_constants::escape_type_class;
         if(::boost::re_detail::w32_is_upper(c, this->m_locale)) return regex_constants::escape_type_not_class;
         return 0;
      }
      return i->second;
   }
   charT tolower(charT c)const
   {
      return ::boost::re_detail::w32_tolower(c, this->m_locale);
   }
   bool isctype(boost::uint32_t mask, charT c)const
   {
      return ::boost::re_detail::w32_is(this->m_locale, mask, c);
   }

private:
   string_type get_default_message(regex_constants::syntax_type);
   // TODO: use a hash table when available!
   map_type m_char_map;
};

template <class charT>
w32_regex_traits_char_layer<charT>::w32_regex_traits_char_layer(::boost::re_detail::lcid_type l) 
   : w32_regex_traits_base<charT>(l)
{
   // we need to start by initialising our syntax map so we know which
   // character is used for which purpose:
   cat_type cat;
   std::string cat_name(w32_regex_traits<charT>::get_catalog_name());
   if(cat_name.size())
   {
      cat = ::boost::re_detail::w32_cat_open(cat_name);
      if(!cat)
      {
         std::string m("Unable to open message catalog: ");
         std::runtime_error err(m + cat_name);
         boost::re_detail::raise_runtime_error(err);
      }
   }
   //
   // if we have a valid catalog then load our messages:
   //
   if(cat)
   {
      for(regex_constants::syntax_type i = 1; i < regex_constants::syntax_max; ++i)
      {
         string_type mss = ::boost::re_detail::w32_cat_get(cat, this->m_locale, i, get_default_message(i));
         for(typename string_type::size_type j = 0; j < mss.size(); ++j)
         {
            this->m_char_map[mss[j]] = i;
         }
      }
   }
   else
   {
      for(regex_constants::syntax_type i = 1; i < regex_constants::syntax_max; ++i)
      {
         const char* ptr = get_default_syntax(i);
         while(ptr && *ptr)
         {
            this->m_char_map[static_cast<charT>(*ptr)] = i;
            ++ptr;
         }
      }
   }
}

template <class charT>
typename w32_regex_traits_char_layer<charT>::string_type 
   w32_regex_traits_char_layer<charT>::get_default_message(regex_constants::syntax_type i)
{
   const char* ptr = get_default_syntax(i);
   string_type result;
   while(ptr && *ptr)
   {
      result.append(1, static_cast<charT>(*ptr));
      ++ptr;
   }
   return result;
}

//
// specialised version for narrow characters:
//
template <>
class BOOST_REGEX_DECL w32_regex_traits_char_layer<char> : public w32_regex_traits_base<char>
{
   typedef std::string string_type;
public:
   w32_regex_traits_char_layer(::boost::re_detail::lcid_type l)
   : w32_regex_traits_base<char>(l)
   {
      init();
   }

   regex_constants::syntax_type syntax_type(char c)const
   {
      return m_char_map[static_cast<unsigned char>(c)];
   }
   regex_constants::escape_syntax_type escape_syntax_type(char c) const
   {
      return m_char_map[static_cast<unsigned char>(c)];
   }
   char tolower(char c)const
   {
      return m_lower_map[static_cast<unsigned char>(c)];
   }
   bool isctype(boost::uint32_t mask, char c)const
   {
      return m_type_map[static_cast<unsigned char>(c)] & mask;
   }

private:
   regex_constants::syntax_type m_char_map[1u << CHAR_BIT];
   char m_lower_map[1u << CHAR_BIT];
   boost::uint16_t m_type_map[1u << CHAR_BIT];
   void init();
};

//
// class w32_regex_traits_implementation:
// provides pimpl implementation for w32_regex_traits.
//
template <class charT>
class w32_regex_traits_implementation : public w32_regex_traits_char_layer<charT>
{
public:
   typedef typename w32_regex_traits<charT>::char_class_type char_class_type;
   BOOST_STATIC_CONSTANT(char_class_type, mask_word = 0x0400); // must be C1_DEFINED << 1
   BOOST_STATIC_CONSTANT(char_class_type, mask_unicode = 0x0800); // must be C1_DEFINED << 2
   BOOST_STATIC_CONSTANT(char_class_type, mask_horizontal = 0x1000); // must be C1_DEFINED << 3
   BOOST_STATIC_CONSTANT(char_class_type, mask_vertical = 0x2000); // must be C1_DEFINED << 4
   BOOST_STATIC_CONSTANT(char_class_type, mask_base = 0x3ff);  // all the masks used by the CT_CTYPE1 group

   typedef std::basic_string<charT> string_type;
   typedef charT char_type;
   w32_regex_traits_implementation(::boost::re_detail::lcid_type l);
   std::string error_string(regex_constants::error_type n) const
   {
      if(!m_error_strings.empty())
      {
         std::map<int, std::string>::const_iterator p = m_error_strings.find(n);
         return (p == m_error_strings.end()) ? std::string(get_default_error_string(n)) : p->second;
      }
      return get_default_error_string(n);
   }
   char_class_type lookup_classname(const charT* p1, const charT* p2) const
   {
      char_class_type result = lookup_classname_imp(p1, p2);
      if(result == 0)
      {
         typedef typename string_type::size_type size_type;
         string_type temp(p1, p2);
         for(size_type i = 0; i < temp.size(); ++i)
            temp[i] = this->tolower(temp[i]);
         result = lookup_classname_imp(&*temp.begin(), &*temp.begin() + temp.size());
      }
      return result;
   }
   string_type lookup_collatename(const charT* p1, const charT* p2) const;
   string_type transform_primary(const charT* p1, const charT* p2) const;
   string_type transform(const charT* p1, const charT* p2) const
   {
      return ::boost::re_detail::w32_transform(this->m_locale, p1, p2);
   }
private:
   std::map<int, std::string>     m_error_strings;   // error messages indexed by numberic ID
   std::map<string_type, char_class_type>  m_custom_class_names; // character class names
   std::map<string_type, string_type>      m_custom_collate_names; // collating element names
   unsigned                       m_collate_type;    // the form of the collation string
   charT                          m_collate_delim;   // the collation group delimiter
   //
   // helpers:
   //
   char_class_type lookup_classname_imp(const charT* p1, const charT* p2) const;
};

template <class charT>
typename w32_regex_traits_implementation<charT>::string_type 
   w32_regex_traits_implementation<charT>::transform_primary(const charT* p1, const charT* p2) const
{
   string_type result;
   //
   // What we do here depends upon the format of the sort key returned by
   // sort key returned by this->transform:
   //
   switch(m_collate_type)
   {
   case sort_C:
   case sort_unknown:
      // the best we can do is translate to lower case, then get a regular sort key:
      {
         result.assign(p1, p2);
         typedef typename string_type::size_type size_type;
         for(size_type i = 0; i < result.size(); ++i)
            result[i] = this->tolower(result[i]);
         result = this->transform(&*result.begin(), &*result.begin() + result.size());
         break;
      }
   case sort_fixed:
      {
         // get a regular sort key, and then truncate it:
         result.assign(this->transform(p1, p2));
         result.erase(this->m_collate_delim);
         break;
      }
   case sort_delim:
         // get a regular sort key, and then truncate everything after the delim:
         result.assign(this->transform(p1, p2));
         std::size_t i;
         for(i = 0; i < result.size(); ++i)
         {
            if(result[i] == m_collate_delim)
               break;
         }
         result.erase(i);
         break;
   }
   if(result.empty())
      result = string_type(1, charT(0));
   return result;
}

template <class charT>
typename w32_regex_traits_implementation<charT>::string_type 
   w32_regex_traits_implementation<charT>::lookup_collatename(const charT* p1, const charT* p2) const
{
   typedef typename std::map<string_type, string_type>::const_iterator iter_type;
   if(m_custom_collate_names.size())
   {
      iter_type pos = m_custom_collate_names.find(string_type(p1, p2));
      if(pos != m_custom_collate_names.end())
         return pos->second;
   }
#if !defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)\
               && !BOOST_WORKAROUND(BOOST_MSVC, < 1300)\
               && !BOOST_WORKAROUND(__BORLANDC__, <= 0x0551)
   std::string name(p1, p2);
#else
   std::string name;
   const charT* p0 = p1;
   while(p0 != p2)
      name.append(1, char(*p0++));
#endif
   name = lookup_default_collate_name(name);
#if !defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)\
               && !BOOST_WORKAROUND(BOOST_MSVC, < 1300)\
               && !BOOST_WORKAROUND(__BORLANDC__, <= 0x0551)
   if(name.size())
      return string_type(name.begin(), name.end());
#else
   if(name.size())
   {
      string_type result;
      typedef std::string::const_iterator iter;
      iter b = name.begin();
      iter e = name.end();
      while(b != e)
         result.append(1, charT(*b++));
      return result;
   }
#endif
   if(p2 - p1 == 1)
      return string_type(1, *p1);
   return string_type();
}

template <class charT>
w32_regex_traits_implementation<charT>::w32_regex_traits_implementation(::boost::re_detail::lcid_type l)
: w32_regex_traits_char_layer<charT>(l)
{
   cat_type cat;
   std::string cat_name(w32_regex_traits<charT>::get_catalog_name());
   if(cat_name.size())
   {
      cat = ::boost::re_detail::w32_cat_open(cat_name);
      if(!cat)
      {
         std::string m("Unable to open message catalog: ");
         std::runtime_error err(m + cat_name);
         boost::re_detail::raise_runtime_error(err);
      }
   }
   //
   // if we have a valid catalog then load our messages:
   //
   if(cat)
   {
      //
      // Error messages:
      //
      for(boost::regex_constants::error_type i = static_cast<boost::regex_constants::error_type>(0); 
         i <= boost::regex_constants::error_unknown; 
         i = static_cast<boost::regex_constants::error_type>(i + 1))
      {
         const char* p = get_default_error_string(i);
         string_type default_message;
         while(*p)
         {
            default_message.append(1, static_cast<charT>(*p));
            ++p;
         }
         string_type s = ::boost::re_detail::w32_cat_get(cat, this->m_locale, i+200, default_message);
         std::string result;
         for(std::string::size_type j = 0; j < s.size(); ++j)
         {
            result.append(1, static_cast<char>(s[j]));
         }
         m_error_strings[i] = result;
      }
      //
      // Custom class names:
      //
      static const char_class_type masks[14] = 
      {
         0x0104u, // C1_ALPHA | C1_DIGIT
         0x0100u, // C1_ALPHA
         0x0020u, // C1_CNTRL
         0x0004u, // C1_DIGIT
         (~(0x0020u|0x0008u) & 0x01ffu) | 0x0400u, // not C1_CNTRL or C1_SPACE
         0x0002u, // C1_LOWER
         (~0x0020u & 0x01ffu) | 0x0400, // not C1_CNTRL
         0x0010u, // C1_PUNCT
         0x0008u, // C1_SPACE
         0x0001u, // C1_UPPER
         0x0080u, // C1_XDIGIT
         0x0040u, // C1_BLANK
         w32_regex_traits_implementation<charT>::mask_word,
         w32_regex_traits_implementation<charT>::mask_unicode,
      };
      static const string_type null_string;
      for(unsigned int j = 0; j <= 13; ++j)
      {
         string_type s(::boost::re_detail::w32_cat_get(cat, this->m_locale, j+300, null_string));
         if(s.size())
            this->m_custom_class_names[s] = masks[j];
      }
   }
   //
   // get the collation format used by m_pcollate:
   //
   m_collate_type = re_detail::find_sort_syntax(this, &m_collate_delim);
}

template <class charT>
typename w32_regex_traits_implementation<charT>::char_class_type 
   w32_regex_traits_implementation<charT>::lookup_classname_imp(const charT* p1, const charT* p2) const
{
   static const char_class_type masks[22] = 
   {
      0,
      0x0104u, // C1_ALPHA | C1_DIGIT
      0x0100u, // C1_ALPHA
      0x0040u, // C1_BLANK
      0x0020u, // C1_CNTRL
      0x0004u, // C1_DIGIT
      0x0004u, // C1_DIGIT
      (~(0x0020u|0x0008u|0x0040) & 0x01ffu) | 0x0400u, // not C1_CNTRL or C1_SPACE or C1_BLANK
      w32_regex_traits_implementation<charT>::mask_horizontal, 
      0x0002u, // C1_LOWER
      0x0002u, // C1_LOWER
      (~0x0020u & 0x01ffu) | 0x0400, // not C1_CNTRL
      0x0010u, // C1_PUNCT
      0x0008u, // C1_SPACE
      0x0008u, // C1_SPACE
      0x0001u, // C1_UPPER
      w32_regex_traits_implementation<charT>::mask_unicode,
      0x0001u, // C1_UPPER
      w32_regex_traits_implementation<charT>::mask_vertical, 
      0x0104u | w32_regex_traits_implementation<charT>::mask_word, 
      0x0104u | w32_regex_traits_implementation<charT>::mask_word, 
      0x0080u, // C1_XDIGIT
   };
   if(m_custom_class_names.size())
   {
      typedef typename std::map<std::basic_string<charT>, char_class_type>::const_iterator map_iter;
      map_iter pos = m_custom_class_names.find(string_type(p1, p2));
      if(pos != m_custom_class_names.end())
         return pos->second;
   }
   std::size_t state_id = 1 + re_detail::get_default_class_id(p1, p2);
   if(state_id < sizeof(masks) / sizeof(masks[0]))
      return masks[state_id];
   return masks[0];
}


template <class charT>
boost::shared_ptr<const w32_regex_traits_implementation<charT> > create_w32_regex_traits(::boost::re_detail::lcid_type l BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(charT))
{
   // TODO: create a cache for previously constructed objects.
   return boost::object_cache< ::boost::re_detail::lcid_type, w32_regex_traits_implementation<charT> >::get(l, 5);
}

} // re_detail

template <class charT>
class w32_regex_traits
{
public:
   typedef charT                         char_type;
   typedef std::size_t                   size_type;
   typedef std::basic_string<char_type>  string_type;
   typedef ::boost::re_detail::lcid_type locale_type;
   typedef boost::uint_least32_t         char_class_type;

   struct boost_extensions_tag{};

   w32_regex_traits()
      : m_pimpl(re_detail::create_w32_regex_traits<charT>(::boost::re_detail::w32_get_default_locale()))
   { }
   static size_type length(const char_type* p)
   {
      return std::char_traits<charT>::length(p);
   }
   regex_constants::syntax_type syntax_type(charT c)const
   {
      return m_pimpl->syntax_type(c);
   }
   regex_constants::escape_syntax_type escape_syntax_type(charT c) const
   {
      return m_pimpl->escape_syntax_type(c);
   }
   charT translate(charT c) const
   {
      return c;
   }
   charT translate_nocase(charT c) const
   {
      return this->m_pimpl->tolower(c);
   }
   charT translate(charT c, bool icase) const
   {
      return icase ? this->m_pimpl->tolower(c) : c;
   }
   charT tolower(charT c) const
   {
      return this->m_pimpl->tolower(c);
   }
   charT toupper(charT c) const
   {
      return ::boost::re_detail::w32_toupper(c, this->m_pimpl->m_locale);
   }
   string_type transform(const charT* p1, const charT* p2) const
   {
      return ::boost::re_detail::w32_transform(this->m_pimpl->m_locale, p1, p2);
   }
   string_type transform_primary(const charT* p1, const charT* p2) const
   {
      return m_pimpl->transform_primary(p1, p2);
   }
   char_class_type lookup_classname(const charT* p1, const charT* p2) const
   {
      return m_pimpl->lookup_classname(p1, p2);
   }
   string_type lookup_collatename(const charT* p1, const charT* p2) const
   {
      return m_pimpl->lookup_collatename(p1, p2);
   }
   bool isctype(charT c, char_class_type f) const
   {
      if((f & re_detail::w32_regex_traits_implementation<charT>::mask_base) 
         && (this->m_pimpl->isctype(f & re_detail::w32_regex_traits_implementation<charT>::mask_base, c)))
         return true;
      else if((f & re_detail::w32_regex_traits_implementation<charT>::mask_unicode) && re_detail::is_extended(c))
         return true;
      else if((f & re_detail::w32_regex_traits_implementation<charT>::mask_word) && (c == '_'))
         return true;
      else if((f & re_detail::w32_regex_traits_implementation<charT>::mask_vertical)
         && (::boost::re_detail::is_separator(c) || (c == '\v')))
         return true;
      else if((f & re_detail::w32_regex_traits_implementation<charT>::mask_horizontal) 
         && this->isctype(c, 0x0008u) && !this->isctype(c, re_detail::w32_regex_traits_implementation<charT>::mask_vertical))
         return true;
      return false;
   }
   int toi(const charT*& p1, const charT* p2, int radix)const
   {
      return ::boost::re_detail::global_toi(p1, p2, radix, *this);
   }
   int value(charT c, int radix)const
   {
      int result = ::boost::re_detail::global_value(c);
      return result < radix ? result : -1;
   }
   locale_type imbue(locale_type l)
   {
      ::boost::re_detail::lcid_type result(getloc());
      m_pimpl = re_detail::create_w32_regex_traits<charT>(l);
      return result;
   }
   locale_type getloc()const
   {
      return m_pimpl->m_locale;
   }
   std::string error_string(regex_constants::error_type n) const
   {
      return m_pimpl->error_string(n);
   }

   //
   // extension:
   // set the name of the message catalog in use (defaults to "boost_regex").
   //
   static std::string catalog_name(const std::string& name);
   static std::string get_catalog_name();

private:
   boost::shared_ptr<const re_detail::w32_regex_traits_implementation<charT> > m_pimpl;
   //
   // catalog name handler:
   //
   static std::string& get_catalog_name_inst();

#ifdef BOOST_HAS_THREADS
   static static_mutex& get_mutex_inst();
#endif
};

template <class charT>
std::string w32_regex_traits<charT>::catalog_name(const std::string& name)
{
#ifdef BOOST_HAS_THREADS
   static_mutex::scoped_lock lk(get_mutex_inst());
#endif
   std::string result(get_catalog_name_inst());
   get_catalog_name_inst() = name;
   return result;
}

template <class charT>
std::string& w32_regex_traits<charT>::get_catalog_name_inst()
{
   static std::string s_name;
   return s_name;
}

template <class charT>
std::string w32_regex_traits<charT>::get_catalog_name()
{
#ifdef BOOST_HAS_THREADS
   static_mutex::scoped_lock lk(get_mutex_inst());
#endif
   std::string result(get_catalog_name_inst());
   return result;
}

#ifdef BOOST_HAS_THREADS
template <class charT>
static_mutex& w32_regex_traits<charT>::get_mutex_inst()
{
   static static_mutex s_mutex = BOOST_STATIC_MUTEX_INIT;
   return s_mutex;
}
#endif


} // boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable: 4103)
#endif
#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif

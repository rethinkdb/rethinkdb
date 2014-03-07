// Copyright 2008 John Maddock
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_AUTO_INDEX_HPP
#define BOOST_AUTO_INDEX_HPP

#include <boost/version.hpp>

#if BOOST_VERSION < 104400
#  error "This tool requires Boost 1.44 or later to build."
#endif

#define BOOST_FILESYSTEM_VERSION 3

#include "tiny_xml.hpp"
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <fstream>
#include <cctype>
#include <map>
#include <set>
#include <sstream>

struct index_info
{
   std::string term;            // The term goes in the index entry
   boost::regex search_text;    // What to search for when indexing the term.
   boost::regex search_id;      // What id's this term may be indexed in.
   std::string category;        // Index category (function, class, macro etc).
};
inline bool operator < (const index_info& a, const index_info& b)
{
   return (a.term != b.term) ? (a.term < b.term) : (a.category < b.category);
}


struct index_entry;
typedef boost::shared_ptr<index_entry> index_entry_ptr;
bool operator < (const index_entry_ptr& a, const index_entry_ptr& b);
typedef std::set<index_entry_ptr> index_entry_set;

struct index_entry
{
   std::string key;            // The index term.
   std::string sort_key;       // upper case version of term used for sorting.
   std::string id;             // The id of the block that we will link to.
   std::string category;       // The category of this entry (function, class, macro etc).
   index_entry_set sub_keys;   // All our sub-keys.
   bool preferred;             // This entry is the preferred one for this key

   index_entry() : preferred(false) {}
   index_entry(const std::string& k) : key(k), sort_key(k), preferred(false)  { boost::to_upper(sort_key); }
   index_entry(const std::string& k, const std::string& i) : key(k), sort_key(k), id(i), preferred(false)  { boost::to_upper(sort_key); }
   index_entry(const std::string& k, const std::string& i, const std::string& c) : key(k), sort_key(k), id(i), category(c), preferred(false)  { boost::to_upper(sort_key); }
};


inline bool operator < (const index_entry_ptr& a, const index_entry_ptr& b)
{
   return ((a->sort_key != b->sort_key) ? (a->sort_key < b->sort_key) : (a->category < b->category));
}

struct id_rewrite_rule
{
   bool base_on_id;          // rewrite the title if "id" matches the section id, otherwise rewrite if title matches "id".
   boost::regex id;          // regex for the id or title to match
   std::string new_name;     // either literal string or format string for the new name.

   id_rewrite_rule(const std::string& i, const std::string& n, bool b)
      : base_on_id(b), id(i), new_name(n) {}
};

struct node_id
{
   const std::string* id;
   node_id* prev;
};

struct title_info
{
   std::string title;
   title_info* prev;
};

struct file_scanner
{
   boost::regex scanner, file_name_filter, section_filter;
   std::string format_string, type, term_formatter;
};

inline bool operator < (const file_scanner & a, const file_scanner& b)
{
   return a.type < b.type;
}

typedef std::multiset<file_scanner> file_scanner_set_type;

void process_script(const std::string& script);
void scan_dir(const std::string& dir, const std::string& mask, bool recurse);
void scan_file(const std::string& file);
void generate_indexes();
const std::string* find_attr(boost::tiny_xml::element_ptr node, const char* name);

extern file_scanner_set_type file_scanner_set;

inline void add_file_scanner(const std::string& type, const std::string& scanner, const std::string& format, const std::string& term_formatter, const std::string& id_filter, const std::string& file_filter)
{
   file_scanner s;
   s.type = type;
   s.scanner = scanner;
   s.format_string = format;
   s.term_formatter = term_formatter;
   if(file_filter.size())
      s.file_name_filter = file_filter; 
   if(id_filter.size())
      s.section_filter = id_filter;
   file_scanner_set.insert(s);
}

extern std::set<index_info> index_terms;
extern std::set<std::pair<std::string, std::string> > found_terms;
extern bool no_duplicates;
extern bool verbose;
extern index_entry_set index_entries;
extern boost::tiny_xml::element_list indexes;
extern std::list<id_rewrite_rule> id_rewrite_list;
extern bool internal_indexes;
extern std::string prefix;
extern std::string internal_index_type;
extern boost::regex debug;

#endif

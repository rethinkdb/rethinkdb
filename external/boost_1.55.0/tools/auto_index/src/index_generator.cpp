// Copyright 2008 John Maddock
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "auto_index.hpp"
#include <boost/range.hpp>
#include <boost/format.hpp>

//
// Get a numerical ID for the next item:
//
std::string get_next_index_id()
{
   static int index_id_count = 0;
   std::stringstream s;
   s << "idx_id_" << index_id_count;
   ++index_id_count;
   return s.str();
}

void raise_invalid_xml(const std::string& parent, const std::string& child)
{
   throw std::runtime_error("Error: element " + child + " can not appear inside the container " + parent + ": try using a different value for property \"auto-index-type\".");
}
//
// Validate that the container for the Index is in a valid place:
//
void check_index_type_and_placement(const std::string& parent, const std::string& container)
{
   if(container == "section")
   {
      if((parent != "appendix")
         && (parent != "article")
         && (parent != "chapter")
         && (parent != "partintro")
         && (parent != "preface")
         && (parent != "section"))
         raise_invalid_xml(parent, container);
   }
   else if(container == "appendix")
   {
      if((parent != "article")
         && (parent != "book")
         && (parent != "part"))
         raise_invalid_xml(parent, container);
   }
   else if(container == "index")
   {
      if((parent != "appendix")
         && (parent != "article")
         && (parent != "book")
         && (parent != "chapter")
         && (parent != "part")
         && (parent != "preface")
         && (parent != "sect1")
         && (parent != "sect2")
         && (parent != "sect3")
         && (parent != "sect4")
         && (parent != "sect5")
         && (parent != "section")
         )
         raise_invalid_xml(parent, container);
   }
   else if((container == "article") || (container == "chapter") || (container == "reference"))
   {
      if((parent != "book")
         && (parent != "part"))
         raise_invalid_xml(parent, container);
   }
   else if(container == "part")
   {
      if(parent != "book")
         raise_invalid_xml(parent, container);
   }
   else if(container == "refsect1")
   {
      if(parent != "refentry")
         raise_invalid_xml(parent, container);
   }
   else if(container == "refsect2")
   {
      if(parent != "refsect1")
         raise_invalid_xml(parent, container);
   }
   else if(container == "refsect3")
   {
      if(parent != "refsect2")
         raise_invalid_xml(parent, container);
   }
   else if(container == "refsection")
   {
      if((parent != "refsection") && (parent != "refentry"))
         raise_invalid_xml(parent, container);
   }
   else if(container == "sect1")
   {
      if((parent != "appendix")
         && (parent != "article")
         && (parent != "chapter")
         && (parent != "partintro")
         && (parent != "preface")
         )
         raise_invalid_xml(parent, container);
   }
   else if(container == "sect2")
   {
      if(parent != "sect1")
         raise_invalid_xml(parent, container);
   }
   else if(container == "sect3")
   {
      if(parent != "sect2")
         raise_invalid_xml(parent, container);
   }
   else if(container == "sect4")
   {
      if(parent != "sect3")
         raise_invalid_xml(parent, container);
   }
   else if(container == "sect5")
   {
      if(parent != "sect4")
         raise_invalid_xml(parent, container);
   }
   else
   {
      throw std::runtime_error("Error: element " + container + " is unknown, and can not be used as a container for an index: try using a different value for property \"auto-index-type\".");
   }
}

boost::tiny_xml::element_ptr make_element(const std::string& name)
{
   boost::tiny_xml::element_ptr result(new boost::tiny_xml::element);
   result->name = name;
   return result;
}

boost::tiny_xml::element_ptr add_attribute(boost::tiny_xml::element_ptr ptr, const std::string& name, const std::string& value)
{
   boost::tiny_xml::attribute attr;
   attr.name = name;
   attr.value = value;
   ptr->attributes.push_back(attr);
   return ptr;
}

boost::regex make_primary_key_matcher(const std::string& s)
{
   static const boost::regex e("[-_[:space:]]+|([.\\[{}()\\*+?|^$])");
   static const char* format = "(?1\\\\$1:[-_[:space:]]+)";
   return boost::regex(regex_replace(s, e, format, boost::regex_constants::format_all), boost::regex::icase|boost::regex::perl);
}

//
// Generate an index entry using our own internal method:
//
template <class Range>
boost::tiny_xml::element_ptr generate_entry(const Range& range, const std::string* pcategory, int level = 0, const boost::regex* primary_key = 0)
{
   boost::tiny_xml::element_ptr list = add_attribute(add_attribute(::add_attribute(make_element("itemizedlist"), "mark", "none"), "spacing", "compact"), "role", "index");

   for(typename boost::range_iterator<Range>::type i = boost::begin(range); i != boost::end(range);)
   {
      std::string key = (*i)->key;
      index_entry_set entries;
      bool preferred = false;
      std::string id;
      bool collapse = false;
      
      //
      // Create a regular expression for comparing key to other key's:
      //
      boost::regex key_regex;
      if(level == 0)
      {
         key_regex = make_primary_key_matcher(key);
         primary_key = &key_regex;
      }
      //
      // Begin by consolidating entries with identical keys but possibly different categories:
      //
      while((i != boost::end(range)) && ((*i)->key == key))
      {
         if((0 == pcategory) || (pcategory->size() == 0) || (pcategory && (**i).category == *pcategory))
         {
            entries.insert((*i)->sub_keys.begin(), (*i)->sub_keys.end());
            if((*i)->preferred)
               preferred = true;
            if((*i)->id.size())
            {
               if(id.size())
               {
                  std::cerr << "WARNING: two identical index terms have different link destinations!!" << std::endl;
               }
               id = (*i)->id;
            }
         }
         ++i;
      }
      //
      // Only actually generate content if we have anything in the entries set:
      //
      if(entries.size() || id.size())
      {
         //
         // See if we can collapse any sub-entries into this one:
         //
         if(entries.size() == 1)
         {
            if((regex_match((*entries.begin())->key, *primary_key) || ((*entries.begin())->key == key)) 
               && ((*entries.begin())->id.size()) 
               && ((*entries.begin())->id != id))
            {
               collapse = true;
               id = (*entries.begin())->id;
            }
         }
         //
         // See if this key is the same as the primary key, if it is then make it prefered:
         //
         if(level && regex_match(key, *primary_key))
         {
            preferred = true;
         }
         boost::tiny_xml::element_ptr item = make_element("listitem");
         boost::tiny_xml::element_ptr para = make_element("para");
         item->elements.push_back(para);
         list->elements.push_back(item);
         if(preferred)
         {
            para->elements.push_back(add_attribute(make_element("emphasis"), "role", "bold"));
            para = para->elements.back();
         }
         if(id.size())
         {
            boost::tiny_xml::element_ptr link = add_attribute(make_element("link"), "linkend", id);
            para->elements.push_back(link);
            para = link;
         }
         std::string classname = (boost::format("index-entry-level-%1%") % level).str();
         para->elements.push_back(add_attribute(make_element("phrase"), "role", classname));
         para = para->elements.back();
         para->content = key;
         if(!collapse && entries.size())
         {
            std::pair<index_entry_set::const_iterator, index_entry_set::const_iterator> subrange(entries.begin(), entries.end());
            item->elements.push_back(generate_entry(subrange, 0, level+1, primary_key));
         }
      }
   }
   return list;
}
//
// Generate indexes using our own internal method:
//
void generate_indexes()
{
   for(boost::tiny_xml::element_list::const_iterator i = indexes.begin(); i != indexes.end(); ++i)
   {
      boost::tiny_xml::element_ptr node = *i;
      const std::string* category = find_attr(node, "type");
      bool has_title = false;

      for(boost::tiny_xml::element_list::const_iterator k = (*i)->elements.begin(); k != (*i)->elements.end(); ++k)
      {
         if((**k).name == "title")
         {
            has_title = true;
            break;
         }
      }

      boost::tiny_xml::element_ptr navbar = make_element("para");
      node->elements.push_back(navbar);

      index_entry_set::const_iterator m = index_entries.begin();
      index_entry_set::const_iterator n = m;
      boost::tiny_xml::element_ptr vlist = make_element("variablelist");
      node->elements.push_back(vlist);
      while(n != index_entries.end())
      {
         char current_letter = std::toupper((*n)->key[0]);
         std::string id_name = get_next_index_id();
         boost::tiny_xml::element_ptr entry = add_attribute(make_element("varlistentry"), "id", id_name);
         boost::tiny_xml::element_ptr term = make_element("term");
         term->content = std::string(1, current_letter);
         entry->elements.push_back(term);
         boost::tiny_xml::element_ptr item = make_element("listitem");
         entry->elements.push_back(item);
         while((n != index_entries.end()) && (std::toupper((*n)->key[0]) == current_letter))
            ++n;
         std::pair<index_entry_set::const_iterator, index_entry_set::const_iterator> range(m, n);
         item->elements.push_back(generate_entry(range, category));
         if(item->elements.size() && (*item->elements.begin())->elements.size())
         {
            vlist->elements.push_back(entry);
            boost::tiny_xml::element_ptr p = make_element("");
            p->content = " ";
            if(navbar->elements.size())
            {
               navbar->elements.push_back(p);
            }
            p = add_attribute(make_element("link"), "linkend", id_name);
            p->content = current_letter;
            navbar->elements.push_back(p);
         }
         m = n;
      }

      node->name = internal_index_type;
      boost::tiny_xml::element_ptr p(node->parent);
      while(p->name.empty())
         p = boost::tiny_xml::element_ptr(p->parent);
      check_index_type_and_placement(p->name, node->name);
      node->attributes.clear();
      if(!has_title)
      {
         boost::tiny_xml::element_ptr t = make_element("title");
         t->content = "Index";
         node->elements.push_front(t);
      }
   }
}


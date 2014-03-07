
#ifndef DATE_TIME_SPECIAL_VALUES_PARSER_HPP__
#define DATE_TIME_SPECIAL_VALUES_PARSER_HPP__

/* Copyright (c) 2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date: 
 */


#include "boost/date_time/string_parse_tree.hpp"
#include "boost/date_time/special_defs.hpp"
#include <string>
#include <vector>

namespace boost { namespace date_time {

  //! Class for special_value parsing 
  /*! 
   * TODO: add doc-comments for which elements can be changed
   * Parses input stream for strings representing special_values. 
   * Special values parsed are:
   *  - not_a_date_time
   *  - neg_infin
   *  - pod_infin
   *  - min_date_time
   *  - max_date_time
   */
  template<class date_type, typename charT>
  class special_values_parser
  {
   public:
    typedef std::basic_string<charT>        string_type;
    //typedef std::basic_stringstream<charT>  stringstream_type;
    typedef std::istreambuf_iterator<charT> stream_itr_type;
    //typedef typename string_type::const_iterator const_itr;
    //typedef typename date_type::year_type  year_type;
    //typedef typename date_type::month_type month_type;
    typedef typename date_type::duration_type duration_type;
    //typedef typename date_type::day_of_week_type day_of_week_type;
    //typedef typename date_type::day_type day_type;
    typedef string_parse_tree<charT> parse_tree_type;
    typedef typename parse_tree_type::parse_match_result_type match_results;
    typedef std::vector<std::basic_string<charT> > collection_type;

    typedef charT char_type;
    static const char_type nadt_string[16];
    static const char_type neg_inf_string[10];
    static const char_type pos_inf_string[10];
    static const char_type min_date_time_string[18];
    static const char_type max_date_time_string[18];
   
    //! Creates a special_values_parser with the default set of "sv_strings"
    special_values_parser()
    {
      sv_strings(string_type(nadt_string),
                 string_type(neg_inf_string),
                 string_type(pos_inf_string),
                 string_type(min_date_time_string),
                 string_type(max_date_time_string));
    }

    //! Creates a special_values_parser using a user defined set of element strings
    special_values_parser(const string_type& nadt_str,
                          const string_type& neg_inf_str,
                          const string_type& pos_inf_str,
                          const string_type& min_dt_str,
                          const string_type& max_dt_str)
    {
      sv_strings(nadt_str, neg_inf_str, pos_inf_str, min_dt_str, max_dt_str);
    }

    special_values_parser(typename collection_type::iterator beg, typename collection_type::iterator end)
    {
      collection_type phrases;
      std::copy(beg, end, std::back_inserter(phrases));
      m_sv_strings = parse_tree_type(phrases, static_cast<int>(not_a_date_time));
    }

    special_values_parser(const special_values_parser<date_type,charT>& svp)
    {
      this->m_sv_strings = svp.m_sv_strings;
    }

    //! Replace special value strings
    void sv_strings(const string_type& nadt_str,
                    const string_type& neg_inf_str,
                    const string_type& pos_inf_str,
                    const string_type& min_dt_str,
                    const string_type& max_dt_str)
    {
      collection_type phrases;
      phrases.push_back(nadt_str);
      phrases.push_back(neg_inf_str);
      phrases.push_back(pos_inf_str);
      phrases.push_back(min_dt_str);
      phrases.push_back(max_dt_str);
      m_sv_strings = parse_tree_type(phrases, static_cast<int>(not_a_date_time));
    }

    /* Does not return a special_value because if the parsing fails, 
     * the return value will always be not_a_date_time 
     * (mr.current_match retains its default value of -1 on a failed 
     * parse and that casts to not_a_date_time). */
    //! Sets match_results.current_match to the corresponding special_value or -1
    bool match(stream_itr_type& sitr, 
                        stream_itr_type& str_end,
                        match_results& mr) const
    {
      unsigned int level = 0;
      m_sv_strings.match(sitr, str_end, mr, level);
      return (mr.current_match != match_results::PARSE_ERROR);
    }
    /*special_values match(stream_itr_type& sitr, 
                        stream_itr_type& str_end,
                        match_results& mr) const
    {
      unsigned int level = 0;
      m_sv_strings.match(sitr, str_end, mr, level);
      if(mr.current_match == match_results::PARSE_ERROR) {
        throw std::ios_base::failure("Parse failed. No match found for '" + mr.cache + "'");
      }
      return static_cast<special_values>(mr.current_match);
    }*/
                     

   private:
    parse_tree_type m_sv_strings;
    
  };

  template<class date_type, class CharT>
  const typename special_values_parser<date_type, CharT>::char_type
  special_values_parser<date_type, CharT>::nadt_string[16] =
  {'n','o','t','-','a','-','d','a','t','e','-','t','i','m','e'};
  template<class date_type, class CharT>
  const typename special_values_parser<date_type, CharT>::char_type
  special_values_parser<date_type, CharT>::neg_inf_string[10] =
  {'-','i','n','f','i','n','i','t','y'};
  template<class date_type, class CharT>
  const typename special_values_parser<date_type, CharT>::char_type
  special_values_parser<date_type, CharT>::pos_inf_string[10] =
  {'+','i','n','f','i','n','i','t','y'};
  template<class date_type, class CharT>
  const typename special_values_parser<date_type, CharT>::char_type
  special_values_parser<date_type, CharT>::min_date_time_string[18] =
  {'m','i','n','i','m','u','m','-','d','a','t','e','-','t','i','m','e'};
  template<class date_type, class CharT>
  const typename special_values_parser<date_type, CharT>::char_type
  special_values_parser<date_type, CharT>::max_date_time_string[18] =
  {'m','a','x','i','m','u','m','-','d','a','t','e','-','t','i','m','e'};

} } //namespace

#endif // DATE_TIME_SPECIAL_VALUES_PARSER_HPP__


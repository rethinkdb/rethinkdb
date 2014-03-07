//  inspect program  -------------------------------------------------------------------//

//  Copyright Beman Dawes 2002.
//  Copyright Rene Rivera 2004-2006.
//  Copyright Gennaro Prota 2006.

//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  This program recurses through sub-directories looking for various problems.
//  It contains some Boost specific features, like ignoring "CVS" and "bin",
//  and the code that identifies library names assumes the Boost directory
//  structure.

//  See http://www.boost.org/tools/inspect/ for more information.

const char* boost_no_inspect = "boost-" "no-inspect";

//  Directories with a file name of the boost_no_inspect value are not inspected.
//  Files that contain the boost_no_inspect value are not inspected.


#include <vector>
#include <list>
#include <algorithm>
#include <cstring>

#include "boost/shared_ptr.hpp"
#include "boost/lexical_cast.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/fstream.hpp"

#include <stdio.h>  // for popen, pclose
#if defined(_MSC_VER)
# define POPEN _popen
# define PCLOSE _pclose
#else
# define POPEN popen
# define PCLOSE pclose
#endif

#include "time_string.hpp"

#include "inspector.hpp"

// the inspectors
#include "copyright_check.hpp"
#include "crlf_check.hpp"
#include "end_check.hpp"
#include "license_check.hpp"
#include "link_check.hpp"
#include "path_name_check.hpp"
#include "tab_check.hpp"
#include "ascii_check.hpp"
#include "apple_macro_check.hpp"
#include "assert_macro_check.hpp"
#include "deprecated_macro_check.hpp"
#include "minmax_check.hpp"
#include "unnamed_namespace_check.hpp"

#include "cvs_iterator.hpp"

#if !defined(INSPECT_USE_BOOST_TEST)
#define INSPECT_USE_BOOST_TEST 0
#endif

#if INSPECT_USE_BOOST_TEST
#include "boost/test/included/prg_exec_monitor.hpp"
#endif

namespace fs = boost::filesystem;

using namespace boost::inspect;

namespace
{
  class inspector_element
  {
    typedef boost::shared_ptr< boost::inspect::inspector > inspector_ptr;

  public:
    inspector_ptr  inspector;
    explicit
    inspector_element( boost::inspect::inspector * p ) : inspector(p) {}
  };

  typedef std::list< inspector_element > inspector_list;

  long file_count = 0;
  long directory_count = 0;
  long error_count = 0;
  const int max_offenders = 5;  // maximum "worst offenders" to display 

  boost::inspect::string_set content_signatures;

  struct error_msg
  {
    string library;
    string rel_path;
    string msg;
    int    line_number;

    bool operator<( const error_msg & rhs ) const
    {
      if ( library < rhs.library ) return true;
      if ( library > rhs.library ) return false;
      if ( rel_path < rhs.rel_path ) return true;
      if ( rel_path > rhs.rel_path ) return false;
      if ( line_number < rhs.line_number ) return true;
      if ( line_number > rhs.line_number ) return false;
      return msg < rhs.msg;
    }
  };

  typedef std::vector< error_msg > error_msg_vector;
  error_msg_vector msgs;

  struct lib_error_count
  {
    int     error_count;
    string  library;

    bool operator<( const lib_error_count & rhs ) const
    {
      return error_count > rhs.error_count;
    }
  };

  typedef std::vector< lib_error_count > lib_error_count_vector;
  lib_error_count_vector libs;

//  run subversion to get revisions info  ------------------------------------//
//
// implemented as function object that can be passed to boost::execution_monitor
// in order to swallow any errors from 'svn info'.

  struct svn_check
  {
    explicit svn_check(const fs::path & inspect_root) :
      inspect_root(inspect_root), fp(0) {}

    int operator()() {
      string rev("unknown");
      string repos("unknown");
      string command("cd ");
      command += inspect_root.string() + " && svn info";

      fp = (POPEN(command.c_str(), "r"));
      if (fp)
      {
        static const int line_max = 128;
        char line[line_max];
        while (fgets(line, line_max, fp) != NULL)
        {
          string ln(line);
          string::size_type pos;
          if ((pos = ln.find("Revision: ")) != string::npos)
            rev = ln.substr(pos + 10);
          else if ((pos = ln.find("URL: ")) != string::npos)
            repos = ln.substr(pos + 5);
        }
      }

      result = repos + " at revision " + rev;
      return 0;
    }

    ~svn_check() { if (fp) PCLOSE(fp); }

    const fs::path & inspect_root;
    std::string result;
    FILE* fp;
  private:
    svn_check(svn_check const&);
    svn_check const& operator=(svn_check const&);
  };

  // Small helper class because svn_check can't be passed by copy.
  template <typename F, typename R>
  struct nullary_function_ref
  {
    explicit nullary_function_ref(F& f) : f(f) {}
    R operator()() const { return f(); }
    F& f;
  };

//  get info (as a string) if inspect_root is svn working copy  --------------//

  string info( const fs::path & inspect_root )
  {
    svn_check check(inspect_root);

#if !INSPECT_USE_BOOST_TEST
    check();
#else

    try {
      boost::execution_monitor e;
      e.execute(nullary_function_ref<svn_check, int>(check));
    }
    catch(boost::execution_exception const& e) {
      if (e.code() == boost::execution_exception::system_error) {
        // There was an error running 'svn info' - it probably
        // wasn't run in a subversion repo.
        return string("unknown");
      }
      else {
        throw;
      }
    }

#endif

    return check.result;
  }

//  visit_predicate (determines which directories are visited)  --------------//

  typedef bool(*pred_type)(const path&);

  bool visit_predicate( const path & pth )
  {
    string local( boost::inspect::relative_to( pth, fs::initial_path() ) );
    string leaf( pth.leaf().string() );
    return
      // so we can inspect a checkout
      leaf != "CVS"
      // don't look at binaries
      && leaf != "bin"
      && leaf != "bin.v2"
      // no point in checking doxygen xml output
      && local.find("doc/xml") != 0
      && local.find("doc\\xml") != 0
      // ignore some web files
      && leaf != ".htaccess"
      // ignore svn files:
      && leaf != ".svn"
      // ignore other version control files
      && leaf != ".git"
      && leaf != ".bzr"
      // ignore OS X directory info files:
      && leaf != ".DS_Store"
      // ignore if tag file present
      && !boost::filesystem::exists(pth / boost_no_inspect)
      ;
  }

//  library_from_content  ----------------------------------------------------//

  string library_from_content( const string & content )
  {
    const string unknown_library ( "unknown" );
    const string lib_root ( "www.boost.org/libs/" );
    string::size_type pos( content.find( lib_root ) );

    string lib = unknown_library;

    if ( pos != string::npos ) {

        pos += lib_root.length();

        const char delims[] = " " // space and...
                              "/\n\r\t";

        string::size_type n = content.find_first_of( string(delims), pos );
        if (n != string::npos)
            lib = string(content, pos, n - pos);
        
    }

    return lib;
  }

//  find_signature  ----------------------------------------------------------//

  bool find_signature( const path & file_path,
    const boost::inspect::string_set & signatures )
  {
    string name( file_path.leaf().string() );
    if ( signatures.find( name ) == signatures.end() )
    {
      string::size_type pos( name.rfind( '.' ) );
      if ( pos == string::npos
        || signatures.find( name.substr( pos ) )
          == signatures.end() ) return false;
    }
    return true;
  }

//  load_content  ------------------------------------------------------------//

  void load_content( const path & file_path, string & target )
  {
    target = "";

    if ( !find_signature( file_path, content_signatures ) ) return;

    fs::ifstream fin( file_path, std::ios_base::in|std::ios_base::binary );
    if ( !fin )
      throw string( "could not open input file: " ) + file_path.string();
    std::getline( fin, target, '\0' ); // read the whole file
  }

//  check  -------------------------------------------------------------------//

  void check( const string & lib,
    const path & pth, const string & content, const inspector_list & insp_list )
  {
    // invoke each inspector
    for ( inspector_list::const_iterator itr = insp_list.begin();
      itr != insp_list.end(); ++itr )
    {
      itr->inspector->inspect( lib, pth ); // always call two-argument form
      if ( find_signature( pth, itr->inspector->signatures() ) )
      {
          itr->inspector->inspect( lib, pth, content );
      }
    }
  }

//  visit_all  ---------------------------------------------------------------//

  template< class DirectoryIterator >
  void visit_all( const string & lib,
    const path & dir_path, const inspector_list & insps )
  {
    static DirectoryIterator end_itr;
    ++directory_count;

    for ( DirectoryIterator itr( dir_path ); itr != end_itr; ++itr )
    {

      if ( fs::is_directory( *itr ) )
      {
        if ( visit_predicate( *itr ) )
        {
          string cur_lib( boost::inspect::impute_library( *itr ) );
          check( cur_lib, *itr, "", insps );
          visit_all<DirectoryIterator>( cur_lib, *itr, insps );
        }
      }
      else
      {
        ++file_count;
        string content;
        load_content( *itr, content );
        if (content.find(boost_no_inspect) == string::npos)
          check( lib.empty() ? library_from_content( content ) : lib,
                 *itr, content, insps );
      }
    }
  }

//  display  -----------------------------------------------------------------//

  enum display_format_type
  {
    display_html, display_text
  }
  display_format = display_html;

  enum display_mode_type
  {
    display_full, display_brief
  }
  display_mode = display_full;

//  display_summary_helper  --------------------------------------------------//

  void display_summary_helper( const string & current_library, int err_count )
  {
    if (display_text == display_format)
    {
        std::cout << "  " << current_library << " (" << err_count << ")\n";
    }
    else
    {
      std::cout
        << "  <a href=\"#"
        << current_library          // what about malformed for URI refs? [gps]
        << "\">" << current_library
        << "</a> ("
        << err_count << ")<br>\n";
    }
  }

//  display_summary  ---------------------------------------------------------//

  void display_summary()
  {
    if (display_text == display_format)
    {
      std::cout << "Summary:\n";
    }
    else
    {
      std::cout <<
        "<h2>Summary</h2>\n"
        "<blockquote>\n"
        ;
    }

    string current_library( msgs.begin()->library );
    int err_count = 0;
    for ( error_msg_vector::iterator itr ( msgs.begin() );
      itr != msgs.end(); ++itr )
    {
      if ( current_library != itr->library )
      {
        display_summary_helper( current_library, err_count );
        current_library = itr->library;
        err_count = 0;
      }
      ++err_count;
    }
    display_summary_helper( current_library, err_count );

    if (display_text == display_format)
      std::cout << "\n";
    else
      std::cout << "</blockquote>\n"; 
  }

//  html_encode  -------------------------------------------------------------//

  std::string html_encode(std::string const& text)
  {
    std::string result;
    
    for(std::string::const_iterator it = text.begin(),
        end = text.end(); it != end; ++it)
    {
      switch(*it) {
      case '<':
        result += "&lt;";
        break;
      case '>':
        result += "&gt;";
        break;
      case '&':
        result += "&amp;";
        break;
      default:
        result += *it;
      }      
    }
    
    return result;
  }

//  display_details  ---------------------------------------------------------//

  void display_details()
  {
    if (display_text == display_format)
    {
      // display error messages with group indication
      error_msg current;
      string sep;
      for ( error_msg_vector::iterator itr ( msgs.begin() );
        itr != msgs.end(); ++itr )
      {
        if ( current.library != itr->library )
        {
          if ( display_full == display_mode )
              std::cout << "\n|" << itr->library << "|\n";
          else
              std::cout << "\n\n|" << itr->library << '|';
        }

        if ( current.library != itr->library
          || current.rel_path != itr->rel_path )
        {
          if ( display_full == display_mode )
          {
            std::cout << "  " << itr->rel_path << ":\n";
          }
          else
          {
            path current_rel_path(current.rel_path);
            path this_rel_path(itr->rel_path);
            if (current_rel_path.branch_path() != this_rel_path.branch_path())
            {
              std::cout << "\n  " << this_rel_path.branch_path().string() << '/';
            }
            std::cout << "\n    " << this_rel_path.leaf() << ':';
          }
        }
        if ( current.library != itr->library
          || current.rel_path != itr->rel_path
          || current.msg != itr->msg )
        {
          const string m = itr->msg;

          if ( display_full == display_mode )
              std::cout << "    " << m << '\n';
          else
              std::cout << ' ' << m;
        }
        current.library = itr->library;
        current.rel_path = itr->rel_path;
        current.msg = itr->msg;
      }
      std::cout << "\n";
    }
    else  // html
    {
      // display error messages with group indication
      error_msg current;
      bool first_sep = true;
      bool first = true;
      for ( error_msg_vector::iterator itr ( msgs.begin() );
        itr != msgs.end(); ++itr )
      {
        if ( current.library != itr->library )
        {
          if ( !first ) std::cout << "</pre>\n";
          std::cout << "\n<h3><a name=\"" << itr->library
                    << "\">" << itr->library << "</a></h3>\n<pre>";
        }
        if ( current.library != itr->library
          || current.rel_path != itr->rel_path )
        {
          std::cout << "\n";
          std::cout << itr->rel_path;
          first_sep = true;
        }
        if ( current.library != itr->library
          || current.rel_path != itr->rel_path
          || current.msg != itr->msg )
        {
          std::string sep;
          if (first_sep)
            if (itr->line_number) sep = ":<br>&nbsp;&nbsp;&nbsp; ";
            else sep = ": ";
          else
            if (itr->line_number) sep = "<br>&nbsp;&nbsp;&nbsp; ";
            else sep = ", ";

          // print the message
          if (itr->line_number)
            std::cout << sep << "(line " << itr->line_number << ") " << html_encode(itr->msg);
          else std::cout << sep << html_encode(itr->msg);

          first_sep = false;
        }
        current.library = itr->library;
        current.rel_path = itr->rel_path;
        current.msg = itr->msg;
        first = false;
      }
      std::cout << "</pre>\n";
    }
  }


//  worst_offenders_count_helper  --------------------------------------------------//

  void worst_offenders_count_helper( const string & current_library, int err_count )
  {
        lib_error_count lec;
        lec.library = current_library;
        lec.error_count = err_count;
        libs.push_back( lec );
  }
//  worst_offenders_count  -----------------------------------------------------//

  void worst_offenders_count()
  {
    if ( msgs.empty() )
    {
      return;
    }
    string current_library( msgs.begin()->library );
    int err_count = 0;
    for ( error_msg_vector::iterator itr ( msgs.begin() );
      itr != msgs.end(); ++itr )
    {
      if ( current_library != itr->library )
      {
        worst_offenders_count_helper( current_library, err_count );
        current_library = itr->library;
        err_count = 0;
      }
      ++err_count;
    }
    worst_offenders_count_helper( current_library, err_count );
  }

//  display_worst_offenders  -------------------------------------------------//

  void display_worst_offenders()
  {
    if (display_text == display_format)
    {
      std::cout << "Worst Offenders:\n";
    }
    else
    {
      std::cout <<
        "<h2>Worst Offenders</h2>\n"
        "<blockquote>\n"
        ;
    }

    int display_count = 0;
    int last_error_count = 0;
    for ( lib_error_count_vector::iterator itr ( libs.begin() );
          itr != libs.end()
            && (display_count < max_offenders
                || itr->error_count == last_error_count);
          ++itr, ++display_count )
    {
      if (display_text == display_format)
      {
        std::cout << itr->library << " " << itr->error_count << "\n";
      }
      else
      {
        std::cout
          << "  <a href=\"#"
          << itr->library
          << "\">" << itr->library
          << "</a> ("
          << itr->error_count << ")<br>\n";
      }
      last_error_count = itr->error_count;
    }

    if (display_text == display_format)
      std::cout << "\n";
    else
      std::cout << "</blockquote>\n"; 
  }


  const char * options()
  {
    return
         "  -license\n"
         "  -copyright\n"
         "  -crlf\n"
         "  -end\n"
         "  -link\n"
         "  -path_name\n"
         "  -tab\n"
         "  -ascii\n"
         "  -apple_macro\n"
         "  -assert_macro\n"
         "  -deprecated_macro\n"
         "  -minmax\n"
         "  -unnamed\n"
         " default is all checks on; otherwise options specify desired checks"
         "\n";
  }

  const char * doctype_declaration()
  {
    return
         "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
         "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"
         ;
  }

  std::string validator_link(const std::string & text)
  {
    return
        // with link to validation service
        "<a href=\"http://validator.w3.org/check?uri=referer\">"
        + text
        + "</a>"
        ;
  }

} // unnamed namespace

namespace boost
{
  namespace inspect
  {

//  line_break  --------------------------------------------------------------//

    const char * line_break()
    {
      return display_format ? "\n" : "<br>\n";
    }


//  register_signature  ------------------------------------------------------//

    void inspector::register_signature( const string & signature )
    {
      m_signatures.insert( signature );
      content_signatures.insert( signature );
    }

//  error  -------------------------------------------------------------------//

    void inspector::error( const string & library_name,
      const path & full_path, const string & msg, int line_number )
    {
      ++error_count;
      error_msg err_msg;
      err_msg.library = library_name;
      err_msg.rel_path = relative_to( full_path, fs::initial_path() );
      err_msg.msg = msg;
      err_msg.line_number = line_number;
      msgs.push_back( err_msg );

//     std::cout << library_name << ": "
//        << full_path.string() << ": "
//        << msg << '\n';

    }

    source_inspector::source_inspector()
    {
      // C/C++ source code...
      register_signature( ".c" );
      register_signature( ".cpp" );
      register_signature( ".css" );
      register_signature( ".cxx" );
      register_signature( ".h" );
      register_signature( ".hpp" );
      register_signature( ".hxx" );
      register_signature( ".inc" );
      register_signature( ".ipp" );

      // Boost.Build BJam source code...
      register_signature( "Jamfile" );
      register_signature( ".jam" );
      register_signature( ".v2" );

      // Other scripts; Python, shell, autoconfig, etc.
      register_signature( "configure.in" );
      register_signature( "GNUmakefile" );
      register_signature( "Makefile" );
      register_signature( ".bat" );
      register_signature( ".mak" );
      register_signature( ".pl" );
      register_signature( ".py" );
      register_signature( ".sh" );

      // Hypertext, Boost.Book, and other text...
      register_signature( "news" );
      register_signature( "readme" );
      register_signature( "todo" );
      register_signature( "NEWS" );
      register_signature( "README" );
      register_signature( "TODO" );
      register_signature( ".boostbook" );
      register_signature( ".htm" );
      register_signature( ".html" );
      register_signature( ".rst" );
      register_signature( ".sgml" );
      register_signature( ".shtml" );
      register_signature( ".txt" );
      register_signature( ".xml" );
      register_signature( ".xsd" );
      register_signature( ".xsl" );
      register_signature( ".qbk" );
    }

    hypertext_inspector::hypertext_inspector()
    {
      register_signature( ".htm" );
      register_signature( ".html" );
      register_signature( ".shtml" );
    }

//  impute_library  ----------------------------------------------------------//

    // may return an empty string [gps]
    string impute_library( const path & full_dir_path )
    {
      path relative( relative_to( full_dir_path, fs::initial_path() ) );
      if ( relative.empty() ) return "boost-root";
      string first( (*relative.begin()).string() );
      string second =  // borland 5.61 requires op=
        ++relative.begin() == relative.end()
          ? string() : (*++relative.begin()).string();

      if ( first == "boost" )
        return second;

      return (( first == "libs" || first == "tools" ) && !second.empty())
        ? second : first;
    }

  } // namespace inspect
} // namespace boost

//  cpp_main()  --------------------------------------------------------------//

#if !INSPECT_USE_BOOST_TEST
int main( int argc_param, char * argv_param[] )
#else
int cpp_main( int argc_param, char * argv_param[] )
#endif
{
  // <hack> for the moment, let's be on the safe side
  // and ensure we don't modify anything being pointed to;
  // then we'll do some cleanup here
  int argc = argc_param;
  const char* const * argv = &argv_param[0];

  if ( argc > 1 && (std::strcmp( argv[1], "-help" ) == 0
    || std::strcmp( argv[1], "--help" ) == 0 ) )
  {
    std::clog << "Usage: inspect [-cvs] [-text] [-brief] [options...]\n\n"
      " Options:\n"
      << options() << '\n';
    return 0;
  }

  bool license_ck = true;
  bool copyright_ck = true;
  bool crlf_ck = true;
  bool end_ck = true;
  bool link_ck = true;
  bool path_name_ck = true;
  bool tab_ck = true;
  bool ascii_ck = true;
  bool apple_ck = true;
  bool assert_ck = true;
  bool deprecated_ck = true;
  bool minmax_ck = true;
  bool unnamed_ck = true;
  bool cvs = false;

  if ( argc > 1 && std::strcmp( argv[1], "-cvs" ) == 0 )
  {
    cvs = true;
    --argc; ++argv;
  }

  if ( argc > 1 && std::strcmp( argv[1], "-text" ) == 0 )
  {
    display_format = display_text;
    --argc; ++argv;
  }

  if ( argc > 1 && std::strcmp( argv[1], "-brief" ) == 0 )
  {
    display_mode = display_brief;
    --argc; ++argv;
  }

  if ( argc > 1 && *argv[1] == '-' )
  {
    license_ck = false;
    copyright_ck = false;
    crlf_ck = false;
    end_ck = false;
    link_ck = false;
    path_name_ck = false;
    tab_ck = false;
    ascii_ck = false;
    apple_ck = false;
    assert_ck = false;
    deprecated_ck = false;
    minmax_ck = false;
    unnamed_ck = false;
  }

  bool invalid_options = false;
  for(; argc > 1; --argc, ++argv )
  {
    if ( std::strcmp( argv[1], "-license" ) == 0 )
      license_ck = true;
    else if ( std::strcmp( argv[1], "-copyright" ) == 0 )
      copyright_ck = true;
    else if ( std::strcmp( argv[1], "-crlf" ) == 0 )
        crlf_ck = true;
    else if ( std::strcmp( argv[1], "-end" ) == 0 )
        end_ck = true;
    else if ( std::strcmp( argv[1], "-link" ) == 0 )
      link_ck = true;
    else if ( std::strcmp( argv[1], "-path_name" ) == 0 )
      path_name_ck = true;
    else if ( std::strcmp( argv[1], "-tab" ) == 0 )
      tab_ck = true;
    else if ( std::strcmp( argv[1], "-ascii" ) == 0 )
      ascii_ck = true;
    else if ( std::strcmp( argv[1], "-apple_macro" ) == 0 )
      apple_ck = true;
    else if ( std::strcmp( argv[1], "-assert_macro" ) == 0 )
      assert_ck = true;
    else if ( std::strcmp( argv[1], "-deprecated_macro" ) == 0 )
      deprecated_ck = true;
    else if ( std::strcmp( argv[1], "-minmax" ) == 0 )
        minmax_ck = true;
    else if ( std::strcmp( argv[1], "-unnamed" ) == 0 )
        unnamed_ck = true;
    else
    {
      std::cerr << "unknown option: " << argv[1] << '\n';
      invalid_options = true;
    }
  }
  if ( invalid_options ) {
      std::cerr << "\nvalid options are:\n"
                << options();
      return 1;
  }

  string inspector_keys;
  fs::initial_path();
  

  { // begin reporting block

  // since this is in its own block; reporting will happen
  // automatically, from each registered inspector, when
  // leaving, due to destruction of the inspector_list object
  inspector_list inspectors;

  if ( license_ck )
    inspectors.push_back( inspector_element( new boost::inspect::license_check ) );
  if ( copyright_ck )
    inspectors.push_back( inspector_element( new boost::inspect::copyright_check ) );
  if ( crlf_ck )
    inspectors.push_back( inspector_element( new boost::inspect::crlf_check ) );
  if ( end_ck )
    inspectors.push_back( inspector_element( new boost::inspect::end_check ) );
  if ( link_ck )
    inspectors.push_back( inspector_element( new boost::inspect::link_check ) );
  if ( path_name_ck )
    inspectors.push_back( inspector_element( new boost::inspect::file_name_check ) );
  if ( tab_ck )
      inspectors.push_back( inspector_element( new boost::inspect::tab_check ) );
  if ( ascii_ck )
      inspectors.push_back( inspector_element( new boost::inspect::ascii_check ) );
  if ( apple_ck )
      inspectors.push_back( inspector_element( new boost::inspect::apple_macro_check ) );
  if ( assert_ck )
      inspectors.push_back( inspector_element( new boost::inspect::assert_macro_check ) );
  if ( deprecated_ck )
      inspectors.push_back( inspector_element( new boost::inspect::deprecated_macro_check ) );
  if ( minmax_ck )
      inspectors.push_back( inspector_element( new boost::inspect::minmax_check ) );
  if ( unnamed_ck )
      inspectors.push_back( inspector_element( new boost::inspect::unnamed_namespace_check ) );

  // perform the actual inspection, using the requested type of iteration
  if ( cvs )
    visit_all<hack::cvs_iterator>( "boost-root",
      fs::initial_path(), inspectors );
  else
    visit_all<fs::directory_iterator>( "boost-root",
      fs::initial_path(), inspectors );

  // close
  for ( inspector_list::iterator itr = inspectors.begin();
        itr != inspectors.end(); ++itr )
  {
    itr->inspector->close();
  }

  string run_date ( "n/a" );
  boost::time_string( run_date );

  if (display_text == display_format)
  {
    std::cout
      <<
        "Boost Inspection Report\n"
        "Run Date: " << run_date  << "\n"
        "\n"
        "An inspection program <http://www.boost.org/tools/inspect/index.html>\n"
        "checks each file in the current Boost CVS for various problems,\n"
        "generating an HTML page as output.\n"
        "\n"
      ;

    std::cout
      << "Totals:\n"
      << "  " << file_count << " files scanned\n"
      << "  " << directory_count << " directories scanned (including root)\n"
      << "  " << error_count << " problems reported\n"
      << '\n'
      ;
  }
  else
  {
    //
    std::cout << doctype_declaration() << '\n';

    std::cout
      << "<html>\n"
      "<head>\n"
      "<style> body { font-family: sans-serif; } </style>\n"
      "<title>Boost Inspection Report</title>\n"
      "</head>\n"

      "<body>\n"
      // we should not use a table, of course [gps]
      "<table>\n"
      "<tr>\n"
      "<td><img src=\"http://www.boost.org/boost.png\" alt=\"Boost logo\" />"
      "</td>\n"
      "<td>\n"
      "<h1>Boost Inspection Report</h1>\n"
      "<b>Run Date:</b> " << run_date  << "\n"
      //"&nbsp;&nbsp;/ " << validator_link( "validate me" ) << " /\n"
      "</td>\n"
      "</tr>\n"
      "</table>\n"

      "<p>This report is generated by an <a href=\"http://www.boost.org/tools/inspect/index.html\">inspection\n"
      "program</a> that checks files for the problems noted below.</p>\n"
      ;
    std::cout
      << "<p>The files checked were from "
      << info( fs::initial_path() )
      << ".</p>\n";


    std::cout
      << "<h2>Totals</h2>\n"
      << file_count << " files scanned<br>\n"
      << directory_count << " directories scanned (including root)<br>\n"
      << error_count << " problems reported\n<p>";
  }

  for ( inspector_list::iterator itr = inspectors.begin();
        itr != inspectors.end(); ++itr )
  {

    inspector_keys += static_cast<string>("  ")
        + itr->inspector->name()
        + ' ' + itr->inspector->desc()
        + line_break()
        ;
  }

  if (display_text == display_format)
     std::cout << "\nProblem counts:\n";
  else
    std::cout << "\n<h2>Problem counts</h2>\n<blockquote><p>\n" ;

  } // end of block: starts reporting

  if (display_text == display_format)
    std::cout << "\n" ;
  else
    std::cout << "</blockquote>\n";

  std::sort( msgs.begin(), msgs.end() );

  worst_offenders_count();
  std::stable_sort( libs.begin(), libs.end() );

  if ( !libs.empty() )
    display_worst_offenders();

  if ( !msgs.empty() )
  {
    display_summary();

    if (display_text == display_format)
    {
      std::cout << "Details:\n" << inspector_keys;
      std::cout << "\nDirectories with a file named \"" << boost_no_inspect << "\" will not be inspected.\n"
                   "Files containing \"" << boost_no_inspect << "\" will not be inspected.\n";
   }
    else
    {
      std::cout << "<h2>Details</h2>\n" << inspector_keys;
      std::cout << "\n<p>Directories with a file named \"" << boost_no_inspect << "\" will not be inspected.<br>\n"
                   "Files containing \"" << boost_no_inspect << "\" will not be inspected.</p>\n";
    }
    display_details();
  }

  if (display_text == display_format)
  {
    std::cout << "\n\n" ;
  }
  else
  {
    std::cout
      << "</body>\n"
      "</html>\n";
  }

  return 0;
}

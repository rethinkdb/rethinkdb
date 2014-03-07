//  Generate Library Status HTML from jam regression test output  -----------//

//  Copyright Bryce Lelbach 2011 
//  Copyright Beman Dawes 2002-2011.

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//  See http://www.boost.org/tools/regression/ for documentation.

//Note: This version of the original program builds a large table
//which includes all build variations such as build/release, static/dynamic, etc.


/*******************************************************************************

This program was designed to work unchanged on all platforms and
configurations.  All output which is platform or configuration dependent
is obtained from external sources such as the .xml file from
process_jam_log execution, the tools/build/xxx-tools.jam files, or the
output of the config_info tests.

Please avoid adding platform or configuration dependencies during
program maintenance.

*******************************************************************************/

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>

namespace fs = boost::filesystem;

#include "detail/tiny_xml.hpp"
namespace xml = boost::tiny_xml;

#include <boost/iterator/transform_iterator.hpp>

#include <cstdlib>  // for abort, exit
#include <string>
#include <vector>
#include <set>
#include <utility>  // for make_pair on STLPort
#include <map>
#include <algorithm> // max_element, find_if
#include <iostream>
#include <fstream>
#include <ctime>
#include <stdexcept>
#include <cassert>
#include <utility> // for pair

using std::string;

const string pass_msg( "Pass" );
const string warn_msg( "<i>Warn</i>" );
const string fail_msg( "<font color=\"#FF0000\"><i>Fail</i></font>" );
const string missing_residue_msg( "<i>Missing</i>" );

const std::size_t max_compile_msg_size = 10000;

namespace
{
    fs::path locate_root; // locate-root (AKA ALL_LOCATE_TARGET) complete path
    bool ignore_pass = false;
    bool no_warn = false;
    bool no_links = false;

    // transform pathname to something html can accept
    struct char_xlate {
        typedef char result_type;
        result_type operator()(char c) const{
            if(c == '/' || c == '\\')
                return '-';
            return c;
        }
    };
    typedef boost::transform_iterator<char_xlate, std::string::const_iterator> html_from_path; 

    template<class I1, class I2>
    std::ostream & operator<<(
    std::ostream &os, 
    std::pair<I1, I2> p
    ){
        while(p.first != p.second)
            os << *p.first++;
        return os;
    }

    struct col_node {
        int rows, cols;
        bool is_leaf;
        typedef std::pair<const std::string, col_node> subcolumn;
        typedef std::map<std::string, col_node> subcolumns_t;
        subcolumns_t m_subcolumns;
        bool operator<(const col_node &cn) const;
        col_node() :
            is_leaf(false)
        {}
        std::pair<int, int> get_spans();
    };

    std::pair<int, int> col_node::get_spans(){
        rows = 1;
        cols = 0;
        if(is_leaf){
            cols = 1;
        }
        if(! m_subcolumns.empty()){
            BOOST_FOREACH(
                subcolumn & s,
                m_subcolumns
            ){
                std::pair<int, int> spans;
                spans = s.second.get_spans();
                rows = (std::max)(rows, spans.first);
                cols += spans.second;
            }
            ++rows;
        }
        return std::make_pair(rows, cols);
    }

    void build_node_tree(const fs::path & dir_root, col_node & node){
        bool has_directories = false;
        bool has_files = false;
        BOOST_FOREACH(
            fs::directory_entry & d,
            std::make_pair(
                fs::directory_iterator(dir_root), 
                fs::directory_iterator()
            )
         ){
             if(fs::is_directory(d)){
                has_directories = true;
                std::pair<col_node::subcolumns_t::iterator, bool> result 
                    = node.m_subcolumns.insert(
                        std::make_pair(d.path().filename().string(), col_node())
                    );
                build_node_tree(d, result.first->second);
             }
             else{
                 has_files = true;
             }
        }
        if(has_directories && has_files)
            throw std::string("invalid bin directory structure");
        node.is_leaf = has_files;
    }

    fs::ofstream report;
    fs::ofstream links_file;
    string links_name;

    string specific_compiler; // if running on one toolset only

    const string empty_string;

    //  extract object library name from target directory string  ----------------//

    string extract_object_library_name( const string & s )
    {
        string t( s );
        string::size_type pos = t.find( "/build/" );
        if ( pos != string::npos ) pos += 7;
        else if ( (pos = t.find( "/test/" )) != string::npos ) pos += 6;
        else return "";
        return t.substr( pos, t.find( "/", pos ) - pos );
    }

    //  find_element  ------------------------------------------------------------//

    struct element_equal {
        const string & m_name;
        element_equal(const string & name) :
            m_name(name)
        {}
        bool operator()(const xml::element_ptr & xep) const {
            return xep.get()->name == m_name;
        }
    };

    xml::element_list::const_iterator find_element(
        const xml::element & root, const string & name 
    ){
        return std::find_if(
            root.elements.begin(), 
            root.elements.end(), 
            element_equal(name)
        );
    }

    //  element_content  ---------------------------------------------------------//
    const string & element_content(
        const xml::element & root, const string & name
    ){ 
        xml::element_list::const_iterator itr;
        itr = find_element(root, name);
        if(root.elements.end() == itr)
            return empty_string;
        return (*itr)->content;
    }

    //  attribute_value  ----------------------------------------------------------//

    struct attribute_equal {
        const string & m_name;
        attribute_equal(const string & name) :
            m_name(name)
        {}
        bool operator()(const xml::attribute & a) const {
            return a.name == m_name;
        }
    };

    const string & attribute_value( 
        const xml::element & element,
        const string & attribute_name 
    ){
        xml::attribute_list::const_iterator itr;
        itr = std::find_if(
            element.attributes.begin(), 
            element.attributes.end(), 
            attribute_equal(attribute_name)
        );
        if(element.attributes.end() == itr){
            static const string empty_string;
            return empty_string;
        }
        return itr->value;
    }

    //  generate_report  ---------------------------------------------------------//

    // return 0 if nothing generated, 1 otherwise, except 2 if compiler msgs
    int generate_report( 
        const xml::element & db,
        const std::string source_library_name,
        const string & test_type,
        const fs::path & target_dir,
        bool pass,
        bool always_show_run_output 
        )
    {
        // compile msgs sometimes modified, so make a local copy
        string compile( ((pass && no_warn)
            ? empty_string :  element_content( db, "compile" )) );

        const string & link( pass ? empty_string : element_content( db, "link" ) );
        const string & run( (pass && !always_show_run_output)
            ? empty_string : element_content( db, "run" ) );
        string lib( (pass ? empty_string : element_content( db, "lib" )) );

        // some compilers output the filename even if there are no errors or
        // warnings; detect this if one line of output and it contains no space.
        string::size_type pos = compile.find( '\n', 1 );
        if ( pos != string::npos && compile.size()-pos <= 2
            && compile.find( ' ' ) == string::npos ) compile.clear();

        if ( lib.empty() 
            && (
                compile.empty() || test_type == "compile_fail"
            ) 
            && link.empty() 
            && run.empty()
        ) 
            return 0; 

        int result = 1; // some kind of msg for sure

        // limit compile message length
        if ( compile.size() > max_compile_msg_size )
        {
            compile.erase( max_compile_msg_size );
            compile += "...\n   (remainder deleted because of excessive size)\n";
        }

        const string target_dir_string = target_dir.string();

        links_file << "<h2><a name=\"";
        links_file << std::make_pair(
            html_from_path(target_dir_string.begin()), 
            html_from_path(target_dir_string.end())
            )
            << "\">"
            << std::make_pair(
            html_from_path(target_dir_string.begin()), 
            html_from_path(target_dir_string.end())
            )
            ;
        links_file << "</a></h2>\n";;

        if ( !compile.empty() )
        {
            ++result;
            links_file << "<h3>Compiler output:</h3><pre>"
                << compile << "</pre>\n";
        }
        if ( !link.empty() )
            links_file << "<h3>Linker output:</h3><pre>" << link << "</pre>\n";
        if ( !run.empty() )
            links_file << "<h3>Run output:</h3><pre>" << run << "</pre>\n";

        // for an object library failure, generate a reference to the object
        // library failure message, and (once only) generate the object
        // library failure message itself
        static std::set< string > failed_lib_target_dirs; // only generate once
        if ( !lib.empty() )
        {
            if ( lib[0] == '\n' ) lib.erase( 0, 1 );
            string object_library_name( extract_object_library_name( lib ) );

            // changing the target directory naming scheme breaks
            // extract_object_library_name()
            assert( !object_library_name.empty() );
            if ( object_library_name.empty() )
                std::cerr << "Failed to extract object library name from " << lib << "\n";

            links_file << "<h3>Library build failure: </h3>\n"
                "See <a href=\"#"
                << source_library_name << "-"
                << object_library_name << "-" 
                << std::make_pair(
                html_from_path(target_dir_string.begin()), 
                html_from_path(target_dir_string.end())
                )
                << source_library_name << " - "
                << object_library_name << " - " 
                << std::make_pair(
                html_from_path(target_dir_string.begin()), 
                html_from_path(target_dir_string.end())
                )
                << "</a>";
            if ( failed_lib_target_dirs.find( lib ) == failed_lib_target_dirs.end() )
            {
                failed_lib_target_dirs.insert( lib );
                fs::path pth( locate_root / lib / "test_log.xml" );
                fs::ifstream file( pth );
                if ( file )
                {
                    xml::element_ptr db = xml::parse( file, pth.string() );
                    generate_report( 
                        *db, 
                        source_library_name, 
                        test_type,
                        target_dir,
                        false,
                        false
                    );
                }
                else
                {
                    links_file << "<h2><a name=\""
                        << object_library_name << "-" 
                        << std::make_pair(
                        html_from_path(target_dir_string.begin()), 
                        html_from_path(target_dir_string.end())
                        )
                        << "\">"
                        << object_library_name << " - " 
                        << std::make_pair(
                        html_from_path(target_dir_string.begin()), 
                        html_from_path(target_dir_string.end())
                        )
                        << "</a></h2>\n"
                        << "test_log.xml not found\n";
                }
            }
        }
        return result;
    }

    struct has_fail_result {
        //bool operator()(const boost::shared_ptr<const xml::element> e) const {
        bool operator()(const xml::element_ptr & e) const {
            return attribute_value(*e, "result") == "fail";
        }
    };

    //  do_cell  ---------------------------------------------------------------//
    bool do_cell(
        const fs::path & target_dir,
        const string & lib_name,
        const string & test_name,
        string & target,
        bool profile
    ){
        // return true if any results except pass_msg
        bool pass = false;

        fs::path xml_file_path( target_dir / "test_log.xml" );
        if ( !fs::exists( xml_file_path ) )
        {
            fs::path test_path = target_dir / (test_name + ".test");
            target += "<td align=\"right\">";
            target += fs::exists( test_path) ? pass_msg : fail_msg;
            target += "</td>";
            return true;
        }


        string test_type( "unknown" );
        bool always_show_run_output( false );

        fs::ifstream file( xml_file_path );
        xml::element_ptr dbp = xml::parse( file, xml_file_path.string() );
        const xml::element & db( *dbp );

        always_show_run_output
            = attribute_value( db, "show-run-output" ) == "true";

        // if we don't find any failures
        // mark it as a pass
        pass = (db.elements.end() == std::find_if(
            db.elements.begin(),
            db.elements.end(),
            has_fail_result()
        ));

        int anything_generated = 0;
        if (!no_links){
            anything_generated = 
                generate_report(
                db, 
                lib_name, 
                test_type,
                target_dir,
                pass,
                always_show_run_output
            );
        }

        // generate the status table cell pass/warn/fail HTML
        target += "<td align=\"right\">";
        if ( anything_generated != 0 )
        {
            target += "<a href=\"";
            target += links_name;
            target += "#";
            const string target_dir_string = target_dir.string();
            std::copy(
                html_from_path(target_dir_string.begin()), 
                html_from_path(target_dir_string.end()),
                std::back_inserter(target)
                );
            target += "\">";
            target += pass
                ? (anything_generated < 2 ? pass_msg : warn_msg)
                : fail_msg;
            target += "</a>";
        }
        else  target += pass ? pass_msg : fail_msg;

        // if profiling
        if(profile && pass){
            // add link to profile
            target += " <a href=\"";
            target += (target_dir / "profile.txt").string();
            target += "\"><i>Profile</i></a>";
        }
        target += "</td>";
        return (anything_generated != 0) || !pass;
    }

    bool visit_node_tree(
        const col_node & node,
        fs::path dir_root,
        const string & lib_name,
        const string & test_name,
        string & target,
        bool profile
    ){
        bool retval = false;
        if(node.is_leaf){
            return do_cell(
                dir_root,
                lib_name,
                test_name,
                target,
                profile
            );
        }
        BOOST_FOREACH(
            const col_node::subcolumn & s,
            node.m_subcolumns
        ){
            fs::path subdir = dir_root / s.first;
            retval |= visit_node_tree(
                s.second, 
                subdir,
                lib_name,
                test_name,
                target,
                s.first == "profile"
            );
        }
        return retval;
    }

    // emit results for each test
    void do_row(
        col_node test_node,
        const fs::path & test_dir,
        const string & lib_name,
        const string & test_name,
        string & target 
    ){
        string::size_type row_start_pos = target.size();

        target += "<tr>";

        target += "<td>";
        //target += "<a href=\"" + url_prefix_dir_view + "/libs/" + lib_name + "\">";
        target += test_name;
        //target += "</a>";
        target += "</td>";

        bool no_warn_save = no_warn;

        // emit cells on this row
        bool anything_to_report = visit_node_tree(
            test_node, 
            test_dir,
            lib_name,
            test_name,
            target,
            false
        );

        target += "</tr>";

        if ( ignore_pass 
        && ! anything_to_report ) 
            target.erase( row_start_pos );

        no_warn = no_warn_save;
    }

    //  do_table_body  -----------------------------------------------------------//

    void do_table_body(
        col_node root_node, 
        const string & lib_name,
        const fs::path & test_lib_dir 
    ){
        // rows are held in a vector so they can be sorted, if desired.
        std::vector<string> results;

        BOOST_FOREACH(
            fs::directory_entry & d,
            std::make_pair(
                fs::directory_iterator(test_lib_dir), 
                fs::directory_iterator()
            )
         ){
            if(! fs::is_directory(d))
                continue;

            // if the file name contains ".test"
            if(d.path().extension() != ".test")
                continue;

            string test_name = d.path().stem().string();

            results.push_back( std::string() ); 
            do_row(
                root_node, //*test_node_itr++,
                d, // test dir
                lib_name,
                test_name,
                results[results.size()-1] 
            );
        }

        std::sort( results.begin(), results.end() );

        BOOST_FOREACH(string &s, results)
            report << s << "\n";
    }

    //  column header-----------------------------------------------------------//
    int header_depth(const col_node & root){
        int max_depth = 1;
        BOOST_FOREACH(
            const col_node::subcolumn &s,
            root.m_subcolumns
        ){
            max_depth = (std::max)(max_depth, s.second.rows);
        }
        return max_depth;
    }

    void header_cell(int rows, int cols, const std::string & name){
        // add row cells
        report << "<td align=\"center\" " ;
        if(1 < cols)
            report << "colspan=\"" << cols << "\" " ;
        if(1 < rows)
            // span rows to the end the header
            report << "rowspan=\"" << rows << "\" " ;
        report << ">" ;
        report << name;
        report << "</td>\n";
    }

    void emit_column_headers(
        const col_node & node, 
        int display_row, 
        int current_row,
        int row_count
    ){
        if(current_row < display_row){
            if(! node.m_subcolumns.empty()){
                BOOST_FOREACH(
                    const col_node::subcolumn &s,
                    node.m_subcolumns
                ){
                    emit_column_headers(
                        s.second, 
                        display_row, 
                        current_row + 1, 
                        row_count
                    );
                }
            }
            return;
        }
        /*
        if(node.is_leaf && ! node.m_subcolumns.empty()){
            header_cell(row_count - current_row, 1, std::string(""));
        }
        */
        BOOST_FOREACH(col_node::subcolumn s, node.m_subcolumns){
            if(1 == s.second.rows)
                header_cell(row_count - current_row, s.second.cols, s.first);
            else
                header_cell(1, s.second.cols, s.first);
        }
    }

    fs::path find_lib_test_dir(fs::path const& initial_path){
        // walk up from the path were we started until we find
        // bin or bin.v2

        fs::path test_lib_dir = initial_path;
        do{
            if(fs::is_directory( test_lib_dir / "bin.v2")){
                test_lib_dir /= "bin.v2";
                break;
            }
            if(fs::is_directory( test_lib_dir / "bin")){
                // v1 includes the word boost
                test_lib_dir /= "bin";
                if(fs::is_directory( test_lib_dir / "boost")){
                    test_lib_dir /= "boost";
                }
                break;
            }
        }while(! test_lib_dir.empty());

        if(test_lib_dir.empty())
            throw std::string("binary path not found");

        return test_lib_dir;
    }

    string find_lib_name(fs::path lib_test_dir){
        // search the path backwards for the magic name "libs"
        fs::path::iterator e_itr = lib_test_dir.end();
        while(lib_test_dir.begin() != e_itr){
            if(*--e_itr == "libs")
                break;
        }

        // if its found
        if(lib_test_dir.begin() != e_itr){
            // use the whole path since the "libs"
            ++e_itr;
        }
        // otherwise, just use the last two components
        else{
            e_itr = lib_test_dir.end();
            if(e_itr != lib_test_dir.begin()){
                if(--e_itr != lib_test_dir.begin()){
                    --e_itr;
                }
            }
        }

        fs::path library_name;
        while(lib_test_dir.end() != e_itr){
            library_name /= *e_itr++;
        }
        return library_name.string();
    }

    fs::path find_boost_root(fs::path initial_path){
        fs::path boost_root = initial_path;
        for(;;){
            if(fs::is_directory( boost_root / "boost")){
                break;
            }
            if(boost_root.empty())
                throw std::string("boost root not found");
            boost_root.remove_filename();
        }

        return boost_root;
    }

    //  do_table  ----------------------------------------------------------------//
    void do_table(const fs::path & lib_test_dir, const string & lib_name)
    {
        col_node root_node;

        BOOST_FOREACH(
            fs::directory_entry & d,
            std::make_pair(
                fs::directory_iterator(lib_test_dir), 
                fs::directory_iterator()
            )
         ){
            if(! fs::is_directory(d))
                continue;
            fs::path p = d.path();
            if(p.extension() != ".test")
                continue;
            build_node_tree(d, root_node);
        }
        
        // visit directory nodes and record nodetree
        report << "<table border=\"1\" cellspacing=\"0\" cellpadding=\"5\">\n";

        // emit
        root_node.get_spans();
        int row_count = header_depth(root_node);
        report << "<tr>\n";
        report << "<td rowspan=\"" << row_count << "\">Test Name</td>\n";

        // emit column headers
        int row_index = 0;
        for(;;){
            emit_column_headers(root_node, row_index, 0, row_count);
            report << "</tr>" ;
            if(++row_index == row_count)
                break;
            report << "<tr>\n";
        }

        // now the rest of the table body
        do_table_body(root_node, lib_name, lib_test_dir);

        report << "</table>\n";
   }
}// unnamed namespace

//  main  --------------------------------------------------------------------//

#define BOOST_NO_CPP_MAIN_SUCCESS_MESSAGE
#include <boost/test/included/prg_exec_monitor.hpp>

int cpp_main( int argc, char * argv[] ) // note name!
{
    fs::path initial_path = fs::initial_path();

    while ( argc > 1 && *argv[1] == '-' )
    {
        if ( argc > 2 && std::strcmp( argv[1], "--compiler" ) == 0 )
        { specific_compiler = argv[2]; --argc; ++argv; }
        else if ( argc > 2 && std::strcmp( argv[1], "--locate-root" ) == 0 )
        { locate_root = fs::path( argv[2] ); --argc; ++argv; }
        else if ( std::strcmp( argv[1], "--ignore-pass" ) == 0 ) ignore_pass = true;
        else if ( std::strcmp( argv[1], "--no-warn" ) == 0 ) no_warn = true;
        else if ( std::strcmp( argv[1], "--v2" ) == 0 )
        {--argc; ++argv ;} // skip
        else if ( argc > 2 && std::strcmp( argv[1], "--jamfile" ) == 0)
        {--argc; ++argv;} // skip
        else { std::cerr << "Unknown option: " << argv[1] << "\n"; argc = 1; }
        --argc;
        ++argv;
    }

    if ( argc != 2 && argc != 3 )
    {
        std::cerr <<
            "Usage: library_status [options...] status-file [links-file]\n"
            "  boost-root is the path to the boost tree root directory.\n"
            "  status-file and links-file are paths to the output files.\n"
            "  options: --compiler name     Run for named compiler only\n"
            "           --ignore-pass       Do not report tests which pass all compilers\n"
            "           --no-warn           Warnings not reported if test passes\n"
            "           --locate-root path  Path to ALL_LOCATE_TARGET for bjam;\n"
            "                               default boost-root.\n"
            "Example: library_status --compiler gcc /boost-root cs.html cs-links.html\n"
            "Note: Only the leaf of the links-file path is\n"
            "used in status-file HTML links. Thus for browsing, status-file,\n"
            "links-file must be in the same directory.\n"
            ;
        return 1;
    }

    if(locate_root.empty())
        if(! fs::exists("bin") && ! fs::exists("bin.v2"))
            locate_root = find_boost_root(initial_path);

    report.open( fs::path( argv[1] ) );
    if ( !report )
    {
        std::cerr << "Could not open report output file: " << argv[2] << std::endl;
        return 1;
    }

    if ( argc == 3 )
    {
        fs::path links_path( argv[2] );
        links_name = links_path.filename().string();
        links_file.open( links_path );
        if ( !links_file )
        {
            std::cerr << "Could not open links output file: " << argv[3] << std::endl;
            return 1;
        }
    }
    else no_links = true;

    const string library_name = find_lib_name(initial_path);

    char run_date[128];
    std::time_t tod;
    std::time( &tod );
    std::strftime( run_date, sizeof(run_date),
        "%X UTC, %A %d %B %Y", std::gmtime( &tod ) );

    report 
        << "<html>\n"
        << "<head>\n"
        << "<title>Boost Library Status Automatic Test</title>\n"
        << "</head>\n"
        << "<body bgcolor=\"#ffffff\" text=\"#000000\">\n"
        << "<table border=\"0\">\n"
        << "<h1>Library Status: " + library_name + "</h1>\n"
        << "<b>Run Date:</b> "
        << run_date
        << "\n<br>"
        ;

    report << "</td>\n</table>\n<br>\n";

    if ( !no_links )
    {
        links_file
            << "<html>\n"
            << "<head>\n"
            << "<title>Boost Library Status Error Log</title>\n"
            << "</head>\n"
            << "<body bgcolor=\"#ffffff\" text=\"#000000\">\n"
            << "<table border=\"0\">\n"
            << "<h1>Library Status: " + library_name + "</h1>\n"
            << "<b>Run Date:</b> "
            << run_date
            << "\n<br></table>\n<br>\n"
            ;
    }

    // detect whether in a a directory which looks like
    // bin/<library name>/test
    // or just
    // bin
    fs::path library_test_directory = find_lib_test_dir(locate_root);
    // if libs exists, drop down a couple of levels
    if(fs::is_directory( library_test_directory / "libs")){
        library_test_directory /= "libs";
        library_test_directory /= library_name;
    }

    do_table(library_test_directory, library_name);

    report << "</body>\n"
        "</html>\n"
        ;

    if ( !no_links )
    {
        links_file
            << "</body>\n"
            "</html>\n"
            ;
    }
    return 0;
}

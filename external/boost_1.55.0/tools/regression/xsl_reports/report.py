
# Copyright (c) MetaCommunications, Inc. 2003-2004
#
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

import shutil
import os.path
import os
import string
import time
import sys

import utils
import runner


report_types = [ 'us', 'ds', 'ud', 'dd', 'l', 'p', 'x', 'i', 'n', 'ddr', 'dsr' ]

if __name__ == '__main__':
    run_dir = os.path.abspath( os.path.dirname( sys.argv[ 0 ] ) )
else:
    run_dir = os.path.abspath( os.path.dirname( sys.modules[ __name__ ].__file__ ) )


def map_path( path ):
    return os.path.join( run_dir, path ) 


def xsl_path( xsl_file_name, v2 = 0 ):
    if v2:
        return map_path( os.path.join( 'xsl/v2', xsl_file_name ) )
    else:
        return map_path( os.path.join( 'xsl', xsl_file_name ) )


def make_result_pages( 
          test_results_file
        , expected_results_file
        , failures_markup_file
        , tag
        , run_date
        , comment_file
        , results_dir
        , result_prefix
        , reports
        , v2
        ):

    utils.log( 'Producing the reports...' )
    __log__ = 1
    
    output_dir = os.path.join( results_dir, result_prefix )
    utils.makedirs( output_dir )
    
    if comment_file != '':
        comment_file = os.path.abspath( comment_file )
        
    if expected_results_file != '':
        expected_results_file = os.path.abspath( expected_results_file )
    else:
        expected_results_file = os.path.abspath( map_path( 'empty_expected_results.xml' ) )
        

    extended_test_results = os.path.join( output_dir, 'extended_test_results.xml' )
    if 'x' in reports:    
        utils.log( '    Merging with expected results...' )
        utils.libxslt( 
              utils.log
            , test_results_file
            , xsl_path( 'add_expected_results.xsl', v2 )
            , extended_test_results
            , { 'expected_results_file': expected_results_file
              , 'failures_markup_file' : failures_markup_file
              , 'source' : tag }
            )

    links = os.path.join( output_dir, 'links.html' )
    
    utils.makedirs( os.path.join( output_dir, 'output' ) )
    for mode in ( 'developer', 'user' ):
        utils.makedirs( os.path.join( output_dir, mode , 'output' ) )
        
    if 'l' in reports:        
        utils.log( '    Making test output files...' )
        utils.libxslt( 
              utils.log
            , extended_test_results
            , xsl_path( 'links_page.xsl', v2 )
            , links
            , {
                  'source':                 tag
                , 'run_date':               run_date 
                , 'comment_file':           comment_file
                , 'explicit_markup_file':   failures_markup_file
                }
            )


    issues = os.path.join( output_dir, 'developer', 'issues.html'  )
    if 'i' in reports:
        utils.log( '    Making issues list...' )
        utils.libxslt( 
              utils.log
            , extended_test_results
            , xsl_path( 'issues_page.xsl', v2 )
            , issues
            , {
                  'source':                 tag
                , 'run_date':               run_date
                , 'comment_file':           comment_file
                , 'explicit_markup_file':   failures_markup_file
                }
            )

    for mode in ( 'developer', 'user' ):
        if mode[0] + 'd' in reports:
            utils.log( '    Making detailed %s  report...' % mode )
            utils.libxslt( 
                  utils.log
                , extended_test_results
                , xsl_path( 'result_page.xsl', v2 )
                , os.path.join( output_dir, mode, 'index.html' )
                , { 
                      'links_file':             'links.html'
                    , 'mode':                   mode
                    , 'source':                 tag
                    , 'run_date':               run_date 
                    , 'comment_file':           comment_file
                    , 'expected_results_file':  expected_results_file
                    , 'explicit_markup_file' :  failures_markup_file
                    }
                )
    
    for mode in ( 'developer', 'user' ):
        if mode[0] + 's' in reports:
            utils.log( '    Making summary %s  report...' % mode )
            utils.libxslt(
                  utils.log
                , extended_test_results
                , xsl_path( 'summary_page.xsl', v2 )
                , os.path.join( output_dir, mode, 'summary.html' )
                , { 
                      'mode' :                  mode 
                    , 'source':                 tag
                    , 'run_date':               run_date 
                    , 'comment_file':           comment_file
                    , 'explicit_markup_file' :  failures_markup_file
                    }
                )

    if v2 and "ddr" in reports:
        utils.log( '    Making detailed %s release report...' % mode )
        utils.libxslt( 
                  utils.log
                , extended_test_results
                , xsl_path( 'result_page.xsl', v2 )
                , os.path.join( output_dir, "developer", 'index_release.html' )
                , { 
                      'links_file':             'links.html'
                    , 'mode':                   "developer"
                    , 'source':                 tag
                    , 'run_date':               run_date 
                    , 'comment_file':           comment_file
                    , 'expected_results_file':  expected_results_file
                    , 'explicit_markup_file' :  failures_markup_file
                    , 'release':                "yes"
                    }
                )

    if v2 and "dsr" in reports:
        utils.log( '    Making summary %s release report...' % mode )
        utils.libxslt(
                  utils.log
                , extended_test_results
                , xsl_path( 'summary_page.xsl', v2 )
                , os.path.join( output_dir, "developer", 'summary_release.html' )
                , { 
                      'mode' :                  "developer"
                    , 'source':                 tag
                    , 'run_date':               run_date 
                    , 'comment_file':           comment_file
                    , 'explicit_markup_file' :  failures_markup_file
                    , 'release':                'yes'
                    }
                )
        
    if 'e' in reports:
        utils.log( '    Generating expected_results ...' )
        utils.libxslt(
              utils.log
            , extended_test_results
            , xsl_path( 'produce_expected_results.xsl', v2 )
            , os.path.join( output_dir, 'expected_results.xml' )
            )

    if v2 and 'n' in reports:
        utils.log( '    Making runner comment files...' )
        utils.libxslt(
              utils.log
            , extended_test_results
            , xsl_path( 'runners.xsl', v2 )
            , os.path.join( output_dir, 'runners.html' )
            )

    shutil.copyfile(
          xsl_path( 'html/master.css', v2 )
        , os.path.join( output_dir, 'master.css' )
        )


def build_xsl_reports( 
          locate_root_dir
        , tag
        , expected_results_file
        , failures_markup_file
        , comment_file
        , results_dir
        , result_file_prefix
        , dont_collect_logs = 0
        , reports = report_types
        , v2 = 0
        , user = None
        , upload = False
        ):

    ( run_date ) = time.strftime( '%Y-%m-%dT%H:%M:%SZ', time.gmtime() )
    
    test_results_file = os.path.join( results_dir, 'test_results.xml' )
    bin_boost_dir = os.path.join( locate_root_dir, 'bin', 'boost' )

    if v2:
        import merger
        merger.merge_logs(
              tag
            , user
            , results_dir
            , test_results_file
            , dont_collect_logs
            )
    else:
        utils.log( '  dont_collect_logs: %s' % dont_collect_logs )
        if not dont_collect_logs:
            f = open( test_results_file, 'w+' )
            f.write( '<tests>\n' )
            runner.collect_test_logs( [ bin_boost_dir ], f )
            f.write( '</tests>\n' )
            f.close()

    make_result_pages( 
          test_results_file
        , expected_results_file
        , failures_markup_file
        , tag
        , run_date
        , comment_file
        , results_dir
        , result_file_prefix
        , reports
        , v2
        )

    if v2 and upload:
        upload_dir = 'regression-logs/'
        utils.log( 'Uploading v2 results into "%s" [connecting as %s]...' % ( upload_dir, user ) )
        
        archive_name = '%s.tar.gz' % result_file_prefix
        utils.tar( 
              os.path.join( results_dir, result_file_prefix )
            , archive_name
            )
        
        utils.sourceforge.upload( os.path.join( results_dir, archive_name ), upload_dir, user )
        utils.sourceforge.untar( os.path.join( upload_dir, archive_name ), user, background = True )


def accept_args( args ):
    args_spec = [ 
          'locate-root='
        , 'tag='
        , 'expected-results='
        , 'failures-markup='
        , 'comment='
        , 'results-dir='
        , 'results-prefix='
        , 'dont-collect-logs'
        , 'reports='
        , 'v2'
        , 'user='
        , 'upload'
        , 'help'
        ]
        
    options = { 
          '--comment': ''
        , '--expected-results': ''
        , '--failures-markup': ''
        , '--reports': string.join( report_types, ',' )
        , '--tag': None
        , '--user': None
        , 'upload': False
        }
    
    utils.accept_args( args_spec, args, options, usage )
    if not options.has_key( '--results-dir' ):
         options[ '--results-dir' ] = options[ '--locate-root' ]

    if not options.has_key( '--results-prefix' ):
        if options.has_key( '--v2' ):
            options[ '--results-prefix' ] = 'all'
        else:
            options[ '--results-prefix' ] = ''
    
    return ( 
          options[ '--locate-root' ]
        , options[ '--tag' ]
        , options[ '--expected-results' ]
        , options[ '--failures-markup' ]
        , options[ '--comment' ]
        , options[ '--results-dir' ]
        , options[ '--results-prefix' ]
        , options.has_key( '--dont-collect-logs' )
        , options[ '--reports' ].split( ',' )
        , options.has_key( '--v2' )
        , options[ '--user' ]
        , options.has_key( '--upload' )
        )


def usage():
    print 'Usage: %s [options]' % os.path.basename( sys.argv[0] )
    print    '''
\t--locate-root         the same as --locate-root in compiler_status
\t--tag                 the tag for the results (i.e. 'CVS-HEAD')
\t--expected-results    the file with the results to be compared with
\t                      the current run
\t--failures-markup     the file with the failures markup
\t--comment             an html comment file (will be inserted in the reports)
\t--results-dir         the directory containing -links.html, -fail.html
\t                      files produced by compiler_status (by default the
\t                      same as specified in --locate-root)
\t--results-prefix      the prefix of -links.html, -fail.html
\t                      files produced by compiler_status
\t--v2                  v2 reports (combine multiple runners results into a 
\t                      single set of reports)

The following options are valid only for v2 reports:

\t--user                SourceForge user name for a shell account
\t--upload              upload v2 reports to SourceForge 

The following options are useful in debugging:

\t--dont-collect-logs dont collect the test logs
\t--reports           produce only the specified reports
\t                        us - user summary
\t                        ds - developer summary
\t                        ud - user detailed
\t                        dd - developer detailed
\t                        l  - links
\t                        p  - patches
\t                        x  - extended results file
\t                        i  - issues
'''

def main():
    build_xsl_reports( *accept_args( sys.argv[ 1 : ] ) )

if __name__ == '__main__':
    main()

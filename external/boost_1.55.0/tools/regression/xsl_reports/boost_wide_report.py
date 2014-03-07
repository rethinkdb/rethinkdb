
# Copyright (c) MetaCommunications, Inc. 2003-2007
#
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

import shutil
import codecs
import xml.sax.handler
import xml.sax.saxutils
import glob
import re
import os.path
import os
import string
import time
import sys
import ftplib

import utils

report_types = [ 'us', 'ds', 'ud', 'dd', 'l', 'p', 'i', 'n', 'ddr', 'dsr', 'udr', 'usr' ]

if __name__ == '__main__':
    run_dir = os.path.abspath( os.path.dirname( sys.argv[ 0 ] ) )
else:
    run_dir = os.path.abspath( os.path.dirname( sys.modules[ __name__ ].__file__ ) )


def map_path( path ):
    return os.path.join( run_dir, path ) 


def xsl_path( xsl_file_name ):
    return map_path( os.path.join( 'xsl/v2', xsl_file_name ) )

class file_info:
    def __init__( self, file_name, file_size, file_date ):
        self.name = file_name
        self.size = file_size
        self.date = file_date

    def __repr__( self ):
        return "name: %s, size: %s, date %s" % ( self.name, self.size, self.date )

#
# Find the mod time from unix format directory listing line
#

def get_date( words ):
    date = words[ 5: -1 ]
    t = time.localtime()

    month_names = [ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" ]

    year = time.localtime()[0] # If year is not secified is it the current year
    month = month_names.index( date[0] ) + 1
    day = int( date[1] )
    hours = 0 
    minutes = 0

    if  date[2].find( ":" ) != -1:
        ( hours, minutes ) = [ int(x) for x in date[2].split( ":" ) ]
    else:
        # there is no way to get seconds for not current year dates
        year = int( date[2] )

    return ( year, month, day, hours, minutes, 0, 0, 0, 0 )

def list_ftp( f ):
    # f is an ftp object
    utils.log( "listing source content" )
    lines = []

    # 1. get all lines
    f.dir( lambda x: lines.append( x ) )

    # 2. split lines into words
    word_lines = [ x.split( None, 8 ) for x in lines ]

    # we don't need directories
    result = [ file_info( l[-1], None, get_date( l ) ) for l in word_lines if l[0][0] != "d" ]
    for f in result:
        utils.log( "    %s" % f )
    return result

def list_dir( dir ):
    utils.log( "listing destination content %s" % dir )
    result = []
    for file_path in glob.glob( os.path.join( dir, "*.zip" ) ):
        if os.path.isfile( file_path ):
            mod_time = time.localtime( os.path.getmtime( file_path ) )
            mod_time = ( mod_time[0], mod_time[1], mod_time[2], mod_time[3], mod_time[4], mod_time[5], 0, 0, mod_time[8] )
            # no size (for now)
            result.append( file_info( os.path.basename( file_path ), None, mod_time ) )
    for fi in result:
        utils.log( "    %s" % fi )
    return result

def find_by_name( d, name ):
    for dd in d:
        if dd.name == name:
            return dd
    return None

def diff( source_dir_content, destination_dir_content ):
    utils.log( "Finding updated files" )
    result = ( [], [] ) # ( changed_files, obsolete_files )
    for source_file in source_dir_content:
        found = find_by_name( destination_dir_content, source_file.name )
        if found is None: result[0].append( source_file.name )
        elif time.mktime( found.date ) != time.mktime( source_file.date ): result[0].append( source_file.name )
        else:
            pass
    for destination_file in destination_dir_content:
        found = find_by_name( source_dir_content, destination_file.name )
        if found is None: result[1].append( destination_file.name )
    utils.log( "   Updated files:" )
    for f in result[0]:
        utils.log( "    %s" % f )
    utils.log( "   Obsolete files:" )
    for f in result[1]:
        utils.log( "    %s" % f )
    return result
        
def _modtime_timestamp( file ):
    return os.stat( file ).st_mtime
                

root_paths = []

def shorten( file_path ):
    root_paths.sort( lambda x, y: cmp( len(y ), len( x ) ) )
    for root in root_paths:
        if file_path.lower().startswith( root.lower() ):
            return file_path[ len( root ): ].replace( "\\", "/" )
    return file_path.replace( "\\", "/" )

class action:
    def __init__( self, file_path ):
        self.file_path_ = file_path
        self.relevant_paths_ = [ self.file_path_ ]
        self.boost_paths_ = []
        self.dependencies_ = []
        self.other_results_ = []

    def run( self ):
        utils.log( "%s: run" % shorten( self.file_path_ ) )
        __log__ = 2

        for dependency in self.dependencies_:
            if not os.path.exists( dependency ):
                utils.log( "%s doesn't exists, removing target" % shorten( dependency ) )
                self.clean()
                return

        if not os.path.exists( self.file_path_ ):
            utils.log( "target doesn't exists, building" )            
            self.update()
            return

        dst_timestamp = _modtime_timestamp( self.file_path_ )
        utils.log( "    target: %s [%s]" % ( shorten( self.file_path_ ),  dst_timestamp ) )
        needs_updating = 0
        utils.log( "    dependencies:" )
        for dependency in  self.dependencies_:
            dm = _modtime_timestamp( dependency )
            update_mark = ""
            if dm > dst_timestamp:
                needs_updating = 1
            utils.log( '        %s [%s] %s' % ( shorten( dependency ), dm, update_mark ) )
            
        if  needs_updating:
            utils.log( "target needs updating, rebuilding" )            
            self.update()
            return
        else:
            utils.log( "target is up-to-date" )            


    def clean( self ):
        to_unlink = self.other_results_ + [ self.file_path_ ]
        for result in to_unlink:
            utils.log( '  Deleting obsolete "%s"' % shorten( result ) )
            if os.path.exists( result ):
                os.unlink( result )
    
class merge_xml_action( action ):
    def __init__( self, source, destination, expected_results_file, failures_markup_file, tag ):
        action.__init__( self, destination )
        self.source_ = source
        self.destination_ = destination
        self.tag_ = tag
        
        self.expected_results_file_ = expected_results_file
        self.failures_markup_file_  = failures_markup_file

        self.dependencies_.extend( [
            self.source_
            , self.expected_results_file_
            , self.failures_markup_file_
            ]
            )

        self.relevant_paths_.extend( [ self.source_ ] )
        self.boost_paths_.extend( [ self.expected_results_file_, self.failures_markup_file_ ] ) 


        
    def update( self ):
        def filter_xml( src, dest ):
            
            class xmlgen( xml.sax.saxutils.XMLGenerator ):
                def __init__( self, writer ):
                   xml.sax.saxutils.XMLGenerator.__init__( self, writer )
                  
                   self.trimmed = 0
                   self.character_content = ""

                def startElement( self, name, attrs):
                    self.flush()
                    xml.sax.saxutils.XMLGenerator.startElement( self, name, attrs )

                def endElement( self, name ):
                    self.flush()
                    xml.sax.saxutils.XMLGenerator.endElement( self, name )
                    
                def flush( self ):
                    content = self.character_content
                    self.character_content = ""
                    self.trimmed = 0
                    xml.sax.saxutils.XMLGenerator.characters( self, content )

                def characters( self, content ):
                    if not self.trimmed:
                        max_size = pow( 2, 16 )
                        self.character_content += content
                        if len( self.character_content ) > max_size:
                            self.character_content = self.character_content[ : max_size ] + "...\n\n[The content has been trimmed by the report system because it exceeds %d bytes]" % max_size
                            self.trimmed = 1

            o = open( dest, "w" )
            try: 
                gen = xmlgen( o )
                xml.sax.parse( src, gen )
            finally:
                o.close()

            return dest

            
        utils.log( 'Merging "%s" with expected results...' % shorten( self.source_ ) )
        try:
            trimmed_source = filter_xml( self.source_, '%s-trimmed.xml' % os.path.splitext( self.source_ )[0] )
            utils.libxslt(
                  utils.log
                , trimmed_source
                , xsl_path( 'add_expected_results.xsl' )
                , self.file_path_
                , {
                    "expected_results_file" : self.expected_results_file_
                  , "failures_markup_file": self.failures_markup_file_
                  , "source" : self.tag_ 
                  }
                )

            os.unlink( trimmed_source )

        except Exception, msg:
            utils.log( '  Skipping "%s" due to errors (%s)' % ( self.source_, msg ) )
            if os.path.exists( self.file_path_ ):
                os.unlink( self.file_path_ )

        
    def _xml_timestamp( xml_path ):

        class timestamp_reader( xml.sax.handler.ContentHandler ):
            def startElement( self, name, attrs ):
                if name == 'test-run':
                    self.timestamp = attrs.getValue( 'timestamp' )
                    raise self

        try:
            xml.sax.parse( xml_path, timestamp_reader() )
            raise 'Cannot extract timestamp from "%s". Invalid XML file format?' % xml_path
        except timestamp_reader, x:
            return x.timestamp


class make_links_action( action ):
    def __init__( self, source, destination, output_dir, tag, run_date, comment_file, failures_markup_file ):
        action.__init__( self, destination )
        self.dependencies_.append( source )
        self.source_ = source
        self.output_dir_ = output_dir
        self.tag_        = tag
        self.run_date_   = run_date 
        self.comment_file_ = comment_file
        self.failures_markup_file_ = failures_markup_file
        self.links_file_path_ = os.path.join( output_dir, 'links.html' )
        
    def update( self ):
        utils.makedirs( os.path.join( os.path.dirname( self.links_file_path_ ), "output" ) )
        utils.makedirs( os.path.join( os.path.dirname( self.links_file_path_ ), "developer", "output" ) )
        utils.makedirs( os.path.join( os.path.dirname( self.links_file_path_ ), "user", "output" ) )
        utils.log( '    Making test output files...' )
        try:
            utils.libxslt( 
                  utils.log
                , self.source_
                , xsl_path( 'links_page.xsl' )
                , self.links_file_path_
                , {
                    'source':                 self.tag_
                  , 'run_date':               self.run_date_
                  , 'comment_file':           self.comment_file_
                  , 'explicit_markup_file':   self.failures_markup_file_
                  }
                )
        except Exception, msg:
            utils.log( '  Skipping "%s" due to errors (%s)' % ( self.source_, msg ) )

        open( self.file_path_, "w" ).close()


class unzip_action( action ):
    def __init__( self, source, destination, unzip_func ):
        action.__init__( self, destination )
        self.dependencies_.append( source )
        self.source_     = source
        self.unzip_func_ = unzip_func

    def update( self ):
        try:
            utils.log( '  Unzipping "%s" ... into "%s"' % ( shorten( self.source_ ), os.path.dirname( self.file_path_ ) ) )
            self.unzip_func_( self.source_, os.path.dirname( self.file_path_ ) )
        except Exception, msg:
            utils.log( '  Skipping "%s" due to errors (%s)' % ( self.source_, msg ) )


def ftp_task( site, site_path , destination ):
    __log__ = 1
    utils.log( '' )
    utils.log( 'ftp_task: "ftp://%s/%s" -> %s' % ( site, site_path, destination ) )

    utils.log( '    logging on ftp site %s' % site )
    f = ftplib.FTP( site )
    f.login()
    utils.log( '    cwd to "%s"' % site_path )
    f.cwd( site_path )

    source_content = list_ftp( f )
    source_content = [ x for x in source_content if re.match( r'.+[.](?<!log[.])zip', x.name ) and x.name.lower() != 'boostbook.zip' ]
    destination_content = list_dir( destination )
    d = diff( source_content, destination_content )

    def synchronize():
        for source in d[0]:
            utils.log( 'Copying "%s"' % source )
            result = open( os.path.join( destination, source ), 'wb' )
            f.retrbinary( 'RETR %s' % source, result.write )
            result.close()
            mod_date = find_by_name( source_content, source ).date
            m = time.mktime( mod_date )
            os.utime( os.path.join( destination, source ), ( m, m ) )

        for obsolete in d[1]:
            utils.log( 'Deleting "%s"' % obsolete )
            os.unlink( os.path.join( destination, obsolete ) )

    utils.log( "    Synchronizing..." )
    __log__ = 2
    synchronize()
    
    f.quit()        

def unzip_archives_task( source_dir, processed_dir, unzip_func ):
    utils.log( '' )
    utils.log( 'unzip_archives_task: unpacking updated archives in "%s" into "%s"...' % ( source_dir, processed_dir ) )
    __log__ = 1

    target_files = [ os.path.join( processed_dir, os.path.basename( x.replace( ".zip", ".xml" ) )  ) for x in glob.glob( os.path.join( source_dir, "*.zip" ) ) ] + glob.glob( os.path.join( processed_dir, "*.xml" ) )
    actions = [ unzip_action( os.path.join( source_dir, os.path.basename( x.replace( ".xml", ".zip" ) ) ), x, unzip_func ) for x in target_files ]
    for a in actions:
        a.run()
   
def merge_xmls_task( source_dir, processed_dir, merged_dir, expected_results_file, failures_markup_file, tag ):    
    utils.log( '' )
    utils.log( 'merge_xmls_task: merging updated XMLs in "%s"...' % source_dir )
    __log__ = 1
        
    utils.makedirs( merged_dir )
    target_files = [ os.path.join( merged_dir, os.path.basename( x ) ) for x in glob.glob( os.path.join( processed_dir, "*.xml" ) ) ] + glob.glob( os.path.join( merged_dir, "*.xml" ) )
    actions = [ merge_xml_action( os.path.join( processed_dir, os.path.basename( x ) )
                                  , x
                                  , expected_results_file
                                  , failures_markup_file 
                                  , tag ) for x in target_files ]

    for a in actions:
        a.run()


def make_links_task( input_dir, output_dir, tag, run_date, comment_file, extended_test_results, failures_markup_file ):
    utils.log( '' )
    utils.log( 'make_links_task: make output files for test results in "%s"...' % input_dir )
    __log__ = 1

    target_files = [ x + ".links"  for x in glob.glob( os.path.join( input_dir, "*.xml" ) ) ] + glob.glob( os.path.join( input_dir, "*.links" ) )
    actions = [ make_links_action( x.replace( ".links", "" )
                                   , x
                                   , output_dir
                                   , tag
                                   , run_date
                                   , comment_file
                                   , failures_markup_file 
                                   ) for x in target_files ]

    for a in actions:
        a.run()


class xmlgen( xml.sax.saxutils.XMLGenerator ):
    document_started = 0
    
    def startDocument( self ):
        if not self.document_started:
            xml.sax.saxutils.XMLGenerator.startDocument( self )
            self.document_started = 1


def merge_processed_test_runs( test_runs_dir, tag, writer ):
    utils.log( '' )
    utils.log( 'merge_processed_test_runs: merging processed test runs from %s into a single XML...' % test_runs_dir )
    __log__ = 1
    
    all_runs_xml = xmlgen( writer, encoding='utf-8' )
    all_runs_xml.startDocument()
    all_runs_xml.startElement( 'all-test-runs', {} )
    
    files = glob.glob( os.path.join( test_runs_dir, '*.xml' ) )
    for test_run in files:
        #file_pos = writer.stream.tell()
        file_pos = writer.tell()
        try:
            utils.log( '    Writing "%s" into the resulting XML...' % test_run )
            xml.sax.parse( test_run, all_runs_xml )
        except Exception, msg:
            utils.log( '    Skipping "%s" due to errors (%s)' % ( test_run, msg ) )
            #writer.stream.seek( file_pos )
            #writer.stream.truncate()
            writer.seek( file_pos )
            writer.truncate()

    all_runs_xml.endElement( 'all-test-runs' )
    all_runs_xml.endDocument()


def execute_tasks(
          tag
        , user
        , run_date
        , comment_file
        , results_dir
        , output_dir
        , reports
        , warnings
        , extended_test_results
        , dont_collect_logs
        , expected_results_file
        , failures_markup_file
        ):

    incoming_dir = os.path.join( results_dir, 'incoming', tag )
    processed_dir = os.path.join( incoming_dir, 'processed' )
    merged_dir = os.path.join( processed_dir, 'merged' )
    if not os.path.exists( incoming_dir ):
        os.makedirs( incoming_dir )
    if not os.path.exists( processed_dir ):
        os.makedirs( processed_dir )
    if not os.path.exists( merged_dir ):
        os.makedirs( merged_dir )
    
    if not dont_collect_logs:
        ftp_site = 'boost.cowic.de'
        site_path = '/boost/do-not-publish-this-url/results/%s' % tag

        ftp_task( ftp_site, site_path, incoming_dir )

    unzip_archives_task( incoming_dir, processed_dir, utils.unzip )
    merge_xmls_task( incoming_dir, processed_dir, merged_dir, expected_results_file, failures_markup_file, tag )
    make_links_task( merged_dir
                     , output_dir
                     , tag
                     , run_date
                     , comment_file
                     , extended_test_results
                     , failures_markup_file )


    results_xml_path = os.path.join( output_dir, 'extended_test_results.xml' )
    #writer = codecs.open( results_xml_path, 'w', 'utf-8' )
    writer = open( results_xml_path, 'w' )
    merge_processed_test_runs( merged_dir, tag, writer )
    writer.close()

    
    make_result_pages(
          extended_test_results
        , expected_results_file
        , failures_markup_file
        , tag
        , run_date
        , comment_file
        , output_dir
        , reports
        , warnings
        )

        
def make_result_pages(
          extended_test_results
        , expected_results_file
        , failures_markup_file
        , tag
        , run_date
        , comment_file
        , output_dir
        , reports
        , warnings
        ):

    utils.log( 'Producing the reports...' )
    __log__ = 1

    warnings_text = '+'.join( warnings )
    
    if comment_file != '':
        comment_file = os.path.abspath( comment_file )
        
    links = os.path.join( output_dir, 'links.html' )
    
    utils.makedirs( os.path.join( output_dir, 'output' ) )
    for mode in ( 'developer', 'user' ):
        utils.makedirs( os.path.join( output_dir, mode , 'output' ) )
        
    issues = os.path.join( output_dir, 'developer', 'issues.html'  )
    if 'i' in reports:
        utils.log( '    Making issues list...' )
        utils.libxslt( 
              utils.log
            , extended_test_results
            , xsl_path( 'issues_page.xsl' )
            , issues
            , {
                  'source':                 tag
                , 'run_date':               run_date
                , 'warnings':               warnings_text
                , 'comment_file':           comment_file
                , 'expected_results_file':  expected_results_file
                , 'explicit_markup_file':   failures_markup_file
                , 'release':                "yes"
                }
            )

    for mode in ( 'developer', 'user' ):
        if mode[0] + 'd' in reports:
            utils.log( '    Making detailed %s  report...' % mode )
            utils.libxslt( 
                  utils.log
                , extended_test_results
                , xsl_path( 'result_page.xsl' )
                , os.path.join( output_dir, mode, 'index.html' )
                , { 
                      'links_file':             'links.html'
                    , 'mode':                   mode
                    , 'source':                 tag
                    , 'run_date':               run_date
                    , 'warnings':               warnings_text
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
                , xsl_path( 'summary_page.xsl' )
                , os.path.join( output_dir, mode, 'summary.html' )
                , { 
                      'mode' :                  mode 
                    , 'source':                 tag
                    , 'run_date':               run_date 
                    , 'warnings':               warnings_text
                    , 'comment_file':           comment_file
                    , 'explicit_markup_file' :  failures_markup_file
                    }
                )

    for mode in ( 'developer', 'user' ):
        if mode[0] + 'dr' in reports:
            utils.log( '    Making detailed %s release report...' % mode )
            utils.libxslt( 
                  utils.log
                , extended_test_results
                , xsl_path( 'result_page.xsl' )
                , os.path.join( output_dir, mode, 'index_release.html' )
                , { 
                      'links_file':             'links.html'
                    , 'mode':                   mode
                    , 'source':                 tag
                    , 'run_date':               run_date 
                    , 'warnings':               warnings_text
                    , 'comment_file':           comment_file
                    , 'expected_results_file':  expected_results_file
                    , 'explicit_markup_file' :  failures_markup_file
                    , 'release':                "yes"
                    }
                )

    for mode in ( 'developer', 'user' ):
        if mode[0] + 'sr' in reports:
            utils.log( '    Making summary %s release report...' % mode )
            utils.libxslt(
                  utils.log
                , extended_test_results
                , xsl_path( 'summary_page.xsl' )
                , os.path.join( output_dir, mode, 'summary_release.html' )
                , { 
                      'mode' :                  mode
                    , 'source':                 tag
                    , 'run_date':               run_date 
                    , 'warnings':               warnings_text
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
            , xsl_path( 'produce_expected_results.xsl' )
            , os.path.join( output_dir, 'expected_results.xml' )
            )

    if  'n' in reports:
        utils.log( '    Making runner comment files...' )
        utils.libxslt(
              utils.log
            , extended_test_results
            , xsl_path( 'runners.xsl' )
            , os.path.join( output_dir, 'runners.html' )
            )

    shutil.copyfile(
          xsl_path( 'html/master.css' )
        , os.path.join( output_dir, 'master.css' )
        )

    fix_file_names( output_dir )


def fix_file_names( dir ):
    """
    The current version of xslproc doesn't correctly handle
    spaces. We have to manually go through the
    result set and decode encoded spaces (%20).
    """
    utils.log( 'Fixing encoded file names...' )
    for root, dirs, files in os.walk( dir ):
        for file in files:
            if file.find( "%20" ) > -1:
                new_name = file.replace( "%20", " " )
                utils.rename(
                      utils.log
                    , os.path.join( root, file )
                    , os.path.join( root, new_name )
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
        , warnings = []
        , user = None
        , upload = False
        ):

    ( run_date ) = time.strftime( '%Y-%m-%dT%H:%M:%SZ', time.gmtime() )

    root_paths.append( locate_root_dir )
    root_paths.append( results_dir )
    
    bin_boost_dir = os.path.join( locate_root_dir, 'bin', 'boost' )
    
    output_dir = os.path.join( results_dir, result_file_prefix )
    utils.makedirs( output_dir )
    
    if expected_results_file != '':
        expected_results_file = os.path.abspath( expected_results_file )
    else:
        expected_results_file = os.path.abspath( map_path( 'empty_expected_results.xml' ) )


    extended_test_results = os.path.join( output_dir, 'extended_test_results.xml' )
        
    execute_tasks(
          tag
        , user
        , run_date
        , comment_file
        , results_dir
        , output_dir
        , reports
        , warnings
        , extended_test_results
        , dont_collect_logs
        , expected_results_file
        , failures_markup_file
        )

    if upload:
        upload_dir = 'regression-logs/'
        utils.log( 'Uploading  results into "%s" [connecting as %s]...' % ( upload_dir, user ) )
        
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
        options[ '--results-prefix' ] = 'all'
    
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
        , options[ '--user' ]
        , options.has_key( '--upload' )
        )


def usage():
    print 'Usage: %s [options]' % os.path.basename( sys.argv[0] )
    print    '''
\t--locate-root         the same as --locate-root in compiler_status
\t--tag                 the tag for the results (i.e. 'trunk')
\t--expected-results    the file with the results to be compared with
\t                      the current run
\t--failures-markup     the file with the failures markup
\t--comment             an html comment file (will be inserted in the reports)
\t--results-dir         the directory containing -links.html, -fail.html
\t                      files produced by compiler_status (by default the
\t                      same as specified in --locate-root)
\t--results-prefix      the prefix of -links.html, -fail.html
\t                      files produced by compiler_status
\t--user                SourceForge user name for a shell account
\t--upload              upload reports to SourceForge 

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
\t                        n  - runner comment files
'''

def main():
    build_xsl_reports( *accept_args( sys.argv[ 1 : ] ) )

if __name__ == '__main__':
    main()

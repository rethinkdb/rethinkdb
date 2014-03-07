
# Copyright (c) MetaCommunications, Inc. 2003-2007
#
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

import xml.sax.saxutils
import zipfile
import ftplib
import time
import stat
import xml.dom.minidom
import xmlrpclib
import httplib

import os.path
import string
import sys
import re
import urlparse


def process_xml_file( input_file, output_file ):
    utils.log( 'Processing test log "%s"' % input_file )
    
    f = open( input_file, 'r' )
    xml = f.readlines()
    f.close()
    
    for i in range( 0, len(xml)):
        xml[i] = string.translate( xml[i], utils.char_translation_table )

    output_file.writelines( xml )


def process_test_log_files( output_file, dir, names ):
    for file in names:
        if os.path.basename( file ) == 'test_log.xml':
            process_xml_file( os.path.join( dir, file ), output_file )


def collect_test_logs( input_dirs, test_results_writer ):
    __log__ = 1
    utils.log( 'Collecting test logs ...' )
    for input_dir in input_dirs:
        utils.log( 'Walking directory "%s" ...' % input_dir )
        os.path.walk( input_dir, process_test_log_files, test_results_writer )

dart_status_from_result = {
    'succeed': 'passed',
    'fail': 'failed',
    'note': 'passed',
    '': 'notrun'
    }

dart_project = {
    'trunk': 'Boost_HEAD',
    '': 'Boost_HEAD'
    }

dart_track = {
    'full': 'Nightly',
    'incremental': 'Continuous',
    '': 'Experimental'
    }

ascii_only_table = ""
for i in range(0,256):
    if chr(i) == '\n' or chr(i) == '\r':
        ascii_only_table += chr(i)
    elif i < 32 or i >= 0x80:
        ascii_only_table += '?'
    else:
        ascii_only_table += chr(i)

class xmlrpcProxyTransport(xmlrpclib.Transport):
    def __init__(self, proxy):
        self.proxy = proxy
    def make_connection(self, host):
        self.realhost = host
        return httplib.HTTP(self.proxy)
    def send_request(self, connection, handler, request_body):
        connection.putrequest('POST','http://%s%s' % (self.realhost,handler))
    def send_host(self, connection, host):
        connection.putheader('Host',self.realhost)
    

def publish_test_logs(
    input_dirs,
    runner_id, tag, platform, comment_file, timestamp, user, source, run_type,
    dart_server = None,
    http_proxy = None,
    **unused
    ):
    __log__ = 1
    utils.log( 'Publishing test logs ...' )
    dart_rpc = None
    dart_dom = {}
    
    def _publish_test_log_files_ ( unused, dir, names ):
        for file in names:
            if os.path.basename( file ) == 'test_log.xml':
                utils.log( 'Publishing test log "%s"' % os.path.join(dir,file) )
                if dart_server:
                    log_xml = open(os.path.join(dir,file)).read().translate(ascii_only_table)
                    #~ utils.log( '--- XML:\n%s' % log_xml)
                    #~ It seems possible to get an empty XML result file :-(
                    if log_xml == "": continue
                    log_dom = xml.dom.minidom.parseString(log_xml)
                    test = {
                        'library': log_dom.documentElement.getAttribute('library'),
                        'test-name': log_dom.documentElement.getAttribute('test-name'),
                        'toolset': log_dom.documentElement.getAttribute('toolset')
                        }
                    if not test['test-name'] or test['test-name'] == '':
                        test['test-name'] = 'unknown'
                    if not test['toolset'] or test['toolset'] == '':
                        test['toolset'] = 'unknown'
                    if not dart_dom.has_key(test['toolset']):
                        dart_dom[test['toolset']] = xml.dom.minidom.parseString(
'''<?xml version="1.0" encoding="UTF-8"?>
<DartSubmission version="2.0" createdby="collect_and_upload_logs.py">
    <Site>%(site)s</Site>
    <BuildName>%(buildname)s</BuildName>
    <Track>%(track)s</Track>
    <DateTimeStamp>%(datetimestamp)s</DateTimeStamp>
</DartSubmission>
'''                         % {
                                'site': runner_id,
                                'buildname': "%s -- %s (%s)" % (platform,test['toolset'],run_type),
                                'track': dart_track[run_type],
                                'datetimestamp' : timestamp
                            } )
                    submission_dom = dart_dom[test['toolset']]
                    for node in log_dom.documentElement.childNodes:
                        if node.nodeType == xml.dom.Node.ELEMENT_NODE:
                            if node.firstChild:
                                log_data = xml.sax.saxutils.escape(node.firstChild.data)
                            else:
                                log_data = ''
                            test_dom = xml.dom.minidom.parseString('''<?xml version="1.0" encoding="UTF-8"?>
<Test>
    <Name>.Test.Boost.%(tag)s.%(library)s.%(test-name)s.%(type)s</Name>
    <Status>%(result)s</Status>
    <Measurement name="Toolset" type="text/string">%(toolset)s</Measurement>
    <Measurement name="Timestamp" type="text/string">%(timestamp)s</Measurement>
    <Measurement name="Log" type="text/text">%(log)s</Measurement>
</Test>
    '''                         % {
                                    'tag': tag,
                                    'library': test['library'],
                                    'test-name': test['test-name'],
                                    'toolset': test['toolset'],
                                    'type': node.nodeName,
                                    'result': dart_status_from_result[node.getAttribute('result')],
                                    'timestamp': node.getAttribute('timestamp'),
                                    'log': log_data
                                })
                            submission_dom.documentElement.appendChild(
                                test_dom.documentElement.cloneNode(1) )
    
    for input_dir in input_dirs:
        utils.log( 'Walking directory "%s" ...' % input_dir )
        os.path.walk( input_dir, _publish_test_log_files_, None )
    if dart_server:
        try:
            rpc_transport = None
            if http_proxy:
                rpc_transport = xmlrpcProxyTransport(http_proxy)
            dart_rpc = xmlrpclib.ServerProxy(
                'http://%s/%s/Command/' % (dart_server,dart_project[tag]),
                rpc_transport )
            for dom in dart_dom.values():
                #~ utils.log('Dart XML: %s' % dom.toxml('utf-8'))
                dart_rpc.Submit.put(xmlrpclib.Binary(dom.toxml('utf-8')))
        except Exception, e:
            utils.log('Dart server error: %s' % e)


def upload_to_ftp( tag, results_file, ftp_proxy, debug_level, ftp_url ):
    
    if not ftp_url:
        ftp_host = 'boost.cowic.de'
        ftp_url = ''.join(['ftp','://anonymous','@',ftp_host,'/boost/do-not-publish-this-url/results/'])
    utils.log( 'Uploading log archive "%s" to %s/%s' % ( results_file, ftp_url, tag ) )
    
    ftp_parts = urlparse.urlparse(ftp_url)
    ftp_netloc = re.split('[@]',ftp_parts[1])
    ftp_user = re.split('[:]',ftp_netloc[0])[0]
    ftp_password = re.split('[:]',ftp_netloc[0]+':anonymous')[1]
    ftp_site = re.split('[:]',ftp_netloc[1])[0]
    ftp_path = ftp_parts[2]
    
    if not ftp_proxy:
        ftp = ftplib.FTP( ftp_site )
        ftp.set_debuglevel( debug_level )
        ftp.login( ftp_user, ftp_password )
    else:
        utils.log( '    Connecting through FTP proxy server "%s"' % ftp_proxy )
        ftp = ftplib.FTP( ftp_proxy )
        ftp.set_debuglevel( debug_level )
        ftp.set_pasv (0) # turn off PASV mode
        ftp.login( '%s@%s' % (ftp_user,ftp_site), ftp_password )

    ftp.cwd( ftp_path )
    try:
        ftp.cwd( tag )
    except ftplib.error_perm:
        for dir in tag.split( '/' ):
            ftp.mkd( dir )
            ftp.cwd( dir )

    f = open( results_file, 'rb' )
    ftp.storbinary( 'STOR %s' % os.path.basename( results_file ), f )
    ftp.quit()


def copy_comments( results_xml, comment_file ):
    results_xml.startElement( 'comment', {} )

    if os.path.exists( comment_file ):
        utils.log( 'Reading comments file "%s"...' % comment_file )
        f = open( comment_file, 'r' )
        try:
            results_xml.characters( f.read() )
        finally:
            f.close()    
    else:
        utils.log( 'Warning: comment file "%s" is not found.' % comment_file )

    lines = ['']
    for arg in sys.argv:
        # Make sure that the ftp details are hidden
        arg = re.sub( 'ftp://.*$', 'ftp://XXXXX', arg )

        # Escape quotes
        arg = re.sub( r'(\\|")', r'\\\1', arg )

        # Quote arguments if needed
        if arg.find( ' ' ) != -1:
            arg = '"%s"' % arg
        if len( lines[-1] ) + len( arg ) + 2 >= 80:
            # align backslashes
            lines[-1] += ' ' * ( 79 - len( lines[-1] ) )
            # indent lines after the first
            lines.append( '  ' )
        lines[-1] += ( arg + ' ' )

    results_xml.characters( '<hr>' )
    results_xml.characters( '<dl>' )
    results_xml.characters( '<dt>Command Line</dt>' )
    results_xml.characters( '<dd>' )
    results_xml.characters( '<pre>' )
    results_xml.characters( '\\\n'.join(lines) )
    results_xml.characters( '</pre>' )
    results_xml.characters( '</dd>' )
    results_xml.characters( '</dl>\n' )
 
    results_xml.endElement( 'comment' )


def compress_file( file_path, archive_path ):
    utils.log( 'Compressing "%s"...' % file_path )

    try:
        z = zipfile.ZipFile( archive_path, 'w', zipfile.ZIP_DEFLATED )
        z.write( file_path, os.path.basename( file_path ) )
        z.close()
        utils.log( 'Done writing "%s".'% archive_path )
    except Exception, msg:
        utils.log( 'Warning: Compressing falied (%s)' % msg )
        utils.log( '         Trying to compress using a platform-specific tool...' )
        try: import zip_cmd
        except ImportError:
            script_dir = os.path.dirname( os.path.abspath( sys.argv[0] ) )
            utils.log( 'Could not find \'zip_cmd\' module in the script directory (%s).' % script_dir )
            raise Exception( 'Compressing failed!' )
        else:
            if os.path.exists( archive_path ):
                os.unlink( archive_path )
                utils.log( 'Removing stale "%s".' % archive_path )
                
            zip_cmd.main( file_path, archive_path )
            utils.log( 'Done compressing "%s".' % archive_path )


def read_timestamp( file ):
    if not os.path.exists( file ):
        result = time.gmtime()
        utils.log( 'Warning: timestamp file "%s" does not exist'% file )
        utils.log( 'Using current UTC time (%s)' % result )
        return result

    return time.gmtime( os.stat( file ).st_mtime )


def collect_logs( 
          results_dir
        , runner_id
        , tag
        , platform
        , comment_file
        , timestamp_file
        , user
        , source
        , run_type
        , dart_server = None
        , http_proxy = None
        , revision = ''
        , **unused
        ):
    
    timestamp = time.strftime( '%Y-%m-%dT%H:%M:%SZ', read_timestamp( timestamp_file ) )
    
    if dart_server:
        publish_test_logs( [ results_dir ],
            runner_id, tag, platform, comment_file, timestamp, user, source, run_type,
            dart_server = dart_server,
            http_proxy = http_proxy )
    
    results_file = os.path.join( results_dir, '%s.xml' % runner_id )
    results_writer = open( results_file, 'w' )
    utils.log( 'Collecting test logs into "%s"...' % results_file )
        
    results_xml = xml.sax.saxutils.XMLGenerator( results_writer )
    results_xml.startDocument()
    results_xml.startElement( 
          'test-run'
        , { 
              'tag':        tag
            , 'platform':   platform
            , 'runner':     runner_id
            , 'timestamp':  timestamp
            , 'source':     source
            , 'run-type':   run_type
            , 'revision':   revision
            }
        )
    
    copy_comments( results_xml, comment_file )
    collect_test_logs( [ results_dir ], results_writer )

    results_xml.endElement( "test-run" )
    results_xml.endDocument()
    results_writer.close()
    utils.log( 'Done writing "%s".' % results_file )

    compress_file(
          results_file
        , os.path.join( results_dir,'%s.zip' % runner_id )
        )


def upload_logs(
          results_dir
        , runner_id
        , tag
        , user
        , ftp_proxy
        , debug_level
        , send_bjam_log = False
        , timestamp_file = None
        , dart_server = None
        , ftp_url = None
        , **unused
        ):

    logs_archive = os.path.join( results_dir, '%s.zip' % runner_id )
    upload_to_ftp( tag, logs_archive, ftp_proxy, debug_level, ftp_url )
    if send_bjam_log:
        bjam_log_path = os.path.join( results_dir, 'bjam.log' )
        if not timestamp_file:
            timestamp_file = bjam_log_path

        timestamp = time.strftime( '%Y-%m-%d-%H-%M-%S', read_timestamp( timestamp_file ) )
        logs_archive = os.path.join( results_dir, '%s.%s.log.zip' % ( runner_id, timestamp ) )
        compress_file( bjam_log_path, logs_archive )
        upload_to_ftp( '%s/logs' % tag, logs_archive, ftp_proxy, debug_level, ftp_url )


def collect_and_upload_logs( 
          results_dir
        , runner_id
        , tag
        , platform
        , comment_file
        , timestamp_file
        , user
        , source
        , run_type
        , revision = None
        , ftp_proxy = None
        , debug_level = 0
        , send_bjam_log = False
        , dart_server = None
        , http_proxy = None
        , ftp_url = None
        , **unused
        ):
    
    collect_logs( 
          results_dir
        , runner_id
        , tag
        , platform
        , comment_file
        , timestamp_file
        , user
        , source
        , run_type
        , revision = revision
        , dart_server = dart_server
        , http_proxy = http_proxy
        )
    
    upload_logs(
          results_dir
        , runner_id
        , tag
        , user
        , ftp_proxy
        , debug_level
        , send_bjam_log
        , timestamp_file
        , dart_server = dart_server
        , ftp_url = ftp_url
        )


def accept_args( args ):
    args_spec = [ 
          'locate-root='
        , 'runner='
        , 'tag='
        , 'platform='
        , 'comment='
        , 'timestamp='
        , 'source='
        , 'run-type='
        , 'user='
        , 'ftp-proxy='
        , 'proxy='
        , 'debug-level='
        , 'send-bjam-log'
        , 'help'
        , 'dart-server='
        , 'revision='
        , 'ftp='
        ]
    
    options = {
          '--tag'           : 'trunk'
        , '--platform'      : sys.platform
        , '--comment'       : 'comment.html'
        , '--timestamp'     : 'timestamp'
        , '--user'          : None
        , '--source'        : 'SVN'
        , '--run-type'      : 'full'
        , '--ftp-proxy'     : None
        , '--proxy'         : None
        , '--debug-level'   : 0
        , '--dart-server'   : 'beta.boost.org:8081'
        , '--revision'      : None
        , '--ftp'           : None
        
        }
    
    utils.accept_args( args_spec, args, options, usage )
        
    return {
          'results_dir'     : options[ '--locate-root' ]
        , 'runner_id'       : options[ '--runner' ]
        , 'tag'             : options[ '--tag' ]
        , 'platform'        : options[ '--platform']
        , 'comment_file'    : options[ '--comment' ]
        , 'timestamp_file'  : options[ '--timestamp' ]
        , 'user'            : options[ '--user' ]
        , 'source'          : options[ '--source' ]
        , 'run_type'        : options[ '--run-type' ]
        , 'ftp_proxy'       : options[ '--ftp-proxy' ]
        , 'http_proxy'      : options[ '--proxy' ]
        , 'debug_level'     : int(options[ '--debug-level' ])
        , 'send_bjam_log'   : options.has_key( '--send-bjam-log' )
        , 'dart_server'     : options[ '--dart-server' ]
        , 'revision'        : options[ '--revision' ]
        , 'ftp'             : options[ '--ftp' ]
        }


commands = {
      'collect-and-upload'  : collect_and_upload_logs
    , 'collect-logs'        : collect_logs
    , 'upload-logs'         : upload_logs
    }

def usage():
    print 'Usage: %s [command] [options]' % os.path.basename( sys.argv[0] )
    print    '''
Commands:
\t%s

Options:
\t--locate-root   directory to to scan for "test_log.xml" files
\t--runner        runner ID (e.g. "Metacomm")
\t--timestamp     path to a file which modification time will be used 
\t                as a timestamp of the run ("timestamp" by default)
\t--comment       an HTML comment file to be inserted in the reports
\t                ("comment.html" by default)
\t--tag           the tag for the results ("trunk" by default)
\t--user          SourceForge user name for a shell account (optional)
\t--source        where Boost sources came from ("SVN" or "tarball";
\t                "SVN" by default)
\t--run-type      "incremental" or "full" ("full" by default)
\t--send-bjam-log in addition to regular XML results, send in full bjam
\t                log of the regression run
\t--proxy         HTTP proxy server address and port (e.g.
\t                'http://www.someproxy.com:3128', optional)
\t--ftp-proxy     FTP proxy server (e.g. 'ftpproxy', optional)
\t--debug-level   debugging level; controls the amount of debugging 
\t                output printed; 0 by default (no debug output)
\t--dart-server   The dart server to send results to.
\t--ftp           The ftp URL to upload results to.
''' % '\n\t'.join( commands.keys() )

    
def main():
    if len(sys.argv) > 1 and sys.argv[1] in commands:
        command = sys.argv[1]
        args = sys.argv[ 2: ]
    else:
        command = 'collect-and-upload'
        args = sys.argv[ 1: ]
    
    commands[ command ]( **accept_args( args ) )


if __name__ != '__main__':  import utils
else:
    # in absense of relative import...
    xsl_path = os.path.abspath( os.path.dirname( sys.argv[ 0 ] ) )
    while os.path.basename( xsl_path ) != 'xsl_reports': xsl_path = os.path.dirname( xsl_path )
    sys.path.append( xsl_path )

    import utils
    main()

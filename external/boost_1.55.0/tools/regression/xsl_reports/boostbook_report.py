import ftplib
import optparse
import os
import time
import urlparse
import utils
import shutil
import sys
import zipfile
import xml.sax.saxutils


import utils.libxslt

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

#def check_for_new_upload( target_dir, boostbook_info ):

def accept_args( args ):        
    parser = optparse.OptionParser()
    parser.add_option( '-t', '--tag', dest='tag', help="the tag for the results (i.e. 'RC_1_34_0')" )
    parser.add_option( '-d', '--destination', dest='destination', help='destination directory' )

    if len(args) == 0:
        parser.print_help()
        sys.exit( 1 )

    (options, args) = parser.parse_args()
    if not options.destination:
        print '-d is required'
        parser.print_help()
        sys.exit( 1 )
    return options

def unzip( archive_path, result_dir ):
    utils.log( 'Unpacking %s into %s' % ( archive_path, result_dir ) )
    z = zipfile.ZipFile( archive_path, 'r', zipfile.ZIP_DEFLATED ) 
    for f in z.infolist():
        dir = os.path.join( result_dir, os.path.dirname( f.filename ) )
        if not os.path.exists( dir ):
            os.makedirs( dir )
        result = open( os.path.join( result_dir, f.filename ), 'wb' )
        result.write( z.read( f.filename ) )
        result.close()

    z.close()

def boostbook_report( options ):
    site = 'fx.meta-comm.com'
    site_path = '/boost-regression/%s' % options.tag
    
    utils.log( 'Opening %s ...' % site )
    f = ftplib.FTP( site )
    f.login()
    utils.log( '   cd %s ...' % site_path )
    f.cwd( site_path )
    
    utils.log( '   dir' )
    lines = []
    f.dir( lambda x: lines.append( x ) )
    word_lines = [ x.split( None, 8 ) for x in lines ]
    boostbook_info = [ ( l[-1], get_date( l ) ) for l in word_lines if l[-1] == "BoostBook.zip" ]
    if len( boostbook_info ) > 0:
        boostbook_info = boostbook_info[0]
        utils.log( 'BoostBook found! (%s)' % ( boostbook_info, ) )
        local_copy = os.path.join( options.destination,'BoostBook-%s.zip' % options.tag )
        
        if 1: 
            if os.path.exists( local_copy ):
                utils.log( 'Local copy exists. Checking if it is older than uploaded one...' )
                uploaded_mtime = time.mktime( boostbook_info[1] )
                local_mtime    = os.path.getmtime( local_copy )
                utils.log( '    uploaded: %s %s, local: %s %s' % 
                           ( uploaded_mtime
                             , boostbook_info[1]
                             , local_mtime 
                             , time.localtime( local_mtime )) ) 
                modtime = time.localtime( os.path.getmtime( local_copy ) )
                if uploaded_mtime <= local_mtime:
                    utils.log( 'Local copy is newer: exiting' )
                    sys.exit()
                
        if 1:
            temp = os.path.join( options.destination,'BoostBook.zip' )
            result = open( temp, 'wb' )
            f.retrbinary( 'RETR %s' % boostbook_info[0], result.write )
            result.close()
            
            if os.path.exists( local_copy ):
                os.unlink( local_copy )
            os.rename( temp, local_copy )
            m = time.mktime( boostbook_info[1] )
            os.utime( local_copy, ( m, m ) )


        docs_name = os.path.splitext( os.path.basename( local_copy ) )[0]
        if 1:
            unpacked_docs_dir = os.path.join( options.destination, docs_name )
            utils.log( 'Dir %s ' % unpacked_docs_dir )
            if os.path.exists( unpacked_docs_dir ):
                utils.log( 'Cleaning up...' )
                shutil.rmtree( unpacked_docs_dir )
            os.makedirs( unpacked_docs_dir )
            
            unzip( local_copy, unpacked_docs_dir )

        utils.system( [ 'cd %s' % unpacked_docs_dir
                       , 'tar -c -f ../%s.tar.gz -z --exclude=tarball *' % docs_name ] )
        
        process_boostbook_build_log( os.path.join( unpacked_docs_dir, 'boostbook.log' ), read_timestamp( unpacked_docs_dir ) )
        utils.libxslt( log
                         , os.path.abspath( os.path.join( unpacked_docs_dir, 'boostbook.log.xml' ) )
                         , os.path.abspath( os.path.join( os.path.dirname( __file__ ), 'xsl', 'v2', 'boostbook_log.xsl' ) ) 
                         , os.path.abspath( os.path.join( unpacked_docs_dir, 'boostbook.log.html' ) ) )

        
def log( msg ):
    print msg
    
def process_boostbook_build_log( path, timestamp ):
    f = open( path + '.xml', 'w' )
    g = xml.sax.saxutils.XMLGenerator( f )
    lines = open( path ).read().splitlines()
    output_lines = []
    result = 'success'
    for line in lines:
        type = 'output'
        if line.startswith( '...failed' ):
            type = 'failure'
            result='failure'

        if line.startswith( 'runtime error:' ):
            type = 'failure'
    
        if line.startswith( '...skipped' ):
            type = 'skipped'
        output_lines.append( ( type, line ) )
        
    g.startDocument()
    g.startElement( 'build', { 'result':  result, 'timestamp': timestamp } )
    for line in output_lines:
        g.startElement( 'line', { 'type': line[0]} )
        g.characters( line[1] )
        g.endElement( 'line' )
    g.endElement( 'build' )
    g.endDocument()
    
        
def read_timestamp( docs_directory ):
    f = open( os.path.join( docs_directory, 'timestamp' ) )
    try:
        return f.readline()
    finally:
        f.close()
    
def main():
    options = accept_args( sys.argv[1:])
    boostbook_report( options )

if __name__ == '__main__':
    main()
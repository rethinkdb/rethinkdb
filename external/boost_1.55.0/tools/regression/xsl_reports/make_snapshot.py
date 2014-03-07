
# Copyright (c) MetaCommunications, Inc. 2003-2007
#
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

import tarfile
import shutil
import time
import os.path
import string
import sys
import traceback


def retry( f, args, max_attempts=5, sleep_secs=10 ):
    for attempts in range( max_attempts, -1, -1 ):
        try:
            return f( *args )
        except Exception, msg:
            utils.log( '%s failed with message "%s"' % ( f.__name__, msg ) )
            if attempts == 0: 
                utils.log( 'Giving up.' )
                raise

            utils.log( 'Retrying (%d more attempts).' % attempts )
            time.sleep( sleep_secs )


def rmtree( path ):
    if os.path.exists( path ):
        if sys.platform == 'win32':
            os.system( 'del /f /s /q "%s" >nul 2>&1' % path )
            shutil.rmtree( path )
        else:
            os.system( 'rm -f -r "%s"' % path )


def svn_command( command ):
    utils.log( 'Executing SVN command "%s"' % command )
    rc = os.system( command )
    if rc != 0:
        raise Exception( 'SVN command "%s" failed with code %d' % ( command, rc ) )


def svn_export( sources_dir, user, tag ):
    if user is None or user == 'anonymous':
        command = 'svn export --force http://svn.boost.org/svn/boost/%s %s' % ( tag, sources_dir )
    else:
        command = 'svn export --force --non-interactive --username=%s https://svn.boost.org/svn/boost/%s %s' \
                  % ( user, tag, sources_dir )

    os.chdir( os.path.basename( sources_dir ) )
    retry(
         svn_command
       , ( command, )
       )


def make_tarball(
          working_dir
        , tag
        , user
        , site_dir
        ):
    timestamp = time.time()
    timestamp_suffix = time.strftime( '%y-%m-%d-%H%M', time.gmtime( timestamp ) )

    tag_suffix = tag.split( '/' )[-1]
    sources_dir = os.path.join(
          working_dir
        , 'boost-%s-%s' % ( tag_suffix, timestamp_suffix )
        )

    if os.path.exists( sources_dir ):
        utils.log( 'Directory "%s" already exists, cleaning it up...' % sources_dir )
        rmtree( sources_dir )

    try:
        os.mkdir( sources_dir )
        utils.log( 'Exporting files from SVN...' )
        svn_export( sources_dir, user, tag )
    except:
        utils.log( 'Cleaning up...' )
        rmtree( sources_dir )
        raise


    tarball_name = 'boost-%s.tar.bz2' % tag_suffix
    tarball_path = os.path.join( working_dir, tarball_name )

    utils.log( 'Archiving "%s" to "%s"...' % ( sources_dir, tarball_path ) )
    tar = tarfile.open( tarball_path, 'w|bz2' )
    tar.posix = False # see http://tinyurl.com/4ebd8

    tar.add( sources_dir, os.path.basename( sources_dir ) )
    tar.close()

    tarball_timestamp_path = os.path.join( working_dir, 'boost-%s.timestamp' % tag_suffix )

    utils.log( 'Writing timestamp into "%s"...' % tarball_timestamp_path )
    timestamp_file = open( tarball_timestamp_path, 'w' )
    timestamp_file.write( '%f' % timestamp )
    timestamp_file.close()

    md5sum_path = os.path.join( working_dir, 'boost-%s.md5' % tag_suffix )
    utils.log( 'Writing md5 checksum into "%s"...' % md5sum_path )
    old_dir = os.getcwd()
    os.chdir( os.path.dirname( tarball_path ) )
    os.system( 'md5sum -b "%s" >"%s"' % ( os.path.basename( tarball_path ), md5sum_path ) )
    os.chdir( old_dir )
    
    if site_dir is not None:
        utils.log( 'Moving "%s" to the site location "%s"...' % ( tarball_name, site_dir ) )
        temp_site_dir = os.path.join( site_dir, 'temp' )
        if not os.path.exists( temp_site_dir ):
            os.mkdir( temp_site_dir )
                
        shutil.move( tarball_path, temp_site_dir )
        shutil.move( os.path.join( temp_site_dir, tarball_name ), site_dir )
        shutil.move( tarball_timestamp_path, site_dir )
        shutil.move( md5sum_path, site_dir )
        utils.log( 'Removing "%s"...' % sources_dir )
        rmtree( sources_dir )


def accept_args( args ):
    args_spec = [ 
          'working-dir='
        , 'tag='
        , 'user='
        , 'site-dir='
        , 'mail='
        , 'help'
        ]
        
    options = { 
          '--tag': 'trunk'
        , '--user': None
        , '--site-dir': None
        }
    
    utils.accept_args( args_spec, args, options, usage )
        
    return ( 
          options[ '--working-dir' ]
        , options[ '--tag' ]
        , options[ '--user' ]
        , options[ '--site-dir' ]
        )


def usage():
    print 'Usage: %s [options]' % os.path.basename( sys.argv[0] )
    print    '''
\t--working-dir   working directory
\t--tag           snapshot tag (i.e. 'trunk')
\t--user          Boost SVN user ID (optional)
\t--site-dir      site directory to copy the snapshot to (optional)
'''

def main():
    make_tarball( *accept_args( sys.argv[ 1: ] ) )

if __name__ != '__main__':  import utils
else:
    # in absense of relative import...
    xsl_path = os.path.abspath( os.path.dirname( sys.argv[ 0 ] ) )
    while os.path.basename( xsl_path ) != 'xsl_reports': xsl_path = os.path.dirname( xsl_path )
    sys.path.append( xsl_path )

    import utils
    main()

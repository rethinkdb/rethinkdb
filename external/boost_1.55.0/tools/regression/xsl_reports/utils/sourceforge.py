
import utils.checked_system
import os
import sys

site_dir = '/home/groups/b/bo/boost/htdocs/'

def download( source, destination, user ):
    if sys.platform == 'win32':
        destination = os.popen( 'cygpath "%s"' % destination ).read().splitlines()[0]

    utils.checked_system( [ 
          'rsync -v -r -z --progress %(user)s@shell.sourceforge.net:%(site_dir)s%(source)s %(dest)s'
                % { 'user': user, 'site_dir': site_dir, 'source': source, 'dest': destination }
        ] )


def upload( source, destination, user ):
    if sys.platform == 'win32':
        source = os.popen( 'cygpath "%s"' % source ).read().splitlines()[0]
        
    utils.checked_system( [ 
          'rsync -v -r -z --progress %(source)s %(user)s@shell.sourceforge.net:%(site_dir)s%(dest)s'
                % { 'user': user, 'site_dir': site_dir, 'source': source, 'dest': destination }
        ] )


def checked_system( commands, user, background = False ):
    if not background:
        cmd = 'ssh -l %s shell.sourceforge.net "%s"'
    else:
        cmd = 'ssh -f -l %s shell.sourceforge.net "%s"'

    utils.checked_system( 
          [ cmd % ( user, '&&'.join( commands ) ) ]
        )


def untar( archive, user, background ):
    checked_system( 
          [
              'cd %s' % os.path.join( site_dir, os.path.dirname( archive ) )
            , 'tar -x -z --overwrite --mode=+w -f %s' % os.path.basename( archive )
            , 'rm -f %s' % archive
            ]
        , user = user
        , background = background
        )

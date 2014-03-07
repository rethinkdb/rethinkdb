
import utils.checked_system
import os.path

def tar( source_dir, archive_name ):
    utils.checked_system( [
          'cd %s' % source_dir
        , 'tar -c -f ../%s -z *' % archive_name
        ] )

def untar( archive_path ):
    #utils.checked_system( [ 'tar -xjf "%s"' % archive_path ] )
    utils.checked_system( [ 
          'cd %s' % os.path.dirname( archive_path )
        , 'tar -xjf "%s"' % os.path.basename( archive_path )
        ] )

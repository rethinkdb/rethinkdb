
import zipfile
import os.path

def unzip( archive_path, result_dir ):
    z = zipfile.ZipFile( archive_path, 'r', zipfile.ZIP_DEFLATED ) 
    for f in z.infolist():
        result = open( os.path.join( result_dir, f.filename ), 'wb' )
        result.write( z.read( f.filename ) )
        result.close()

    z.close()

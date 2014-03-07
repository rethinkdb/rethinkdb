
import os

def check_existance( name ):
    a = os.popen( '%s --version' % name )
    output = a.read()
    rc = a.close()
    if rc is not None:
        raise Exception( '"%s" is required' % name )

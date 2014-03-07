
import inspect
import sys

def log_level():
   frames = inspect.stack()
   level = 0
   for i in frames[ 3: ]:
       if i[0].f_locals.has_key( '__log__' ):
           level = level + i[0].f_locals[ '__log__' ]
   return level


def stdlog( message ):
    sys.stderr.write( '# ' + '    ' * log_level() +  message + '\n' )
    sys.stderr.flush()

log = stdlog

try:
    import locale
    locale.setlocale(locale.LC_ALL, '')
except:
    pass


# try to import a litre_config.py file.
try:
    import litre_config as config
except:
    class config: pass
    
import sys
try: # if the user has set up docutils_root in his config, add it to the PYTHONPATH.
    sys.path += ['%s/docutils' % config.docutils_root
                 , '%s/docutils/extras' % config.docutils_root]
except: pass

import docutils.writers
import cplusplus
import os

from docutils.core import publish_cmdline, default_description

description = ('Literate programming from ReStructuredText '
               'sources.  ' + default_description)

def _pop_option(prefix):
    found = None
    for opt in sys.argv:
        if opt.startswith(prefix):
            sys.argv = [ x for x in sys.argv if x != opt ]
            found = opt
            if prefix.endswith('='):
                found = opt[len(prefix):]
    return found


dump_dir = _pop_option('--dump_dir=')
max_output_lines = _pop_option('--max_output_lines=')

if dump_dir:
    
    cplusplus.Writer.translator = cplusplus.DumpTranslator
    if _pop_option('--workaround'):
        cplusplus.Writer.translator = cplusplus.WorkaroundTranslator
        config.includes.insert(0, os.path.join(os.path.split(dump_dir)[0], 'patches'))
        
    config.dump_dir = os.path.abspath(dump_dir)
    if _pop_option('--cleanup_source'):
        config.line_hash = None
        
    if not os.path.exists(config.dump_dir):
        os.makedirs(config.dump_dir)

if max_output_lines:
    config.max_output_lines = int(max_output_lines)

config.bjam_options = _pop_option('--bjam=')

config.includes = []

publish_cmdline(
    writer=cplusplus.Writer(config),
    description=description
    )

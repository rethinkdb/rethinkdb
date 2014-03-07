# Copyright (c) Aleksey Gurtovoy 2008-2009
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import shutil
import os


def build( src_dir, build_dir ):

    src = os.path.join( build_dir, 'refmanual.gen' )
    
    def cleanup():
        if os.path.exists( src ):
            os.unlink( src )

        if os.path.exists( build_dir ):
            shutil.rmtree( build_dir )

    def generate_html():
        os.mkdir( build_dir )
        os.chdir( build_dir )
        os.system( 'python %s %s' % ( os.path.join( src_dir, 'refmanual.py' ), build_dir ) )
        os.system( 'rst2htmlrefdoc.py --traceback -g -d -t --no-frames --dont-copy-stylesheet --stylesheet-path=style.css %s refmanual.html' % src ) 

    cleanup()
    generate_html()


build(
      os.path.join( os.getcwd(), 'refmanual' )
    , os.path.join( os.getcwd(), '_build' )
    )

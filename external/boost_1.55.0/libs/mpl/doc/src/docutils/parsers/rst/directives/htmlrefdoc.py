
# Copyright Aleksey Gurtovoy 2009
#
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

from docutils.parsers.rst import Directive
from docutils import nodes


class license_and_copyright( nodes.General, nodes.Element ): pass

class LicenseAndCopyright( Directive ):
    has_content = True

    def run( self ):
        self.assert_has_content()
        result_node = license_and_copyright( rawsource = '\n'.join( self.content ) )
        self.state.nested_parse( self.content, self.content_offset, result_node )
        return [ result_node ]

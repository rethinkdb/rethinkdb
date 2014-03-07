
# Copyright Aleksey Gurtovoy 2004-2009
#
# Distributed under the Boost Software License, Version 1.0. 
# (See accompanying file LICENSE_1_0.txt or copy at 
# http://www.boost.org/LICENSE_1_0.txt)

from docutils.writers import html4_frames
from docutils.writers import html4css1
from docutils import nodes

import re
import string


class Writer(html4_frames.Writer):
        
    def __init__(self):
        self.__super = html4_frames.Writer
        self.__super.__init__(self)
        self.translator = refdoc_translator


class refdoc_translator(html4_frames.frame_pages_translator):

    tocframe_width      = 25
    re_include          = re.compile(r'(\s*#include\s+<)(.*?\.hpp)?(>\s*)?')
    re_identifier       = re.compile(r'(.*?\W*)(\w+)(\W.*?)?')
    re_modtime          = re.compile(r'\s*modtime:\s*(.*)')
    re_auto_id          = re.compile(r'^id(\d+)$')
    in_literal_block    = 0
    in_reference        = 0
    
    def __init__(self, document, index_page, page_files_dir, extension):
        self.docframe += ' refmanual'
        self.__super = html4_frames.frame_pages_translator
        self.__super.__init__(self, document, index_page, page_files_dir, extension)


    def visit_section( self, node ):
        base = self
        self = self.active_visitor()
        if self.section_level == 1:
            self.section_level = 2           
        base.__super.visit_section( base, node )
        
    def depart_section( self, node ):
        self.__super.depart_section( self, node )
        self = self.active_visitor()
        if self.section_level == 2:
            self.section_level = 1


    def visit_title( self, node ):
        self.__super.visit_title( self, node )        
        if self.re_auto_id.match( self._node_id( node.parent ) ):
            name = nodes.make_id( node.astext() )
            self = self.active_visitor()
            self.body.append( self.starttag(
                  {}, 'a', '', name=name, href='#%s' % name, CLASS='subsection-title'
                ) )

    def depart_title( self, node ):
        base = self
        if self.re_auto_id.match( self._node_id( node.parent ) ):
            self = self.active_visitor()
            self.body.append( '</a>')

        base.__super.depart_title( base, node )


    def visit_table(self, node):
        self = self.active_visitor()
        self.body.append(
            self.starttag(node, 'table', CLASS='docutils table', border="1"))
 

    def visit_reference(self, node):
        self.in_reference = 1
        if len(node) == 1 and isinstance(node[0], nodes.literal) and node[0].has_key('class'):
            if node.has_key('class') and node['class'].find(node[0]['class']) == -1:
                node['class'] += ' %s' % node[0]['class']
            else:
                node['class'] = node[0]['class']
            
        self.__super.visit_reference(self, node)


    def depart_reference(self, node):
        self.__super.depart_reference(self, node)
        self.in_reference = 0


    def visit_literal(self, node):
        if self.in_reference:
            self.__super.visit_literal(self, node)
     
        base = self
        self = self.active_visitor()

        self.body.append(self.starttag(node, 'tt', '', CLASS='literal'))
        text = node.astext()

        if base.re_include.search(text):
            text = base.re_include.sub(lambda m: base._handle_include_sub(self, m), text)
            self.body.append('<span class="pre">%s</span>' % text)
        else:
            for token in self.words_and_spaces.findall(text):
                if token.strip():
                    if base.re_identifier.search(token):
                        token = base.re_identifier.sub(lambda m: base._handle_id_sub(self, m), token)
                    else:
                        token = self.encode(token)

                    self.body.append('<span class="pre">%s</span>' % token)
                elif token in ('\n', ' '):
                    # Allow breaks at whitespace:
                    self.body.append(token)
                else:
                    # Protect runs of multiple spaces; the last space can wrap:
                    self.body.append('&nbsp;' * (len(token) - 1) + ' ')

        self.body.append('</tt>')
        # Content already processed:
        raise nodes.SkipNode


    def visit_literal_block(self, node):
        self.__super.visit_literal_block(self, node)
        self.in_literal_block = True

    def depart_literal_block(self, node):
        self.__super.depart_literal_block(self, node)
        self.in_literal_block = False


    def visit_license_and_copyright(self, node):
        self = self.active_visitor()
        self.context.append( len( self.body ) )

    def depart_license_and_copyright(self, node):
        self = self.active_visitor()
        start = self.context.pop()
        self.footer = self.body[start:]
        del self.body[start:]


    def visit_Text(self, node):
        if not self.in_literal_block:
            self.__super.visit_Text(self, node)
        else:
            base = self
            self = self.active_visitor()
            
            text = node.astext()            
            if base.re_include.search(text):
                text = base.re_include.sub(lambda m: base._handle_include_sub(self, m), text)
            elif base.re_identifier.search(text):
                text = base.re_identifier.sub(lambda m: base._handle_id_sub(self, m), text)
            else:
                text = self.encode(text)
            
            self.body.append(text)


    def depart_Text(self, node):
        pass


    def visit_substitution_reference(self, node):
        # debug help
        print 'Unresolved substitution_reference:', node.astext()
        raise nodes.SkipNode


    def _footer_content(self):    
        self = self.active_visitor()
        parts = ''.join( self.footer ).split( '\n' )
        parts = [ '<div class="copyright">%s</div>' % x if x.startswith( 'Copyright' ) else x for x in parts ]        
        return '<td><div class="copyright-footer">%s</div></td>' % '\n'.join( parts ) if len( parts ) else ''


    def _toc_as_text( self, visitor ):
        footer_end = visitor.body.pop()
        visitor.body.append( self._footer_content() )
        visitor.body.append( footer_end )
        return visitor.astext()


    def _handle_include_sub(base, self, match):
        if not match.group(2) or not match.group():
            return self.encode(match.group(0))

        header = match.group(2)
        result = self.encode(match.group(1))
        result += '<a href="%s" class="header">%s</a>' \
                        % ( '../../../../%s' % header
                          , self.encode(header)
                          )
        
        result += self.encode(match.group(3))
        return result

    
    def _handle_id_sub(base, self, match):
        identifier = match.group(2)        

        if not base.document.has_name( identifier.lower() ):
            return self.encode(match.group(0))

        def get_section_id( id ):
            node = base.document.ids[ id ]
            if isinstance( node, nodes.section ):
                return id

            if isinstance( node, nodes.target ):
                return get_section_id( node.get( 'refid' ) )

            return None

        id = get_section_id( base.document.nameids[ identifier.lower() ] )
        if not id:
            return self.encode(match.group(0))

        if id == 'inserter':
            id = 'inserter-class'

        result = self.encode(match.group(1))
        result += '<a href="%s" class="identifier">%s</a>' \
                        % ( base._chunk_ref( base._active_chunk_id(), base._make_chunk_id( id ) )
                          , self.encode(identifier)
                          )
        
        if match.group(3):
            result += self.encode(match.group(3))

        return result


    def _make_chunk_id( self, node_id ):
        if self.re_auto_id.match( node_id ):
            node = self.document.ids[ node_id ]
            return '%s-%s' % ( self._node_id( node.parent ), node['dupnames'][0] )

        if node_id.startswith( 'boost-mpl-' ):
            return node_id[ len( 'boost-mpl-' ): ]

        return node_id


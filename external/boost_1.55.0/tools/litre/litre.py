from docutils import writers
from docutils import nodes

class LitreTranslator(nodes.GenericNodeVisitor):

    def __init__(self, document, config):
        nodes.GenericNodeVisitor.__init__(self,document)
        self._config = config
        
    def default_visit(self, node):
        pass
        # print '**visiting:', repr(node)

    def default_departure(self, node):
        pass
        # print '**departing:', repr(node)

    def visit_raw(self, node):
        if node.has_key('format'):
            key = node['format'].lower()
            if key == 'litre':
                # This is probably very evil ;-)
                #if node.has_key('source'):
                #    node.file = node.attributes['source']
                    
                self._handle_code(node, node.astext())
                
        raise nodes.SkipNode

    def visit_comment(self, node):
        code = node.astext()
        if code[0] == '@':
            self._handle_code(node, code[1:].strip())
                              
    def _handle_code(self, node, code):
        start_line = node.line or 0
        start_line -= code.count('\n') + 2  # docutils bug workaround?
        try:
            self._execute(compile( start_line*'\n' + code, str(node.source), 'exec'))
        except:
            print '\n------- begin offending Python source -------'
            print code            
            print '------- end offending Python source -------'
            raise
            
    def _execute(self, code):
        """Override this to set up local variable context for code before
        invoking it
        """
        eval(code)
        
class Writer(writers.Writer):
    translator = LitreTranslator
    _config = None
    
    def translate(self):
        visitor = self.translator(self.document, self._config)
        self.document.walkabout(visitor)
        self.output = visitor.astext()

    

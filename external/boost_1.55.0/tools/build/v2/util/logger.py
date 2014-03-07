# Copyright Pedro Ferreira 2005. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

import sys

class NullLogger:
    def __init__ (self):
        self.indent_ = ''
    
    def log (self, source_name, *args):
        if self.on () and self.interesting (source_name):
            self.do_log (self.indent_)
            for i in args:
                self.do_log (i)
            self.do_log ('\n')
    
    def increase_indent (self):
        if self.on ():
            self.indent_ += '    '
        
    def decrease_indent (self):
        if self.on () and len (self.indent_) > 4:
            self.indent_ = self.indent_ [-4:]

    def do_log (self, *args):
        pass
    
    def interesting (self, source_name):
        return False

    def on (self):
        return True

class TextLogger (NullLogger):
    def __init__ (self):
        NullLogger.__init__ (self)
    
    def do_log (self, arg):
            sys.stdout.write (str (arg))
    
    def interesting (self, source_name):
        return True

    def on (self):
        return True

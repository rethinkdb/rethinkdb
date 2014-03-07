# Copyright David Abrahams 2004. 
# Copyright Daniel Wallin 2006.
# Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

import os
import tempfile
import litre
import re
import sys
import traceback

# Thanks to Jean Brouwers for this snippet
def _caller(up=0):
    '''Get file name, line number, function name and
       source text of the caller's caller as 4-tuple:
       (file, line, func, text).

       The optional argument 'up' allows retrieval of 
       a caller further back up into the call stack.

       Note, the source text may be None and function
       name may be '?' in the returned result.  In
       Python 2.3+ the file name may be an absolute
       path.
    '''
    try:  # just get a few frames'
        f = traceback.extract_stack(limit=up+2)
        if f:
           return f[0]
    except:
        pass
    # running with psyco?
    return ('', 0, '', None)

class Example:
    closed = False
    in_emph = None
    
    def __init__(self, node, section, line_offset, line_hash = '#'):
        # A list of text fragments comprising the Example.  Start with a #line
        # directive
        self.section = section
        self.line_hash = line_hash
        self.node = node
        self.body = []
        self.line_offset = line_offset
        self._number_of_prefixes = 0

        self.emphasized = [] # indices of text strings that have been
                             # emphasized.  These are generally expected to be
                             # invalid C++ and will need special treatment

    def begin_emphasis(self):
        self.in_emph = len(self.body)

    def end_emphasis(self):
        self.emphasized.append( (self.in_emph, len(self.body)) )
        
    def append(self, s):
        self.append_raw(self._make_line(s))

    def prepend(self, s):
        self.prepend_raw(self._make_line(s))

    def append_raw(self, s):
        self.body.append(s)

    def prepend_raw(self, s):
        self.body.insert(0,s)
        self.emphasized = [ (x[0]+1,x[1]+1) for x in self.emphasized ]
        self._number_of_prefixes += 1

    def replace(self, s1, s2):
        self.body = [x.replace(s1,s2) for x in self.body]
        
    def sub(self, pattern, repl, count = 1, flags = re.MULTILINE):
        pat = re.compile(pattern, flags)
        for i,txt in enumerate(self.body):
            if count > 0:
                x, subs = pat.subn(repl, txt, count)
                self.body[i] = x
                count -= subs
        
    def wrap(self, s1, s2):
        self.append_raw(self._make_line(s2))
        self.prepend_raw(self._make_line(s1, offset = -s1.count('\n')))
        
    def replace_emphasis(self, s, index = 0):
        """replace the index'th emphasized text with s"""
        e = self.emphasized[index]
        self.body[e[0]:e[1]] = [s]
        del self.emphasized[index]

    elipsis = re.compile('^([ \t]*)([.][.][.][ \t]*)$', re.MULTILINE)
    
    def __str__(self):
        # Comment out any remaining emphasized sections
        b = [self.elipsis.sub(r'\1// \2', s) for s in self.body]
        emph = self.emphasized
        emph.reverse()
        for e in emph:
            b.insert(e[1], ' */')
            b.insert(e[0], '/* ')
        emph.reverse()

        # Add initial #line
        b.insert(
            self._number_of_prefixes,
            self._line_directive(self.node.line, self.node.source)
            )

        # Add trailing newline to avoid warnings
        b.append('\n')
        return ''.join(b)

    def __repr__(self):
        return "Example: " + repr(str(self))

    def raw(self):
        return ''.join(self.body)

    def _make_line(self, s, offset = 0):
        c = _caller(2)[1::-1]
        offset -= s.count('\n')
        return '\n%s%s\n' % (self._line_directive(offset = offset, *c), s.strip('\n'))

    def _line_directive(self, line, source, offset = None):
        if self.line_hash is None:
            return '\n'
        
        if offset is None:
            offset = self.line_offset
            
        if line is None or line <= -offset:
            line = 1
        else:
            line += offset

        if source is None:
            return '%sline %d\n' % (self.line_hash, line)
        else:
            return '%sline %d "%s"\n' % (self.line_hash, line, source)


def syscmd(
      cmd
    , expect_error = False
    , input = None
    , max_output_lines = None
    ):

    # On windows close() returns the exit code, on *nix it doesn't so
    # we need to use popen2.Popen4 instead.
    if sys.platform == 'win32':
        stdin, stdout_stderr = os.popen4(cmd)
        if input: stdin.write(input)
        stdin.close()
        
        out = stdout_stderr.read()
        status = stdout_stderr.close()
    else:
        import popen2
        process = popen2.Popen4(cmd)
        if input: process.tochild.write(input)
        out = process.fromchild.read()
        status = process.wait()

    if max_output_lines is not None:
        out = '\n'.join(out.split('\n')[:max_output_lines])
        
    if expect_error:
        status = not status

    if status:
        print
        print '========== offending command ==========='
        print cmd
        print '------------ stdout/stderr -------------'
        print expect_error and 'Error expected, but none seen' or out
    elif expect_error > 1:
        print
        print '------ Output of Expected Error --------'
        print out
        print '----------------------------------------'
        
    sys.stdout.flush()
        
    return (status,out)


def expand_vars(path):
    if os.name == 'nt':
        re_env = re.compile(r'%\w+%')
        return re_env.sub(
              lambda m: os.environ.get( m.group(0)[1:-1] )
            , path
            )
    else:
        return os.path.expandvars(path)

def remove_directory_and_contents(path):
    for root, dirs, files in os.walk(path, topdown=False):
        for name in files:
            os.remove(os.path.join(root, name))
        for name in dirs:
            os.rmdir(os.path.join(root, name))
    os.rmdir(path)

class BuildResult:
    def __init__(self, path):
        self.path = path

    def __repr__(self):
        return self.path

    def __del__(self):
        remove_directory_and_contents(self.path)

class CPlusPlusTranslator(litre.LitreTranslator):

    _exposed_attrs = ['compile', 'test', 'ignore', 'match_stdout', 'stack', 'config'
                      , 'example', 'prefix', 'preprocessors', 'litre_directory',
                      'litre_translator', 'includes', 'build', 'jam_prefix',
                      'run_python']

    last_run_output = ''

    """Attributes that will be made available to litre code"""
    
    def __init__(self, document, config):
        litre.LitreTranslator.__init__(self, document, config)
        self.in_literal = False
        self.in_table = True
        self.preprocessors = []
        self.stack = []
        self.example = None
        self.prefix = []
        self.includes = config.includes
        self.litre_directory = os.path.split(__file__)[0]
        self.config = config
        self.litre_translator = self
        self.line_offset = 0
        self.last_source = None
        self.jam_prefix = []

        self.globals = { 'test_literals_in_tables' : False }
        for m in self._exposed_attrs:
            self.globals[m] = getattr(self, m)

        self.examples = {}
        self.current_section = None

    #
    # Stuff for use by docutils writer framework
    #
    def visit_emphasis(self, node):
        if self.in_literal:
            self.example.begin_emphasis()
        
    def depart_emphasis(self, node):
        if self.in_literal:
            self.example.end_emphasis()

    def visit_section(self, node):
        self.current_section = node['ids'][0]

    def visit_literal_block(self, node):
        if node.source is None:
            node.source = self.last_source
        self.last_source = node.source

        # create a new example
        self.example = Example(node, self.current_section, line_offset = self.line_offset, line_hash = self.config.line_hash)

        self.stack.append(self.example)

        self.in_literal = True

    def depart_literal_block(self, node):
        self.in_literal = False

    def visit_literal(self, node):
        if self.in_table and self.globals['test_literals_in_tables']:
            self.visit_literal_block(node)
        else:
            litre.LitreTranslator.visit_literal(self,node)
        
    def depart_literal(self, node):
        if self.in_table and self.globals['test_literals_in_tables']:
            self.depart_literal_block(node)
        else:
            litre.LitreTranslator.depart_literal(self,node)

    def visit_table(self,node):
        self.in_table = True
        litre.LitreTranslator.visit_table(self,node)
        
    def depart_table(self,node):
        self.in_table = False
        litre.LitreTranslator.depart_table(self,node)
        
    def visit_Text(self, node):
        if self.in_literal:
            self.example.append_raw(node.astext())

    def depart_document(self, node):
        self.write_examples()
           
    #
    # Private stuff
    #
            
    def handled(self, n = 1):
        r = self.stack[-n:]
        del self.stack[-n:]
        return r

    def _execute(self, code):
        """Override of litre._execute; sets up variable context before
        evaluating code
        """
        self.globals['example'] = self.example
        eval(code, self.globals)
        
    #
    # Stuff for use by embedded python code
    #

    def match_stdout(self, expected = None):

        if expected is None:
            expected = self.example.raw()
            self.handled()

        if not re.search(expected, self.last_run_output, re.MULTILINE):
        #if self.last_run_output.strip('\n') != expected.strip('\n'):
            print 'output failed to match example'
            print '-------- Actual Output -------------'
            print repr(self.last_run_output)
            print '-------- Expected Output -----------'
            print repr(expected)
            print '------------------------------------'
            sys.stdout.flush()
            
    def ignore(self, n = 1):
        if n == 'all':
            n = len(self.stack)
        return self.handled(n)

    def wrap(self, n, s1, s2):
        self.stack[-1].append(s2)
        self.stack[-n].prepend(s1)
        

    def compile(
          self
        , howmany = 1
        , pop = -1
        , expect_error = False
        , extension = '.o'
        , options = ['-c']
        , built_handler = lambda built_file: None
        , source_file = None
        , source_suffix = '.cpp'
          # C-style comments by default; handles C++ and YACC
        , make_comment = lambda text: '/*\n%s\n*/' % text 
        , built_file = None
        , command = None
        ):
        """
        Compile examples on the stack, whose topmost item is the last example
        seen but not yet handled so far.

        :howmany: How many of the topmost examples on the stack to compile.
           You can pass a number, or 'all' to indicate that all examples should
           be compiled.

        :pop: How many of the topmost examples to discard.  By default, all of
           the examples that are compiled are discarded.
        
        :expect_error: Whether a compilation error is to be expected.  Any value
           > 1 will cause the expected diagnostic's text to be dumped for
           diagnostic purposes.  It's common to expect an error but see a
           completely unrelated one because of bugs in the example (you can get
           this behavior for all examples by setting show_expected_error_output
           in your config).

        :extension: The extension of the file to build (set to .exe for
           run)

        :options: Compiler flags

        :built_file: A path to use for the built file.  By default, a temp
            filename is conjured up
            
        :built_handler: A function that's called with the name of the built file
           upon success.

        :source_file: The full name of the source file to write
        
        :source_suffix: If source_file is None, the suffix to use for the source file

        :make_comment: A function that transforms text into an appropriate comment.
        
        :command: A function that is passed (includes, opts, target, source), where
           opts is a string representing compiler options, target is the name of
           the file to build, and source is the name of the file into which the
           example code is written.  By default, the function formats
           litre.config.compiler with its argument tuple.
        """

        # Grab one example by default
        if howmany == 'all':
            howmany = len(self.stack)

        source = '\n'.join(
            self.prefix
            + [str(x) for x in self.stack[-howmany:]]
            )

        source = reduce(lambda s, f: f(s), self.preprocessors, source)
        
        if pop:
            if pop < 0:
                pop = howmany
            del self.stack[-pop:]
            
        if len(self.stack):
            self.example = self.stack[-1]

        cpp = self._source_file_path(source_file, source_suffix)
            
        if built_file is None:
            built_file = self._output_file_path(source_file, extension)
        
        opts = ' '.join(options)
        
        includes = ' '.join(['-I%s' % d for d in self.includes])
        if not command:
            command = self.config.compiler

        if type(command) == str:
            command = lambda i, o, t, s, c = command: c % (i, o, t, s)
            
        cmd = command(includes, opts, expand_vars(built_file), expand_vars(cpp))

        if expect_error and self.config.show_expected_error_output:
            expect_error += 1


        comment_cmd = command(includes, opts, built_file, os.path.basename(cpp))
        comment = make_comment(config.comment_text(comment_cmd, expect_error))

        self._write_source(cpp, '\n'.join([comment, source]))

        #print 'wrote in', cpp
        #print 'trying command', cmd

        status, output = syscmd(cmd, expect_error)

        if status or expect_error > 1:
            print
            if expect_error and expect_error < 2:
                print 'Compilation failure expected, but none seen'
            print '------------ begin offending source ------------'
            print open(cpp).read()
            print '------------ end offending source ------------'
            
            if self.config.save_cpp:
                print 'saved in', repr(cpp)
            else:
                self._remove_source(cpp)

            sys.stdout.flush()
        else:
            print '.',
            sys.stdout.flush()
            built_handler(built_file)
                
            self._remove_source(cpp)
            
        try:
            self._unlink(built_file)
        except:
            if not expect_error:
                print 'failed to unlink', built_file
        
        return status

    def test(
          self
        , rule = 'run'
        , howmany = 1
        , pop = -1
        , expect_error = False
        , requirements = ''
        , input = ''
        ):

        # Grab one example by default
        if howmany == 'all':
            howmany = len(self.stack)

        source = '\n'.join(
            self.prefix
            + [str(x) for x in self.stack[-howmany:]]
            )

        source = reduce(lambda s, f: f(s), self.preprocessors, source)

        id = self.example.section
        if not id:
            id = 'top-level'

        if not self.examples.has_key(self.example.section):
            self.examples[id] = [(rule, source)]
        else:
            self.examples[id].append((rule, source))

        if pop:
            if pop < 0:
                pop = howmany
            del self.stack[-pop:]

        if len(self.stack):
            self.example = self.stack[-1]

    def write_examples(self):
        jam = open(os.path.join(self.config.dump_dir, 'Jamfile.v2'), 'w')

        jam.write('''
import testing ;

''')

        for id,examples in self.examples.items():
            for i in range(len(examples)):
                cpp = '%s%d.cpp' % (id, i)

                jam.write('%s %s ;\n' % (examples[i][0], cpp))

                outfile = os.path.join(self.config.dump_dir, cpp)
                print cpp,
                try:
                    if open(outfile, 'r').read() == examples[i][1]:
                        print ' .. skip'
                        continue
                except:
                    pass

                open(outfile, 'w').write(examples[i][1])
                print ' .. written'

        jam.close()

    def build(
          self
        , howmany = 1
        , pop = -1
        , source_file = 'example.cpp'
        , expect_error = False
        , target_rule = 'obj'
        , requirements = ''
        , input = ''
        , output = 'example_output'
        ):

        # Grab one example by default
        if howmany == 'all':
            howmany = len(self.stack)

        source = '\n'.join(
            self.prefix
            + [str(x) for x in self.stack[-howmany:]]
            )

        source = reduce(lambda s, f: f(s), self.preprocessors, source)

        if pop:
            if pop < 0:
                pop = howmany
            del self.stack[-pop:]

        if len(self.stack):
            self.example = self.stack[-1]

        dir = tempfile.mkdtemp()
        cpp = os.path.join(dir, source_file)
        self._write_source(cpp, source)
        self._write_jamfile(
            dir
          , target_rule = target_rule
          , requirements = requirements
          , input = input
          , output = output
          )

        cmd = 'bjam'
        if self.config.bjam_options:
            cmd += ' %s' % self.config.bjam_options

        os.chdir(dir)
        status, output = syscmd(cmd, expect_error)

        if status or expect_error > 1:
            print
            if expect_error and expect_error < 2:
                print 'Compilation failure expected, but none seen'
            print '------------ begin offending source ------------'
            print open(cpp).read()
            print '------------ begin offending Jamfile -----------'
            print open(os.path.join(dir, 'Jamroot')).read()
            print '------------ end offending Jamfile -------------'

            sys.stdout.flush()
        else:
            print '.',
            sys.stdout.flush()

        if status: return None
        else: return BuildResult(dir)

    def _write_jamfile(self, path, target_rule, requirements, input, output):
        jamfile = open(os.path.join(path, 'Jamroot'), 'w')
        contents = r"""
import modules ;

BOOST_ROOT = [ modules.peek : BOOST_ROOT ] ;
use-project /boost : $(BOOST_ROOT) ;

%s

%s %s
  : example.cpp %s
  : <include>.
    %s
    %s
  ;
        """ % (
            '\n'.join(self.jam_prefix)
          , target_rule
          , output
          , input
          , ' '.join(['<include>%s' % d for d in self.includes])
          , requirements
          )

        jamfile.write(contents)

    def run_python(
          self
        , howmany = 1
        , pop = -1
        , module_path = []
        , expect_error = False
        ):
        # Grab one example by default
        if howmany == 'all':
            howmany = len(self.stack)

        if module_path == None: module_path = []

        if isinstance(module_path, BuildResult) or type(module_path) == str:
            module_path = [module_path]

        module_path = map(lambda p: str(p), module_path)

        source = '\n'.join(
            self.prefix
            + [str(x) for x in self.stack[-howmany:]]
            )

        if pop:
            if pop < 0:
                pop = howmany
            del self.stack[-pop:]

        if len(self.stack):
            self.example = self.stack[-1]

        r = re.compile(r'^(>>>|\.\.\.) (.*)$', re.MULTILINE)
        source = r.sub(r'\2', source)
        py = self._source_file_path(source_file = None, source_suffix = 'py')
        open(py, 'w').write(source)

        old_path = os.getenv('PYTHONPATH')
        if old_path == None:
            pythonpath = ':'.join(module_path)
            old_path = ''
        else:
            pythonpath = old_path + ':%s' % ':'.join(module_path)

        os.putenv('PYTHONPATH', pythonpath)
        status, output = syscmd('python %s' % py)

        if status or expect_error > 1:
            print
            if expect_error and expect_error < 2:
                print 'Compilation failure expected, but none seen'
            print '------------ begin offending source ------------'
            print open(py).read()
            print '------------ end offending Jamfile -------------'

            sys.stdout.flush()
        else:
            print '.',
            sys.stdout.flush()

        self.last_run_output = output
        os.putenv('PYTHONPATH', old_path)
        self._unlink(py)

    def _write_source(self, filename, contents):
        open(filename,'w').write(contents)
        
    def _remove_source(self, source_path):
        os.unlink(source_path)

    def _source_file_path(self, source_file, source_suffix):
        if source_file is None:
            cpp = tempfile.mktemp(suffix=source_suffix)
        else:
            cpp = os.path.join(tempfile.gettempdir(), source_file)
        return cpp
    
    def _output_file_path(self, source_file, extension):
        return tempfile.mktemp(suffix=extension)

    def _unlink(self, file):
        file = expand_vars(file)
        if os.path.exists(file):
            os.unlink(file)
    
    def _launch(self, exe, stdin = None):
        status, output = syscmd(exe, input = stdin)
        self.last_run_output = output
        
    def run_(self, howmany = 1, stdin = None, **kw):
        new_kw = { 'options':[], 'extension':'.exe' }
        new_kw.update(kw)

        self.compile(
            howmany
          , built_handler = lambda exe: self._launch(exe, stdin = stdin)
          , **new_kw
        )

    def astext(self):
        return ""
        return '\n\n ---------------- Unhandled Fragment ------------ \n\n'.join(
            [''] # generates a leading announcement
            + [ unicode(s) for s in self.stack]
            )

class DumpTranslator(CPlusPlusTranslator):
    example_index = 1
    
    def _source_file_path(self, source_file, source_suffix):
        if source_file is None:
            source_file = 'example%s%s' % (self.example_index, source_suffix)
            self.example_index += 1
            
        cpp = os.path.join(config.dump_dir, source_file)
        return cpp

    def _output_file_path(self, source_file, extension):
        chapter = os.path.basename(config.dump_dir)
        return '%%TEMP%%\metaprogram-%s-example%s%s' \
            % ( chapter, self.example_index - 1, extension)

    def _remove_source(self, source_path):
        pass


class WorkaroundTranslator(DumpTranslator):
    """Translator used to test/dump workaround examples for vc6 and vc7.  Just
    like a DumpTranslator except that we leave existing files alone.

    Warning: not sensitive to changes in .rst source!!  If you change the actual
    examples in source files you will have to move the example files out of the
    way and regenerate them, then re-incorporate the workarounds.
    """
    def _write_source(self, filename, contents):
        if not os.path.exists(filename):
            DumpTranslator._write_source(self, filename, contents)
    
class Config:
    save_cpp = False
    line_hash = '#'
    show_expected_error_output = False
    max_output_lines = None

class Writer(litre.Writer):
    translator = CPlusPlusTranslator
    
    def __init__(
        self
      , config
    ):
        litre.Writer.__init__(self)
        self._config = Config()
        defaults = Config.__dict__

        # update config elements
        self._config.__dict__.update(config.__dict__)
#             dict([i for i in config.__dict__.items()
#                   if i[0] in config.__all__]))
        

#!/usr/bin/env python
from __future__ import print_function
'''
Generates AST terms and serialization code for the Java driver
'''

import re
import os
import os.path
import json
import codecs
import argparse
import copy
import itertools
import string
import logging

from collections import OrderedDict, namedtuple
from mako.lookup import TemplateLookup

try:
    basestring
except NameError:
    basestring = ("".__class__,)

logger = logging.getLogger("metajava")


jsonf = namedtuple("jsonf", "filename json")

arity_regex = re.compile(r'ReqlFunction(\d+)')


def jsonfile(filename):
    return jsonf(filename,
                 json.load(open(filename), object_pairs_hook=OrderedDict))


def parse_args():
    '''Handle command line arguments etc'''
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command",
                        choices=[
                            'update-terminfo',
                            'generate-java-terminfo',
                            'generate-java-classes',
                        ])
    parser.add_argument("--term-info", type=jsonfile)
    parser.add_argument("--proto-json", type=jsonfile)
    parser.add_argument("--global-info", type=jsonfile)
    parser.add_argument("--java-term-info", type=jsonfile)
    parser.add_argument("--template-dir")
    parser.add_argument("--package-dir")
    parser.add_argument("--output-file")
    parser.add_argument(
        "--debug",
        help="print debug level output",
        dest='debug',
        action='store_true'
    )
    parser.add_argument(
        "--info",
        help="print info level output",
        dest='info',
        action='store_true'
    )
    parser.set_defaults(debug=False, info=False)
    return parser.parse_args()


def main():
    logging.basicConfig(format="[%(name)s] %(message)s")
    args = parse_args()
    if args.debug:
        logging.root.setLevel(logging.DEBUG)
    elif args.info:
        logging.root.setLevel(logging.INFO)

    if args.command == 'update-term-info':
        update_term_info(args.proto_json, args.term_info)
    elif args.command == 'generate-java-terminfo':
        java_term_info(args.term_info.json, args.output_file)
    elif args.command == 'generate-java-classes':
        JavaRenderer(
            global_info=args.global_info,
            proto=args.proto_json.json,
            java_term_info=args.java_term_info,
            template_dir=args.template_dir,
            package_dir=args.package_dir,
        ).render_all()


class update_term_info(object):
    '''Updates term_info.json if new terms were discovered in
    proto_basic.json'''
    def __init__(self, proto, term_meta):
        new_json = self.diff_proto_keys(proto.json, term_meta.json)
        if new_json != term_meta.json:
            self.write_term_metadata(term_meta.filename, new_json)
            # term_meta is a tuple, so we re-use the same dict to avoid
            # mutation
            term_meta.json.clear()
            term_meta.json.update(new_json)
        else:
            os.utime(term_meta.filename, None)

    @staticmethod
    def diff_proto_keys(proto, term_meta):
        '''Finds any new keys in the protobuf file and adds dummy entries
        for them in the term_info.json dictionary'''
        set_meta = set(term_meta.keys())
        proto_items = proto['Term']['TermType']
        diff = [x for x in proto_items.keys()
                if x not in set_meta]
        new = term_meta.copy()
        for key in diff:
            logger.info("Got new term %s with id %s", key, proto_items[key])
            new[key] = OrderedDict([
                ('include_in', ['T_EXPR']),
                ('id', proto_items[key])
            ])
        # Sync up protocol ids (these should never change, but it's best
        # that it's automated since they'd otherwise be specified in two
        # places that would need to be kept in sync.
        for key, val in new.iteritems():
            if val['id'] != proto_items[key]:
                logger.warning("Warning: %s changed from %s to %s",
                    key, val['id'], proto_items[key])
                val['id'] = proto_items[key]
        return new

    def write_term_metadata(self, output_filename, new_json):
        with open(output_filename, 'w') as outfile:
            json.dump(new_json, outfile, indent=2)


class java_term_info(object):
    '''Modifies and writes out a term info file that has helpful metadata
    for rendering java classes'''

    # Java reserved keywords. If we add a term that collides with one
    # of theses, the method names will have a trailing _ added
    JAVA_KEYWORDS = {
            'abstract', 'continue', 'for', 'new', 'switch', 'assert',
            'default', 'goto', 'package', 'synchronized', 'boolean', 'do',
            'if', 'private', 'this', 'break', 'double', 'implements',
            'protected', 'throw', 'byte', 'else', 'import', 'public',
            'throws', 'case', 'enum', 'instanceof', 'return', 'transient',
            'catch', 'extends', 'int', 'short', 'try', 'char', 'final',
            'interface', 'static', 'void', 'class', 'finally', 'long',
            'strictfp', 'volatile', 'const', 'float', 'native', 'super',
            'while'
        }

    # Methods defined on Object that we don't want to inadvertantly override
    OBJECT_METHODS = {
        'clone', 'equals', 'finalize', 'hashCode', 'getClass',
        'notify', 'notifyAll', 'wait', 'toString'
    }

    # Terms we don't want to create
    TERM_BLACKLIST = {
        'IMPLICIT_VAR'  # Java 8 lambda syntax is nice, so no need for row
    }

    # Special aliases for the java driver only
    FORCED_CLASS_RENAMES = {
        'OBJECT': 'ReqlObject',  # Avoids Object collision which is annoying
    }

    # Renames for methods
    METHOD_ALIASES = {
        'GET_FIELD': 'g'  # getField is too long for such a common operation
    }

    # Aliases that should never be emitted as method namespace
    ALIAS_BLACKLIST = {
        'funcall', 'javascript'
    }

    # Manual override of superclasses for terms. Default is ReqlExpr
    SUPERCLASSES = {
        'DB': 'ReqlAst'
    }

    # Translation from T_ style types in term_info to Java classnames
    INCLUDE_IN_CLASSNAMES = {
        'T_DB': 'Db',
        'T_TOP_LEVEL': 'TopLevel',
        'T_EXPR': 'ReqlExpr',
        'T_TABLE': 'Table',
    }

    # How many times to expand when manually expanding 'T_FUNCX' arguments
    FUNCX_EXPAND = 3

    # How many times to expand manual * arguments
    STAR_EXPAND = 4

    def __init__(self, term_info, output_filename):
        self.java_terminfo = copy.deepcopy(term_info)
        self.output_filename = output_filename

        self.modify_term_meta()
        self.write_output()

    def modify_term_meta(self):
        for term, info in list(self.java_terminfo.items()):
            self.delete_if_blacklisted(term, info)
            self.add_methodname(term, info)
            self.add_classname(term, info)
            self.add_superclass(term, info)
            self.translate_include_in(info)
            info['signatures'] = self.reify_signatures(term,
                info.get('signatures', []))

    @classmethod
    def reify_signatures(cls, term, signatures):
        '''This takes the general signatures from terminfo.json and
        turns them into signatures that can actually be created in the
        Java.'''
        reified_sigs = sorted({
            x for sig in signatures
            for x in cls.reify_signature(term, sig)
        })
        return [cls.elaborate_signature(sig) for sig in reified_sigs]

    @staticmethod
    def elaborate_signature(sig):
        '''This expands a list of arguments that have already been
        converted to Java classnames by reify_signature. This is
        pre-computing a bunch of data that it's convenient to have
        for the templates

        Example:
        ["Object", "Object", "Object...", "ReqlFunction2"]
        becomes:
        {"args": [{"type": "Object", "var": "expr"},
                  {"type": "Object", "var": "exprA"},
                  {"type": "Object...", "var": "exprs"},
                  {"type": "ReqlFunction2", "var": "func2"}],
         "first_arg": "ReqlExpr"
        }
        '''
        def num2str(num):
            if num == 0:
                return ''
            if num > 26:
                raise RuntimeError("too many variables in this signature")
            return string.ascii_uppercase[num - 1]

        args = []
        suffix_counts = {}
        first_arg = None
        for arg in sig:
            if not first_arg:
                # If the first argument is Db, we shouldn't output
                # that signature for the Table class etc, since the
                # first argument is implicitly `this` for all methods
                # except the ones defined on TopLevel.

                # The bit about `ReqlExpr` is because we need to
                # accept arguments like Booleans and Numbers that will
                # later be converted to ReqlExpr, so the signature
                # argument type has to be `Object`. But when deciding
                # whether to output the methods, we need to match
                # against the classname. So if the first argument is
                # Object, we want it to be output as a method on the
                # `ReqlExpr` class.
                first_arg = 'ReqlExpr' if arg.startswith('Object') else arg
            if arg == 'Object...':
                varname = 'exprs'
            else:
                suffix = num2str(suffix_counts.setdefault(arg, 0))
                suffix_counts[arg] += 1
                arity_match = arity_regex.match(arg)
                if arity_match:
                    arity = arity_match.groups()[0]
                    varname = 'func' + arity + suffix
                elif arg == 'Object':
                    varname = 'expr' + suffix
                elif arg == 'Javascript':
                    varname = 'js' + suffix
                else:
                    varname = arg.lower() + suffix
            args.append({"type": arg, "var": varname})

        return {
            'args': args,
            'first_arg': first_arg,
        }

    @classmethod
    def reify_signature(cls, term, signature):
        def translate(arg):
            if isinstance(arg, basestring):
                return {
                    'T_DB': 'Db',
                    'T_EXPR': 'Object',
                    'T_TABLE': 'Table',
                    'T_FUNC0': 'ReqlFunction0',
                    'T_FUNC1': 'ReqlFunction1',
                    'T_FUNC2': 'ReqlFunction2',
                    'T_JS': 'Javascript',
                }[arg]
            elif isinstance(arg, list):
                return [translate(a) for a in arg]
            else:
                raise RuntimeError(
                    "Got something unexpected in signature %s", type(arg))

        def expand_alternation(formal_args, index):
            '''This does a manual expansion of '*' args when the type of the
            argument before the star is an alternation. So with
            signatures like:

            ['A', ['B', 'C'], '*']
            will expand to:
            [['A'],
             ['A', 'B'],
             ['A', 'C'],
             ['A', 'B', 'B'],
             ['A', 'B', 'C'],
             ['A', 'C', 'B'],
             ['A', 'C', 'C']]

            Why do this? Well, if we just output Object... as the type
            of the argument, then if one of the args needs to be a
            function, the user will need to specify the type of the
            lambda explicitly, since Java won't be able to infer the
            function type (imagine 'C' above being 'ReqlFunction1').
            This doesn't allow truly arbitrary numbers of arguments as
            reql does in principle, but it should be ok for practical
            purposes and allow convenient type inference.
            '''
            before_prev = tuple(formal_args[:-1])
            prev = formal_args[i-1]
            result = set()
            for reps in range(0, cls.STAR_EXPAND+1):
                result.update(before_prev + p
                              for p in itertools.product(prev, repeat=reps))
            return list(sorted(result))

        def expand_funcx(formal_args):
            '''This manually expands T_FUNCX arguments.
            Example:
            ['A', 'B', '*', 'T_FUNCX']
            will expand to:
            [['A', 'ReqlFunction1'],
             ['A', 'B', 'ReqlFunction2'],
             ['A', 'B', 'B', 'ReqlFunction3']]

            This is because Java needs a concrete number of arguments
            in the lambdas, so we can't make the signature for T_FUNCX
            apply(Object...) or anything like that. Like alternation
            above, this limits how many arguments can be passed to
            `do` and `map`, but in practice you usually don't need an
            arbitrary number of arguments, a few suffice.
            '''
            prev = formal_args[-1].rstrip('...')
            before_prev = tuple(formal_args[:-1])
            base_arity = len(before_prev)
            result = set()
            for arity in range(base_arity, cls.FUNCX_EXPAND+base_arity+1):
                prev_args = before_prev + (prev,) * (arity - base_arity)
                result.add(prev_args + ('ReqlFunction' + str(arity),))
            return list(sorted(result))

        def swap_for_js(arg):
            if arg.startswith('ReqlFunction'):
                return 'Javascript'
            else:
                return arg

        formal_args = []
        expanded = False

        # This is a little hack just for r.do. It formally accepts (in
        # the wire format) the function first, and the arguments
        # last. But all the drivers conventionally accept the function
        # as the last argument since that looks the best when it is
        # chained.
        if term == 'FUNCALL' and 'T_FUNCX' in signature:
            signature = signature[1:] + [signature[0]]

        for i, arg in enumerate(signature):
            if arg == '*':
                prev = formal_args[i-1]
                if isinstance(prev, basestring):
                    # Use normal Java varargs if not alternation
                    formal_args[i-1] += '...'
                elif isinstance(prev, list):
                    expanded = True
                    formal_args = expand_alternation(formal_args, i)
                else:
                    raise RuntimeError(
                        "cant expand signature: {}".format(signature))
            elif arg == 'T_FUNCX':
                expanded = True
                if i == 0:
                    raise RuntimeError(
                        "T_FUNCX can't be the first argument:{}"
                        .format(signature))
                elif not formal_args[-1].endswith('...'):
                    raise RuntimeError(
                        "T_FUNCX must follow a variable argument: {}"
                        .format(signature))
                elif not i+1 == len(signature):
                    raise RuntimeError(
                        "T_FUNCX must be the last argument:{}"
                        .format(signature))
                else:
                    expanded = True
                    formal_args = expand_funcx(formal_args)
            else:
                formal_args.append(translate(arg))

        subtotal = [formal_args] if not expanded else formal_args
        final_args = []
        for sig in subtotal:
            final_args.append(tuple(sig))
            if any(arg.startswith('ReqlFunction') for arg in sig):
                # Javascript term can stand in for function arguments
                final_args.append(tuple(swap_for_js(a) for a in sig))
        return final_args

    def write_output(self):
        with open(self.output_filename, 'w') as outputfile:
            json.dump(self.java_terminfo, outputfile, indent=2)

    def delete_if_blacklisted(self, term, info):
        if term in self.TERM_BLACKLIST:
            del self.java_terminfo[term]

    def add_methodname(self, term, info):
        methodnames = self.nice_names(term, info)

        def filter_methodname(methodname):
            if methodname in self.JAVA_KEYWORDS:
                methodname += '_'
            elif methodname in self.OBJECT_METHODS:
                methodname += '_'
            if methodname in self.ALIAS_BLACKLIST:
                return None
            else:
                return methodname

        info['methodnames'] = [filter_methodname(n)
                               for n in methodnames
                               if filter_methodname(n) is not None]

    def add_classname(self, term, info):
        info['classname'] = self.FORCED_CLASS_RENAMES.get(term, camel(term))

    def add_superclass(self, term, info):
        '''This is sort of hardcoded. It could be obtained from
        ast_type_hierarchy in global_info.json if it became an
        issue'''
        info['superclass'] = self.SUPERCLASSES.get(term, 'ReqlExpr')

    def translate_include_in(self, info):
        info['include_in'] = [self.INCLUDE_IN_CLASSNAMES[t]
                              for t in info.get('include_in')]

    @staticmethod
    def nice_names(term, info):
        '''Whether the nice name for a term is in a given set'''
        result = [dromedary(term)]
        if 'alias' in info:
            result.append(dromedary(info['alias']))
        if term in java_term_info.METHOD_ALIASES:
            result.append(java_term_info.METHOD_ALIASES[term])
        return result


def camel(varname):
    'CamelCase'
    if re.match(r'[A-Z][A-Z0-9_]*$|[a-z][a-z0-9_]*$', varname):
        # if snake-case (upper or lower) camelize it
        suffix = "_" if varname.endswith('_') else ""
        return ''.join(x.title() for x in varname.split('_')) + suffix
    else:
        # if already mixed case, just capitalize the first letter
        return varname[0].upper() + varname[1:]


def dromedary(varname):
    'dromedaryCase'
    if re.match(r'[A-Z][A-Z0-9_]*$|[a-z][a-z0-9_]*$', varname):
        chunks = varname.split('_')
        suffix = "_" if varname.endswith('_') else ""
        return (chunks[0].lower() +
                ''.join(x.title() for x in chunks[1:]) +
                suffix)
    else:
        return varname[0].lower() + varname[1:]


class JavaRenderer(object):
    '''Uses java_term_info.json and global_info.json to render all Java
    AST and interface files'''

    def __init__(self,
                 global_info,
                 proto,
                 java_term_info,
                 template_dir,
                 package_dir):
        self.global_info = global_info
        self.proto = proto
        self.term_info = java_term_info.json
        self.template_dir = template_dir
        self.gen_dir = package_dir+'/gen'
        self.max_arity = self.get_max_arity()
        self.renderer = Renderer(
            template_dir,
            invoking_filenames=[__file__],
            source_files=[java_term_info.filename],
        )
        # Facade methods
        self.get_template_name = self.renderer.get_template_name
        self.render = self.renderer.render

    def render_all(self):
        self.render_proto_enums()
        self.render_ast_subclasses()
        self.render_toplevel()
        self.render_function_interfaces()
        self.render_exceptions()

    def render_toplevel(self):
        '''Specially renders the TopLevel class'''
        self.render(
            'TopLevel.java',
            output_dir=self.gen_dir+'/model',
            all_terms=self.term_info,
        )

    def render_function_interfaces(self):
        '''Finds the maximum arity a function will be from
        java_term_info.json, then creates an interface for each
        arity'''
        for arity in range(0, self.max_arity+1):
            self.render(
                'ReqlFunction.java',
                output_dir=self.gen_dir+'/ast',
                output_name='ReqlFunction{}.java'.format(arity),
                arity=arity,
            )

    def render_exceptions(
            self, hierarchy=None, superclass='runtime_exception'):
        '''Renders the exception hierarchy'''
        if hierarchy is None:
            hierarchy = self.global_info.json['exception_hierarchy']
        for classname, subclasses in hierarchy.items():
            self.render_exception(classname, superclass)
            self.render_exceptions(subclasses, superclass=classname)

    def render_exception(self, classname, superclass):
        '''Renders a single exception class'''
        self.render(
            'Exception.java',
            output_dir=self.gen_dir+'/exc',
            output_name=camel(classname)+'.java',
            classname=classname,
            superclass=superclass,
            dependencies=[self.global_info.filename],
        )

    def render_ast_subclasses(self):
        # Since there is no term associated with it, we specially
        # invoke render_ast_subclass to create the ReqlExpr java file
        self.render_ast_subclass(
            None, {
                "superclass": "ReqlAst",
                "classname": "ReqlExpr",
            }
        )
        # This will render an AstSubclass for each term
        for term_name, meta in self.term_info.items():
            if not meta.get('deprecated'):
                self.render_ast_subclass(term_name, meta)

    def render_ast_subclass(self, term_name, meta):
        output_name = meta['classname'] + '.java'
        template_name = self.get_template_name(
            meta['classname'], directory='ast', default='AstSubclass.java')
        self.render(
            template_name,
            output_dir=self.gen_dir+'/ast',
            output_name=output_name,
            term_name=term_name,
            classname=meta['classname'],
            superclass=meta['superclass'],
            meta=meta,
            all_terms=self.term_info,
            max_arity=self.max_arity,
            optargs=meta.get('optargs'),
        )

    def render_proto_enums(self):
        '''Render protocol enums'''
        self.render_proto_enum("Version", self.proto["VersionDummy"])
        self.render_proto_enum("Protocol", self.proto["VersionDummy"])
        self.render_proto_enum("QueryType", self.proto["Query"])
        self.render_proto_enum("ResponseType", self.proto["Response"])
        self.render_proto_enum("ResponseNote", self.proto["Response"])
        self.render_proto_enum("ErrorType", self.proto["Response"])
        self.render_proto_enum("DatumType", self.proto["Datum"])
        self.render_proto_enum("TermType", self.proto["Term"])

    def render_proto_enum(self, classname, namespace):
        mapping = namespace[classname]
        template_name = self.get_template_name(
            classname, directory='proto', default='Enum.java')
        self.render(template_name,
                    output_dir=self.gen_dir+'/proto',
                    output_name=classname+'.java',
                    classname=classname,
                    package='proto',
                    items=mapping.items(),
                    )

    def get_max_arity(self):
        '''Determines the maximum reql lambda arity that shows up in any
        signature'''
        max_arity = 0
        for info in self.term_info.values():
            for sig in info['signatures']:
                for arg in sig['args']:
                    match = arity_regex.match(arg['type'])
                    if match:
                        # Scrape out the arity from the type
                        # name. This could have been inserted into
                        # java_term_info.json, but it's a lot of
                        # clutter for redundant information.
                        arity = int(match.groups()[0])
                        max_arity = max(max_arity, arity)
        return max_arity


class EmptyTemplate(Exception):
    '''Raised inside templates if they have no reason to be rendered
    because what they're iterating over is empty'''
    pass


class Renderer(object):
    '''Manages rendering templates'''

    def __init__(self, template_dir, invoking_filenames, source_files=None):
        self.template_dir = template_dir
        self.invoking_filenames = invoking_filenames
        self.source_files = source_files or []
        self.tl = TemplateLookup(directories=[template_dir])
        self.template_context = {
            'camel': camel,  # CamelCase function
            'dromedary': dromedary,  # dromeDary case function
            'EmptyTemplate': EmptyTemplate,
        }
        self.mtime_cache = {}
        self.dependency_cache = {}

    def mtime(self, filename):
        try:
            return self.mtime_cache[filename]
        except KeyError:
            mtime = os.path.getmtime(filename)
            self.mtime_cache[filename] = mtime
            return mtime

    def render(self,
               template_name,
               output_dir,
               output_name=None,
               dependencies=None,
               **kwargs):
        if output_name is None:
            output_name = template_name

        tpl = self.tl.get_template(template_name)
        output_path = output_dir + '/' + output_name

        if self.already_rendered(tpl, output_path, dependencies):
            logger.debug(" Up to date: %s", output_path)
            return

        results = self.template_context.copy()
        results.update(kwargs)
        try:
            rendered = tpl.render(**results)
        except EmptyTemplate:
            logger.debug(" Empty template: %s", output_path)
            return
        with codecs.open(output_path, "w", "utf-8") as outfile:
            logger.info("Rendering %s", output_path)
            outfile.write(self.autogenerated_header(
                self.template_dir+'/'+template_name,
                output_path,
                self.invoking_filenames,
            ))
            outfile.write(rendered)

    def get_template_name(self, classname, directory, default):
        '''Returns the template for this class'''
        override_template = '{}/{}.java'.format(directory, classname)
        if self.tl.has_template(override_template):
            logger.debug(
                "Found an override for %s at %s", classname, override_template)
            return override_template
        else:
            return default

    def dependent_templates(self, tpl):
        '''Returns filenames for all templates that are inherited from the
        given template'''
        if tpl.filename in self.dependency_cache:
            return self.dependency_cache[tpl.filename]
        inherit_files = re.findall(r'inherit file="(.*)"', tpl.source)
        op = os.path
        dependencies = set()
        tpl_dir = op.dirname(tpl.filename)
        for parent_relpath in inherit_files:
            parent_filename = op.normpath(op.join(tpl_dir, parent_relpath))
            dependencies.add(parent_filename)
            dependencies.update(
                self.dependent_templates(
                    self.tl.get_template(
                        self.tl.filename_to_uri(parent_filename))))
        dependencies.add(tpl.filename)
        self.dependency_cache[tpl.filename] = dependencies
        return dependencies

    def already_rendered(self, tpl, output_path, dependencies):
        '''Check if rendered file is already up to date'''
        if not os.path.exists(output_path):
            logger.debug(" Rendering since %s doesn't exist", output_path)
            return False

        output_mtime = self.mtime(output_path)

        # Check if this file or the invoking file have changed
        for filename in self.invoking_filenames + self.source_files:
            if self.mtime(filename) > output_mtime:
                logger.debug(
                    " Rendering since %s has changed", filename)
                return False
        if self.mtime(__file__) > output_mtime:
            logger.debug(" Rendering since %s has changed", __file__)
            return False

        # Check if any dependent templates have changed
        for tpl in self.dependent_templates(tpl):
            if self.mtime(tpl) > output_mtime:
                logger.debug(" Rendering since %s is newer", tpl)
                return False

        # Check if any explicitly defined dependencies have changed
        dependencies = dependencies or []
        for dep in dependencies:
            if self.mtime(dep) > output_mtime:
                logger.debug(" Rendering since %s is newer", dep)
                return False
        return True

    def autogenerated_header(self, template_path, output_path, filename):
        rel_tpl = os.path.relpath(template_path, start=output_path)
        filenames = ' and '.join(os.path.basename(f)
                                 for f in self.invoking_filenames)
        return ('// Autogenerated by {}.\n'
                '// Do not edit this file directly.\n'
                '// The template for this file is located at:\n'
                '// {}\n').format(filenames, rel_tpl)


if __name__ == '__main__':
    main()

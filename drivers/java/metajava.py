#!/usr/bin/env python2
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

from collections import OrderedDict, namedtuple
from mako.lookup import TemplateLookup

MTIME = os.path.getmtime(__file__)

__all__ = []


jsonf = namedtuple("jsonf", "filename json")


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
    return parser.parse_args()


def main():
    args = parse_args()

    if args.command == 'update-term-info':
        update_term_info(args.proto_json, args.term_info)
    elif args.command == 'generate-java-terminfo':
        java_term_info(args.term_info.json, args.output_file)
    elif args.command == 'generate-java-classes':
        generate_java_classes(
            global_info=args.global_info.json,
            proto=args.proto_json.json,
            java_term_info=args.java_term_info.json,
            template_dir=args.template_dir,
            package_dir=args.package_dir,
        )
    return #maginot line of conversion

    render_ast_subclasses(java_meta)

    render_global_options(global_info['global_optargs'])
    render_exceptions(global_info['exception_hierarchy'])
    render_optarg_enums(global_info['optarg_enums'])


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
            print("Got new term", key, "with id", proto_items[key], end='')
            new[key] = OrderedDict([
                ('include_in', ['T_EXPR']),
                ('id', proto_items[key])
            ])
        # Sync up protocol ids (these should never change, but it's best
        # that it's automated since they'd otherwise be specified in two
        # places that would need to be kept in sync.
        for key, val in new.iteritems():
            if val['id'] != proto_items[key]:
                print("Warning: {} changed from {} to {}".format(
                    key, val['id'], proto_items[key]))
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

    # Terms we don't want to create
    TERM_BLACKLIST = {
        'row'  # Java 8 lambda syntax is nice, so no need for row
    }

    # Special aliases for the java driver only
    FORCED_CLASS_RENAMES = {
        'OBJECT': 'ReqlObject',  # Avoids Object collision which is annoying
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
    STAR_EXPAND = 2

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
            info['signatures'] = self.reify_signatures(
                info.get('signatures', []))

    @classmethod
    def reify_signatures(cls, signatures):
        '''This takes the general signatures from terminfo.json and
        turns them into signatures that can actually be created in the
        Java.'''
        return [x for sig in signatures for x in cls.reify_signature(sig)]

    @classmethod
    def reify_signature(cls, signature):
        def translate(arg):
            if isinstance(arg, basestring):
                return {
                    'T_DB': 'Db',
                    'T_EXPR': 'Object',
                    'T_TABLE': 'Table',
                    'T_FUNC1': 'ReqlFunction1',
                    'T_FUNC2': 'ReqlFunction2',
                }[arg]
            elif isinstance(arg, list):
                return [translate(a) for a in arg]

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
            before_prev = formal_args[:-1]
            prev = formal_args[i-1]
            result = []
            for reps in range(0, cls.STAR_EXPAND+1):
                result.extend(before_prev+list(p)
                              for p in itertools.product(prev, repeat=reps))
            return result

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
            before_prev = formal_args[:-1]
            base_arity = len(before_prev)
            result = []
            for arity in range(base_arity, cls.FUNCX_EXPAND+base_arity+1):
                result.append(
                    before_prev +  # Any existing prefix arguments
                    [prev]*(arity-base_arity) +  # repeated type
                    ['ReqlFunction'+str(arity)])  # function type
            return result

        formal_args = []
        expanded = False

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
        return [formal_args] if not expanded else formal_args

    def write_output(self):
        with open(self.output_filename, 'w') as outputfile:
            json.dump(self.java_terminfo, outputfile, indent=2)

    def delete_if_blacklisted(self, term, info):
        if self.nice_name(term, info) in self.TERM_BLACKLIST:
            del self.java_terminfo[term]

    def add_methodname(self, term, info):
        methodname = self.nice_name(term, info)
        if methodname in self.JAVA_KEYWORDS:
            methodname += '_'
        info['methodname'] = methodname

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
    def nice_name(term, info):
        '''Whether the nice name for a term is in a given set'''
        return info.get('alias', dromedary(term))


def camel(varname):
    'CamelCase'
    if re.match(r'[A-Z][A-Z0-9_]*$|[a-z][a-z0-9_]*$', varname):
        # if snake-case (upper or lower) camelize it
        return ''.join(x.title() for x in varname.split('_'))
    else:
        # if already mixed case, just capitalize the first letter
        return varname[0].upper() + varname[1:]


def dromedary(varname):
    'dromedaryCase'
    if re.match(r'[A-Z][A-Z0-9_]*$|[a-z][a-z0-9_]*$', varname):
        chunks = varname.split('_')
        return chunks[0].lower() + ''.join(x.title() for x in chunks[1:])
    else:
        return varname[0].lower() + varname[1:]


class generate_java_classes(object):
    '''Uses java_term_info.json and global_info.json to render all Java
    AST and interface files'''

    def __init__(self,
                 global_info,
                 proto,
                 java_term_info,
                 template_dir,
                 package_dir):
        self.tl = TemplateLookup(directories=[template_dir])
        self.global_info = global_info
        self.proto = proto
        self.term_info = java_term_info
        self.template_dir = template_dir
        self.gen_dir = package_dir+'/gen'
        self.template_context = {
            'camel': camel,  # CamelCase function
            'dromedary': dromedary,  # dromeDary case function
        }

        self.render_proto_enums()
        self.render_ast_subclasses()
        self.render_toplevel()
        self.render_exceptions()
        self.render_function_interfaces()

    def render_toplevel(self):
        print("WARNING: didn't render toplevel for real")

    def render_exceptions(self):
        print("WARNING: didn't render exceptions for real")

    def render_function_interfaces(self):
        print("WARNING: didn't render function interfaces. not really.")

    def render_ast_subclasses(self):
        for term_name, meta in self.term_info.items():
            if not meta.get('deprecated'):
                self.render_ast_subclass(term_name, meta)

    def render_ast_subclass(self, term_name, meta):
        output_name = meta['classname'] + '.java'
        template_name = self.get_template_name(
            meta['classname'], directory='gen', default='AstSubclass.java')
        self.render(
            template_name,
            output_dir=self.gen_dir+'/ast',
            output_name=output_name,
            term_name=term_name,
            classname=meta['classname'],
            superclass=meta['superclass'],
            meta=meta,
            all_terms=self.term_info,
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

    def render(self, template_name, output_dir, output_name=None, **kwargs):
        if output_name is None:
            output_name = template_name

        tpl = self.tl.get_template(template_name)
        output_path = output_dir + '/' + output_name

        if self.already_rendered(tpl, output_path):
            return

        with codecs.open(output_path, "w", "utf-8") as outfile:
            print("Rendering", output_path)
            results = self.template_context.copy()
            results.update(kwargs)
            rendered = tpl.render(**results)
            outfile.write(autogenerated_header(
                self.template_dir+'/'+template_name,
                output_path,
            ))
            outfile.write(rendered)

    def get_template_name(self, classname, directory, default):
        '''Returns the template for this class'''
        override_template = '{}/{}.java'.format(directory, classname)
        if self.tl.has_template(override_template):
            print("    Found an override for {} at {}".format(
                classname, override_template))
            return override_template
        else:
            return default

    def dependent_templates(self, tpl):
        '''Returns filenames for all templates that are inherited from the
        given template'''
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
        return dependencies.union([tpl.filename])

    def already_rendered(self, tpl, output_path):
        '''Check if rendered file is already up to date'''
        tpl_mtime = max([os.path.getmtime(t)
                         for t in self.dependent_templates(tpl)])
        output_exists = os.path.exists(output_path)
        return (output_exists and
                tpl_mtime < os.path.getmtime(output_path) and
                MTIME <= os.path.getmtime(output_path))


def java_repr(obj):
    if isinstance(obj, bool):
        return str(obj).lower()
    elif isinstance(obj, basestring):
        return '"{}"'.format(obj)
    elif isinstance(obj, (int, float)):
        return str(obj)
    elif obj is None:
        return 'null'
    else:
        return repr(obj)


def autogenerated_header(template_path, output_path):
    rel_tpl = os.path.relpath(template_path, start=output_path)

    return ('// Autogenerated by {}.\n'
            '// Do not edit this file directly.\n'
            '// The template for this file is located at:\n'
            '// {}\n').format(os.path.basename(__file__), rel_tpl)


def sub_to_super_mapping(type_hierarchy, superclass='ReqlAst'):
    '''Get a data structure for generating the ast hierarchy'''
    mapping = {}
    for ty, subclasses in type_hierarchy:
        if ty in ('T_DATUM', 'T_FUNC'):
            # these won't be generated automatically
            continue
        mapping[ty[2:]] = superclass
        mapping.update(sub_to_super_mapping(subclasses, ty[2:]))
    return mapping


def render_exceptions(hierarchy, superclass='runtime_exception'):
    '''Renders the exception hierarchy'''
    for classname, subclasses in hierarchy.items():
        render_exception(classname, superclass)
        render_exceptions(subclasses, superclass=classname)


def render_exception(classname, superclass):
    '''Renders a single exception class'''
    render(
        'Exception.java',
        output_dir=PACKAGE_DIR,
        output_name=camel(classname)+'.java',
        classname=classname,
        superclass=superclass,
        camel=camel,
    )


def get_proto_file(proto_json):
    '''Loads proto file as json'''
    return json.loads(open(proto_json))


def render_optarg_enums(enum_listing):
    for key, values in enum_listing.items():
        classname = camel(key.lstrip('E_'))
        enum_name_mapping = {val: '"'+val+'"' for val in values}
        template_name = get_template_name(
            classname, directory='model', default='Enum.java')
        render(template_name,
               output_dir=MODEL_DIR,
               output_name=classname+'.java',
               classname=classname,
               package="model",
               items=enum_name_mapping.items())




if __name__ == '__main__':
    main()

#!/usr/bin/python
# Copyright 2006 Rene Rivera
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

'''
Processing of Doxygen generated XML.
'''

import os
import os.path
import sys
import time
import string
import getopt
import glob
import re
import xml.dom.minidom

    
def usage():
    print '''
Usage:
    %s options

Options:
    --xmldir        Directory with the Doxygen xml result files.
    --output        Write the output BoostBook to the given location.
    --id            The ID of the top level BoostBook section.
    --title         The title of the top level BoostBook section.
    --enable-index  Generate additional index sections for classes and
                    types.
''' % ( sys.argv[0] )


def get_args( argv = sys.argv[1:] ):
    spec = [
        'xmldir=',
        'output=',
        'id=',
        'title=',
        'enable-index',
        'help' ]
    options = {
        '--xmldir' : 'xml',
        '--output' : None,
        '--id' : 'dox',
        '--title' : 'Doxygen'
        }
    ( option_pairs, other ) = getopt.getopt( argv, '', spec )
    map( lambda x: options.__setitem__( x[0], x[1] ), option_pairs )
    
    if options.has_key( '--help' ):
        usage()
        sys.exit(1)
    
    return {
        'xmldir' : options['--xmldir'],
        'output' : options['--output'],
        'id' : options['--id'],
        'title' : options['--title'],
        'index' : options.has_key('--enable-index')
        }

def if_attribute(node, attribute, true_value, false_value=None):
    if node.getAttribute(attribute) == 'yes':
        return true_value
    else:
        return false_value

class Doxygen2BoostBook:
    
    def __init__( self, **kwargs ):
        ##
        self.args = kwargs
        self.args.setdefault('id','')
        self.args.setdefault('title','')
        self.args.setdefault('last_revision', time.asctime())
        self.args.setdefault('index', False)
        self.id = '%(id)s.reference' % self.args
        self.args['id'] = self.id
        #~ This is our template BoostBook document we insert the generated content into.
        self.boostbook = xml.dom.minidom.parseString('''<?xml version="1.0" encoding="UTF-8"?>
<section id="%(id)s" name="%(title)s" last-revision="%(last_revision)s">
    <title>%(title)s</title>
    <library-reference id="%(id)s.headers">
        <title>Headers</title>
    </library-reference>
    <index id="%(id)s.classes">
        <title>Classes</title>
    </index>
    <index id="%(id)s.index">
        <title>Index</title>
    </index>
</section>
''' % self.args )
        self.section = {
            'headers' : self._getChild('library-reference',id='%(id)s.headers' % self.args),
            'classes' : self._getChild('index',id='%(id)s.classes' % self.args),
            'index' : self._getChild('index',id='%(id)s.index' % self.args)
            }
        #~ Remove the index sections if we aren't generating it.
        if not self.args['index']:
            self.section['classes'].parentNode.removeChild(self.section['classes'])
            self.section['classes'].unlink()
            del self.section['classes']
            self.section['index'].parentNode.removeChild(self.section['index'])
            self.section['index'].unlink()
            del self.section['index']
        #~ The symbols, per Doxygen notion, that we translated.
        self.symbols = {}
        #~ Map of Doxygen IDs and BoostBook IDs, so we can translate as needed.
        self.idmap = {}
        #~ Marks generation, to prevent redoing it.
        self.generated = False
    
    #~ Add an Doxygen generated XML document to the content we are translating.
    def addDox( self, document ):
        self._translateNode(document.documentElement)
    
    #~ Turns the internal XML tree into an output UTF-8 string.
    def tostring( self ):
        self._generate()
        #~ return self.boostbook.toprettyxml('  ')
        return self.boostbook.toxml('utf-8')
    
    #~ Does post-processing on the partial generated content to generate additional info
    #~ now that we have the complete source documents.
    def _generate( self ):
        if not self.generated:
            self.generated = True
            symbols = self.symbols.keys()
            symbols.sort()
            #~ Populate the header section.
            for symbol in symbols:
                if self.symbols[symbol]['kind'] in ('header'):
                    self.section['headers'].appendChild(self.symbols[symbol]['dom'])
            for symbol in symbols:
                if self.symbols[symbol]['kind'] not in ('namespace', 'header'):
                    container = self._resolveContainer(self.symbols[symbol],
                        self.symbols[self.symbols[symbol]['header']]['dom'])
                    if container.nodeName != 'namespace':
                        ## The current BoostBook to Docbook translation doesn't
                        ## respect, nor assign, IDs to inner types of any kind.
                        ## So nuke the ID entry so as not create bogus links.
                        del self.idmap[self.symbols[symbol]['id']]
                    container.appendChild(self.symbols[symbol]['dom'])
            self._rewriteIDs(self.boostbook.documentElement)
    
    #~ Rewrite the various IDs from Doxygen references to the newly created
    #~ BoostBook references.
    def _rewriteIDs( self, node ):
        if node.nodeName in ('link'):
            if (self.idmap.has_key(node.getAttribute('linkend'))):
                #~ A link, and we have someplace to repoint it at.
                node.setAttribute('linkend',self.idmap[node.getAttribute('linkend')])
            else:
                #~ A link, but we don't have a generated target for it.
                node.removeAttribute('linkend')
        elif hasattr(node,'hasAttribute') and node.hasAttribute('id') and self.idmap.has_key(node.getAttribute('id')):
            #~ Simple ID, and we have a translation.
            node.setAttribute('id',self.idmap[node.getAttribute('id')])
        #~ Recurse, and iterate, depth-first traversal which turns out to be
        #~ left-to-right and top-to-bottom for the document.
        if node.firstChild:
            self._rewriteIDs(node.firstChild)
        if node.nextSibling:
            self._rewriteIDs(node.nextSibling)
    
    def _resolveContainer( self, cpp, root ):
        container = root
        for ns in cpp['namespace']:
            node = self._getChild('namespace',name=ns,root=container)
            if not node:
                node = container.appendChild(
                    self._createNode('namespace',name=ns))
            container = node
        for inner in cpp['name'].split('::'):
            node = self._getChild(name=inner,root=container)
            if not node:
                break
            container = node
        return container
    
    def _setID( self, id, name ):
        self.idmap[id] = name.replace('::','.').replace('/','.')
        #~ print '--| setID:',id,'::',self.idmap[id]
    
    #~ Translate a given node within a given context.
    #~ The translation dispatches to a local method of the form
    #~ "_translate[_context0,...,_contextN]", and the keyword args are
    #~ passed along. If there is no translation handling method we
    #~ return None.
    def _translateNode( self, *context, **kwargs ):
        node = None
        names = [ ]
        for c in context:
            if c:
                if not isinstance(c,xml.dom.Node):
                    suffix = '_'+c.replace('-','_')
                else:
                    suffix = '_'+c.nodeName.replace('-','_')
                    node = c
                names.append('_translate')
                names = map(lambda x: x+suffix,names)
        if node:
            for name in names:
                if hasattr(self,name):
                    return getattr(self,name)(node,**kwargs)
        return None
    
    #~ Translates the children of the given parent node, appending the results
    #~ to the indicated target. For nodes not translated by the translation method
    #~ it copies the child over and recurses on that child to translate any
    #~ possible interior nodes. Hence this will translate the entire subtree.
    def _translateChildren( self, parent, **kwargs ):
        target = kwargs['target']
        for n in parent.childNodes:
            child = self._translateNode(n,target=target)
            if child:
                target.appendChild(child)
            else:
                child = n.cloneNode(False)
                if hasattr(child,'data'):
                    child.data = re.sub(r'\s+',' ',child.data)
                target.appendChild(child)
                self._translateChildren(n,target=child)
    
    #~ Translate the given node as a description, into the description subnode
    #~ of the target. If no description subnode is present in the target it
    #~ is created.
    def _translateDescription( self, node, target=None, tag='description', **kwargs ):
        description = self._getChild(tag,root=target)
        if not description:
            description = target.appendChild(self._createNode(tag))
        self._translateChildren(node,target=description)
        return description
    
    #~ Top level translation of: <doxygen ...>...</doxygen>,
    #~ translates the children.
    def _translate_doxygen( self, node ):
        #~ print '_translate_doxygen:', node.nodeName
        result = []
        for n in node.childNodes:
            newNode = self._translateNode(n)
            if newNode:
                result.append(newNode)
        return result
    
    #~ Top level translation of:
    #~ <doxygenindex ...>
    #~   <compound ...>
    #~     <member ...>
    #~       <name>...</name>
    #~     </member>
    #~     ...
    #~   </compound>
    #~   ...
    #~ </doxygenindex>
    #~ builds the class and symbol sections, if requested.
    def _translate_doxygenindex( self, node ):
        #~ print '_translate_doxygenindex:', node.nodeName
        if self.args['index']:
            entries = []
            classes = []
            #~ Accumulate all the index entries we care about.
            for n in node.childNodes:
                if n.nodeName == 'compound':
                    if n.getAttribute('kind') not in ('file','dir','define'):
                        cpp = self._cppName(self._getChildData('name',root=n))
                        entry = {
                            'name' : cpp['name'],
                            'compoundname' : cpp['compoundname'],
                            'id' : n.getAttribute('refid')
                            }
                        if n.getAttribute('kind') in ('class','struct'):
                            classes.append(entry)
                        entries.append(entry)
                        for m in n.childNodes:
                            if m.nodeName == 'member':
                                cpp = self._cppName(self._getChildData('name',root=m))
                                entry = {
                                    'name' : cpp['name'],
                                    'compoundname' : cpp['compoundname'],
                                    'id' : n.getAttribute('refid')
                                    }
                                if hasattr(m,'getAttribute') and m.getAttribute('kind') in ('class','struct'):
                                    classes.append(entry)
                                entries.append(entry)
            #~ Put them in a sensible order.
            entries.sort(lambda x,y: cmp(x['name'].lower(),y['name'].lower()))
            classes.sort(lambda x,y: cmp(x['name'].lower(),y['name'].lower()))
            #~ And generate the BoostBook for them.
            self._translate_index_(entries,target=self.section['index'])
            self._translate_index_(classes,target=self.section['classes'])
        return None
    
    #~ Translate a set of index entries in the BoostBook output. The output
    #~ is grouped into groups of the first letter of the entry names.
    def _translate_index_(self, entries, target=None, **kwargs ):
        i = 0
        targetID = target.getAttribute('id')
        while i < len(entries):
            dividerKey = entries[i]['name'][0].upper()
            divider = target.appendChild(self._createNode('indexdiv',id=targetID+'.'+dividerKey))
            divider.appendChild(self._createText('title',dividerKey))
            while i < len(entries) and dividerKey == entries[i]['name'][0].upper():
                iename = entries[i]['name']
                ie = divider.appendChild(self._createNode('indexentry'))
                ie = ie.appendChild(self._createText('primaryie',iename))
                while i < len(entries) and entries[i]['name'] == iename:
                    ie.appendChild(self.boostbook.createTextNode(' ('))
                    ie.appendChild(self._createText(
                        'link',entries[i]['compoundname'],linkend=entries[i]['id']))
                    ie.appendChild(self.boostbook.createTextNode(')'))
                    i += 1
    
    #~ Translate a <compounddef ...>...</compounddef>,
    #~ by retranslating with the "kind" of compounddef.
    def _translate_compounddef( self, node, target=None, **kwargs ):
        return self._translateNode(node,node.getAttribute('kind'))
    
    #~ Translate a <compounddef kind="namespace"...>...</compounddef>. For
    #~ namespaces we just collect the information for later use as there is no
    #~ currently namespaces are not included in the BoostBook format. In the future
    #~ it might be good to generate a namespace index.
    def _translate_compounddef_namespace( self, node, target=None, **kwargs ):
        namespace = {
            'id' : node.getAttribute('id'),
            'kind' : 'namespace',
            'name' : self._getChildData('compoundname',root=node),
            'brief' : self._getChildData('briefdescription',root=node),
            'detailed' : self._getChildData('detaileddescription',root=node),
            'parsed' : False
            }
        if self.symbols.has_key(namespace['name']):
            if not self.symbols[namespace['name']]['parsed']:
                self.symbols[namespace['name']]['parsed'] = True
                #~ for n in node.childNodes:
                    #~ if hasattr(n,'getAttribute'):
                        #~ self._translateNode(n,n.getAttribute('kind'),target=target,**kwargs)
        else:
            self.symbols[namespace['name']] = namespace
            #~ self._setID(namespace['id'],namespace['name'])
        return None
    
    #~ Translate a <compounddef kind="class"...>...</compounddef>, which
    #~ forwards to the kind=struct as they are the same.
    def _translate_compounddef_class( self, node, target=None, **kwargs ):
        return self._translate_compounddef_struct(node,tag='class',target=target,**kwargs)
    
    #~ Translate a <compounddef kind="struct"...>...</compounddef> into:
    #~ <header id="?" name="?">
    #~   <struct name="?">
    #~     ...
    #~   </struct>
    #~ </header>
    def _translate_compounddef_struct( self, node, tag='struct', target=None, **kwargs ):
        result = None
        includes = self._getChild('includes',root=node)
        if includes:
            ## Add the header into the output table.
            self._translate_compounddef_includes_(includes,includes,**kwargs)
            ## Compounds are the declared symbols, classes, types, etc.
            ## We add them to the symbol table, along with the partial DOM for them
            ## so that they can be organized into the output later.
            compoundname = self._getChildData('compoundname',root=node)
            compoundname = self._cppName(compoundname)
            self._setID(node.getAttribute('id'),compoundname['compoundname'])
            struct = self._createNode(tag,name=compoundname['name'].split('::')[-1])
            self.symbols[compoundname['compoundname']] = {
                'header' : includes.firstChild.data,
                'namespace' : compoundname['namespace'],
                'id' : node.getAttribute('id'),
                'kind' : tag,
                'name' : compoundname['name'],
                'dom' : struct
                }
            ## Add the children which will be the members of the struct.
            for n in node.childNodes:
                self._translateNode(n,target=struct,scope=compoundname['compoundname'])
            result = struct
        return result
    
    #~ Translate a <compounddef ...><includes ...>...</includes></compounddef>,
    def _translate_compounddef_includes_( self, node, target=None, **kwargs ):
        name = node.firstChild.data
        if not self.symbols.has_key(name):
            self._setID(node.getAttribute('refid'),name)
            self.symbols[name] = {
                'kind' : 'header',
                'id' : node.getAttribute('refid'),
                'dom' : self._createNode('header',
                    id=node.getAttribute('refid'),
                    name=name)
                }
        return None
    
    #~ Translate a <basecompoundref...>...</basecompoundref> into:
    #~ <inherit access="?">
    #~   ...
    #~ </inherit>
    def _translate_basecompoundref( self, ref, target=None, **kwargs ):
        inherit = target.appendChild(self._createNode('inherit',
            access=ref.getAttribute('prot')))
        self._translateChildren(ref,target=inherit)
        return
    
    #~ Translate:
    #~   <templateparamlist>
    #~     <param>
    #~       <type>...</type>
    #~       <declname>...</declname>
    #~       <defname>...</defname>
    #~       <defval>...</defval>
    #~     </param>
    #~     ...
    #~   </templateparamlist>
    #~ Into:
    #~   <template>
    #~     <template-type-parameter name="?" />
    #~     <template-nontype-parameter name="?">
    #~       <type>?</type>
    #~       <default>?</default>
    #~     </template-nontype-parameter>
    #~   </template>
    def _translate_templateparamlist( self, templateparamlist, target=None, **kwargs ):
        template = target.appendChild(self._createNode('template'))
        for param in templateparamlist.childNodes:
            if param.nodeName == 'param':
                type = self._getChildData('type',root=param)
                defval = self._getChild('defval',root=param)
                paramKind = None
                if type in ('class','typename'):
                    paramKind = 'template-type-parameter'
                else:
                    paramKind = 'template-nontype-parameter'
                templateParam = template.appendChild(
                    self._createNode(paramKind,
                        name=self._getChildData('declname',root=param)))
                if paramKind == 'template-nontype-parameter':
                    template_type = templateParam.appendChild(self._createNode('type'))
                    self._translate_type(
                        self._getChild('type',root=param),target=template_type)
                if defval:
                    value = self._getChildData('ref',root=defval.firstChild)
                    if not value:
                        value = self._getData(defval)
                    templateParam.appendChild(self._createText('default',value))
        return template
    
    #~ Translate:
    #~   <briefdescription>...</briefdescription>
    #~ Into:
    #~   <purpose>...</purpose>
    def _translate_briefdescription( self, brief, target=None, **kwargs ):
        self._translateDescription(brief,target=target,**kwargs)
        return self._translateDescription(brief,target=target,tag='purpose',**kwargs)
    
    #~ Translate:
    #~   <detaileddescription>...</detaileddescription>
    #~ Into:
    #~   <description>...</description>
    def _translate_detaileddescription( self, detailed, target=None, **kwargs ):
        return self._translateDescription(detailed,target=target,**kwargs)
    
    #~ Translate:
    #~   <sectiondef kind="?">...</sectiondef>
    #~ With kind specific translation.
    def _translate_sectiondef( self, sectiondef, target=None, **kwargs ):
        self._translateNode(sectiondef,sectiondef.getAttribute('kind'),target=target,**kwargs)
    
    #~ Translate non-function sections.
    def _translate_sectiondef_x_( self, sectiondef, target=None, **kwargs ):
        for n in sectiondef.childNodes:
            if hasattr(n,'getAttribute'):
                self._translateNode(n,n.getAttribute('kind'),target=target,**kwargs)
        return None
    
    #~ Translate:
    #~   <sectiondef kind="public-type">...</sectiondef>
    def _translate_sectiondef_public_type( self, sectiondef, target=None, **kwargs ):
        return self._translate_sectiondef_x_(sectiondef,target=target,**kwargs)
    
    #~ Translate:
    #~   <sectiondef kind="public-sttrib">...</sectiondef>
    def _translate_sectiondef_public_attrib( self, sectiondef, target=None, **kwargs):
        return self._translate_sectiondef_x_(sectiondef,target=target,**kwargs)
    
    #~ Translate:
    #~   <sectiondef kind="?-func">...</sectiondef>
    #~ All the various function group translations end up here for which
    #~ they are translated into:
    #~   <method-group name="?">
    #~   ...
    #~   </method-group>
    def _translate_sectiondef_func_( self, sectiondef, name='functions', target=None, **kwargs ):
        members = target.appendChild(self._createNode('method-group',name=name))
        for n in sectiondef.childNodes:
            if hasattr(n,'getAttribute'):
                self._translateNode(n,n.getAttribute('kind'),target=members,**kwargs)
        return members
    
    #~ Translate:
    #~   <sectiondef kind="public-func">...</sectiondef>
    def _translate_sectiondef_public_func( self, sectiondef, target=None, **kwargs ):
        return self._translate_sectiondef_func_(sectiondef,
            name='public member functions',target=target,**kwargs)
    
    #~ Translate:
    #~   <sectiondef kind="public-static-func">...</sectiondef>
    def _translate_sectiondef_public_static_func( self, sectiondef, target=None, **kwargs):
        return self._translate_sectiondef_func_(sectiondef,
            name='public static functions',target=target,**kwargs)
    
    #~ Translate:
    #~   <sectiondef kind="protected-func">...</sectiondef>
    def _translate_sectiondef_protected_func( self, sectiondef, target=None, **kwargs ):
        return self._translate_sectiondef_func_(sectiondef,
            name='protected member functions',target=target,**kwargs)
    
    #~ Translate:
    #~   <sectiondef kind="private-static-func">...</sectiondef>
    def _translate_sectiondef_private_static_func( self, sectiondef, target=None, **kwargs):
        return self._translate_sectiondef_func_(sectiondef,
            name='private static functions',target=target,**kwargs)
    
    #~ Translate:
    #~   <sectiondef kind="public-func">...</sectiondef>
    def _translate_sectiondef_private_func( self, sectiondef, target=None, **kwargs ):
        return self._translate_sectiondef_func_(sectiondef,
            name='private member functions',target=target,**kwargs)

    #~ Translate:
    #~   <sectiondef kind="user-defined"><header>...</header>...</sectiondef>
    def _translate_sectiondef_user_defined( self, sectiondef, target=None, **kwargs ):
        return self._translate_sectiondef_func_(sectiondef,
            name=self._getChildData('header', root=sectiondef),target=target,**kwargs)
    
    #~ Translate:
    #~   <memberdef kind="typedef" id="?">
    #~     <name>...</name>
    #~   </memberdef>
    #~ To:
    #~   <typedef id="?" name="?">
    #~     <type>...</type>
    #~   </typedef>
    def _translate_memberdef_typedef( self, memberdef, target=None, scope=None, **kwargs ):
        self._setID(memberdef.getAttribute('id'),
            scope+'::'+self._getChildData('name',root=memberdef))
        typedef = target.appendChild(self._createNode('typedef',
            id=memberdef.getAttribute('id'),
            name=self._getChildData('name',root=memberdef)))
        typedef_type = typedef.appendChild(self._createNode('type'))
        self._translate_type(self._getChild('type',root=memberdef),target=typedef_type)
        return typedef
    
    #~ Translate:
    #~   <memberdef kind="function" id="?" const="?" static="?" explicit="?" inline="?">
    #~     <name>...</name>
    #~   </memberdef>
    #~ To:
    #~   <method name="?" cv="?" specifiers="?">
    #~     ...
    #~   </method>
    def _translate_memberdef_function( self, memberdef, target=None, scope=None, **kwargs ):
        name = self._getChildData('name',root=memberdef)
        self._setID(memberdef.getAttribute('id'),scope+'::'+name)
        ## Check if we have some specific kind of method.
        if name == scope.split('::')[-1]:
            kind = 'constructor'
            target = target.parentNode
        elif name == '~'+scope.split('::')[-1]:
            kind = 'destructor'
            target = target.parentNode
        elif name == 'operator=':
            kind = 'copy-assignment'
            target = target.parentNode
        else:
            kind = 'method'
        method = target.appendChild(self._createNode(kind,
            # id=memberdef.getAttribute('id'),
            name=name,
            cv=' '.join([
                if_attribute(memberdef,'const','const','').strip()
                ]),
            specifiers=' '.join([
                if_attribute(memberdef,'static','static',''),
                if_attribute(memberdef,'explicit','explicit',''),
                if_attribute(memberdef,'inline','inline','')
                ]).strip()
            ))
        ## We iterate the children to translate each part of the function.
        for n in memberdef.childNodes:
            self._translateNode(memberdef,'function',n,target=method)
        return method
    
    #~ Translate:
    #~   <memberdef kind="function"...><templateparamlist>...</templateparamlist></memberdef>
    def _translate_memberdef_function_templateparamlist(
        self, templateparamlist, target=None, **kwargs ):
        return self._translate_templateparamlist(templateparamlist,target=target,**kwargs)
    
    #~ Translate:
    #~   <memberdef kind="function"...><type>...</type></memberdef>
    #~ To:
    #~   ...<type>?</type>
    def _translate_memberdef_function_type( self, resultType, target=None, **kwargs ):
        methodType = self._createNode('type')
        self._translate_type(resultType,target=methodType)
        if methodType.hasChildNodes():
            target.appendChild(methodType)
        return methodType
    
    #~ Translate:
    #~   <memberdef kind="function"...><briefdescription>...</briefdescription></memberdef>
    def _translate_memberdef_function_briefdescription( self, description, target=None, **kwargs ):
        result = self._translateDescription(description,target=target,**kwargs)
        ## For functions if we translate the brief docs to the purpose they end up
        ## right above the regular description. And since we just added the brief to that
        ## on the previous line, don't bother with the repetition.
        # result = self._translateDescription(description,target=target,tag='purpose',**kwargs)
        return result
    
    #~ Translate:
    #~   <memberdef kind="function"...><detaileddescription>...</detaileddescription></memberdef>
    def _translate_memberdef_function_detaileddescription( self, description, target=None, **kwargs ):
        return self._translateDescription(description,target=target,**kwargs)
    
    #~ Translate:
    #~   <memberdef kind="function"...><inbodydescription>...</inbodydescription></memberdef>
    def _translate_memberdef_function_inbodydescription( self, description, target=None, **kwargs ):
        return self._translateDescription(description,target=target,**kwargs)
    
    #~ Translate:
    #~   <memberdef kind="function"...><param>...</param></memberdef>
    def _translate_memberdef_function_param( self, param, target=None, **kwargs ):
        return self._translate_param(param,target=target,**kwargs)
    
    #~ Translate:
    #~   <memberdef kind="variable" id="?">
    #~     <name>...</name>
    #~     <type>...</type>
    #~   </memberdef>
    #~ To:
    #~   <data-member id="?" name="?">
    #~     <type>...</type>
    #~   </data-member>
    def _translate_memberdef_variable( self, memberdef, target=None, scope=None, **kwargs ):
        self._setID(memberdef.getAttribute('id'),
            scope+'::'+self._getChildData('name',root=memberdef))
        data_member = target.appendChild(self._createNode('data-member',
            id=memberdef.getAttribute('id'),
            name=self._getChildData('name',root=memberdef)))
        data_member_type = data_member.appendChild(self._createNode('type'))
        self._translate_type(self._getChild('type',root=memberdef),target=data_member_type)
    
    #~ Translate:
    #~   <memberdef kind="enum" id="?">
    #~     <name>...</name>
    #~     ...
    #~   </memberdef>
    #~ To:
    #~   <enum id="?" name="?">
    #~     ...
    #~   </enum>
    def _translate_memberdef_enum( self, memberdef, target=None, scope=None, **kwargs ):
        self._setID(memberdef.getAttribute('id'),
            scope+'::'+self._getChildData('name',root=memberdef))
        enum = target.appendChild(self._createNode('enum',
            id=memberdef.getAttribute('id'),
            name=self._getChildData('name',root=memberdef)))
        for n in memberdef.childNodes:
            self._translateNode(memberdef,'enum',n,target=enum,scope=scope,**kwargs)
        return enum
    
    #~ Translate:
    #~   <memberdef kind="enum"...>
    #~     <enumvalue id="?">
    #~       <name>...</name>
    #~       <initializer>...</initializer>
    #~     </enumvalue>
    #~   </memberdef>
    #~ To:
    #~   <enumvalue id="?" name="?">
    #~     <default>...</default>
    #~   </enumvalue>
    def _translate_memberdef_enum_enumvalue( self, enumvalue, target=None, scope=None, **kwargs ):
        self._setID(enumvalue.getAttribute('id'),
            scope+'::'+self._getChildData('name',root=enumvalue))
        value = target.appendChild(self._createNode('enumvalue',
            id=enumvalue.getAttribute('id'),
            name=self._getChildData('name',root=enumvalue)))
        initializer = self._getChild('initializer',root=enumvalue)
        if initializer:
            self._translateChildren(initializer,
                target=target.appendChild(self._createNode('default')))
        return value
    
    #~ Translate:
    #~   <param>
    #~     <type>...</type>
    #~     <declname>...</declname>
    #~     <defval>...</defval>
    #~   </param>
    #~ To:
    #~   <parameter name="?">
    #~     <paramtype>...</paramtype>
    #~     ...
    #~   </parameter>
    def _translate_param( self, param, target=None, **kwargs):
        parameter = target.appendChild(self._createNode('parameter',
            name=self._getChildData('declname',root=param)))
        paramtype = parameter.appendChild(self._createNode('paramtype'))
        self._translate_type(self._getChild('type',root=param),target=paramtype)
        defval = self._getChild('defval',root=param)
        if defval:
            self._translateChildren(self._getChild('defval',root=param),target=parameter)
        return parameter
    
    #~ Translate:
    #~   <ref kindref="?" ...>...</ref>
    def _translate_ref( self, ref, **kwargs ):
        return self._translateNode(ref,ref.getAttribute('kindref'))
    
    #~ Translate:
    #~   <ref refid="?" kindref="compound">...</ref>
    #~ To:
    #~   <link linkend="?"><classname>...</classname></link>
    def _translate_ref_compound( self, ref, **kwargs ):
        result = self._createNode('link',linkend=ref.getAttribute('refid'))
        classname = result.appendChild(self._createNode('classname'))
        self._translateChildren(ref,target=classname)
        return result
    
    #~ Translate:
    #~   <ref refid="?" kindref="member">...</ref>
    #~ To:
    #~   <link linkend="?">...</link>
    def _translate_ref_member( self, ref, **kwargs ):
        result = self._createNode('link',linkend=ref.getAttribute('refid'))
        self._translateChildren(ref,target=result)
        return result
    
    #~ Translate:
    #~   <type>...</type>
    def _translate_type( self, type, target=None, **kwargs ):
        result = self._translateChildren(type,target=target,**kwargs)
        #~ Filter types to clean up various readability problems, most notably
        #~ with really long types.
        xml = target.toxml('utf-8');
        if (
            xml.startswith('<type>boost::mpl::') or
            xml.startswith('<type>BOOST_PP_') or
            re.match('<type>boost::(lazy_)?(enable|disable)_if',xml)
            ):
            while target.firstChild:
                target.removeChild(target.firstChild)
            target.appendChild(self._createText('emphasis','unspecified'))
        return result
    
    def _getChild( self, tag = None, id = None, name = None, root = None ):
        if not root:
            root = self.boostbook.documentElement
        for n in root.childNodes:
            found = True
            if tag and found:
                found = found and tag == n.nodeName
            if id and found:
                if n.hasAttribute('id'):
                    found = found and n.getAttribute('id') == id
                else:
                    found = found and n.hasAttribute('id') and n.getAttribute('id') == id
            if name and found:
                found = found and n.hasAttribute('name') and n.getAttribute('name') == name
            if found:
                #~ print '--|', n
                return n
        return None
    
    def _getChildData( self, tag, **kwargs ):
        return self._getData(self._getChild(tag,**kwargs),**kwargs)
    
    def _getData( self, node, **kwargs ):
        if node:
            text = self._getChild('#text',root=node)
            if text:
                return text.data.strip()
        return ''
    
    def _cppName( self, type ):
        parts = re.search('^([^<]+)[<]?(.*)[>]?$',type.strip().strip(':'))
        result = {
            'compoundname' : parts.group(1),
            'namespace' : parts.group(1).split('::')[0:-1],
            'name' : parts.group(1).split('::')[-1],
            'specialization' : parts.group(2)
            }
        if result['namespace'] and len(result['namespace']) > 0:
            namespace = '::'.join(result['namespace'])
            while (
                len(result['namespace']) > 0 and (
                    not self.symbols.has_key(namespace) or
                    self.symbols[namespace]['kind'] != 'namespace')
                ):
                result['name'] = result['namespace'].pop()+'::'+result['name']
                namespace = '::'.join(result['namespace'])
        return result
    
    def _createNode( self, tag, **kwargs ):
        result = self.boostbook.createElement(tag)
        for k in kwargs.keys():
            if kwargs[k] != '':
                if k == 'id':
                    result.setAttribute('id',kwargs[k])
                else:
                    result.setAttribute(k,kwargs[k])
        return result
    
    def _createText( self, tag, data, **kwargs ):
        result = self._createNode(tag,**kwargs)
        data = data.strip()
        if len(data) > 0:
            result.appendChild(self.boostbook.createTextNode(data))
        return result


def main( xmldir=None, output=None, id=None, title=None, index=False ):
    #~ print '--- main: xmldir = %s, output = %s' % (xmldir,output)
    
    input = glob.glob( os.path.abspath( os.path.join( xmldir, "*.xml" ) ) )
    input.sort
    translator = Doxygen2BoostBook(id=id, title=title, index=index)
    #~ Feed in the namespaces first to build up the set of namespaces
    #~ and definitions so that lookup is unambiguous when reading in the definitions.
    namespace_files = filter(
        lambda x:
            os.path.basename(x).startswith('namespace'),
        input)
    decl_files = filter(
        lambda x:
            not os.path.basename(x).startswith('namespace') and not os.path.basename(x).startswith('_'),
        input)
    for dox in namespace_files:
        #~ print '--|',os.path.basename(dox)
        translator.addDox(xml.dom.minidom.parse(dox))
    for dox in decl_files:
        #~ print '--|',os.path.basename(dox)
        translator.addDox(xml.dom.minidom.parse(dox))
    
    if output:
        output = open(output,'w')
    else:
        output = sys.stdout
    if output:
        output.write(translator.tostring())


main( **get_args() )

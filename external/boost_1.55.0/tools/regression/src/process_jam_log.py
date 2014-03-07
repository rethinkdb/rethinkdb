#!/usr/bin/python
# Copyright 2008 Rene Rivera
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

import re
import optparse
import time
import xml.dom.minidom
import xml.dom.pulldom
from xml.sax.saxutils import unescape, escape
import os.path

#~ Process a bjam XML log into the XML log format for Boost result processing.
class BJamLog2Results:

    def __init__(self,args=None):
        opt = optparse.OptionParser(
            usage="%prog [options] input")
        opt.add_option( '--output',
            help="output file" )
        opt.add_option( '--runner',
            help="runner ID (e.g. 'Metacomm')" )
        opt.add_option( '--comment',
            help="an HTML comment file to be inserted in the reports" )
        opt.add_option( '--tag',
            help="the tag for the results" )
        opt.add_option( '--incremental',
            help="do incremental run (do not remove previous binaries)",
            action='store_true' )
        opt.add_option( '--platform' )
        opt.add_option( '--source' )
        opt.add_option( '--revision' )
        self.output = None
        self.runner = None
        self.comment='comment.html'
        self.tag='trunk'
        self.incremental=False
        self.platform=''
        self.source='SVN'
        self.revision=None
        self.input = []
        ( _opt_, self.input ) = opt.parse_args(args,self)
        if self.incremental:
            run_type = 'incremental'
        else:
            run_type = 'full'
        self.results = xml.dom.minidom.parseString('''<?xml version="1.0" encoding="UTF-8"?>
<test-run
  source="%(source)s"
  runner="%(runner)s"
  timestamp=""
  platform="%(platform)s"
  tag="%(tag)s"
  run-type="%(run-type)s"
  revision="%(revision)s">
</test-run>
''' % {
            'source' : self.source,
            'runner' : self.runner,
            'platform' : self.platform,
            'tag' : self.tag,
            'run-type' : run_type,
            'revision' : self.revision,
            } )
        
        self.test = {}
        self.target_to_test = {}
        self.target = {}
        self.parent = {}
        self.log = {}
        
        self.add_log()
        self.gen_output()
        
        #~ print self.test
        #~ print self.target
    
    def add_log(self):
        if self.input[0]:
            bjam_xml = self.input[0]
        else:
            bjam_xml = self.input[1]
        events = xml.dom.pulldom.parse(bjam_xml)
        context = []
        test_run = self.results.documentElement
        for (event,node) in events:
            if event == xml.dom.pulldom.START_ELEMENT:
                context.append(node)
                if node.nodeType == xml.dom.Node.ELEMENT_NODE:
                    x_f = self.x_name_(*context)
                    if x_f:
                        events.expandNode(node)
                        # expanding eats the end element, hence walking us out one level
                        context.pop()
                        # call the translator, and add returned items to the result
                        items = (x_f[1])(node)
                        if items:
                            for item in items:
                                if item:
                                    test_run.appendChild(self.results.createTextNode("\n"))
                                    test_run.appendChild(item)
            elif event == xml.dom.pulldom.END_ELEMENT:
                context.pop()
        #~ Add the log items nwo that we've collected all of them.
        items = self.log.values()
        if items:
            for item in items:
                if item:
                    test_run.appendChild(self.results.createTextNode("\n"))
                    test_run.appendChild(item)
    
    def gen_output(self):
        if self.output:
            out = open(self.output,'w')
        else:
            out = sys.stdout
        if out:
            self.results.writexml(out,encoding='utf-8')
    
    def tostring(self):
        return self.results.toxml('utf-8')
    
    def x_name_(self, *context, **kwargs):
        node = None
        names = [ ]
        for c in context:
            if c:
                if not isinstance(c,xml.dom.Node):
                    suffix = '_'+c.replace('-','_').replace('#','_')
                else:
                    suffix = '_'+c.nodeName.replace('-','_').replace('#','_')
                    node = c
                names.append('x')
                names = map(lambda x: x+suffix,names)
        if node:
            for name in names:
                if hasattr(self,name):
                    return (name,getattr(self,name))
        return None
    
    def x(self, *context, **kwargs):
        node = None
        names = [ ]
        for c in context:
            if c:
                if not isinstance(c,xml.dom.Node):
                    suffix = '_'+c.replace('-','_').replace('#','_')
                else:
                    suffix = '_'+c.nodeName.replace('-','_').replace('#','_')
                    node = c
                names.append('x')
                names = map(lambda x: x+suffix,names)
        if node:
            for name in names:
                if hasattr(self,name):
                    return getattr(self,name)(node,**kwargs)
                else:
                    assert False, 'Unknown node type %s'%(name)
        return None
    
    #~ The timestamp goes to the corresponding attribute in the result.
    def x_build_timestamp( self, node ):
        test_run = self.results.documentElement
        test_run.setAttribute('timestamp',self.get_data(node).strip())
        return None
    
    #~ Comment file becomes a comment node.
    def x_build_comment( self, node ):
        comment = None
        if self.comment:
            comment_f = open(self.comment)
            if comment_f:
                comment = comment_f.read()
                comment_f.close()
        if not comment:
            comment = ''
        return [self.new_text('comment',comment)]
    
    #~ Tests are remembered for future reference.
    def x_build_test( self, node ):
        test_run = self.results.documentElement
        test_node = node
        test_name = test_node.getAttribute('name')
        self.test[test_name] = {
            'library' : '/'.join(test_name.split('/')[0:-1]),
            'test-name' : test_name.split('/')[-1],
            'test-type' : test_node.getAttribute('type').lower(),
            'test-program' : self.get_child_data(test_node,tag='source',strip=True),
            'target' : self.get_child_data(test_node,tag='target',strip=True),
            'info' : self.get_child_data(test_node,tag='info',strip=True)
            }
        #~ Add a lookup for the test given the test target.
        self.target_to_test[self.test[test_name]['target']] = test_name
        #~ print "--- %s\n => %s" %(self.test[test_name]['target'],test_name)
        return None
    
    #~ Process the target dependency DAG into an ancestry tree so we can look up
    #~ which top-level library and test targets specific build actions correspond to.
    def x_build_targets_target( self, node ):
        test_run = self.results.documentElement
        target_node = node
        name = self.get_child_data(target_node,tag='name',strip=True)
        path = self.get_child_data(target_node,tag='path',strip=True)
        jam_target = self.get_child_data(target_node,tag='jam-target',strip=True)
        #~ print "--- target :: %s" %(name)
        #~ Map for jam targets to virtual targets.
        self.target[jam_target] = {
            'name' : name,
            'path' : path
            }
        #~ Create the ancestry.
        dep_node = self.get_child(self.get_child(target_node,tag='dependencies'),tag='dependency')
        while dep_node:
            child = self.get_data(dep_node,strip=True)
            child_jam_target = '<p%s>%s' % (path,child.split('//',1)[1])
            self.parent[child_jam_target] = jam_target
            #~ print "--- %s\n  ^ %s" %(jam_target,child_jam_target)
            dep_node = self.get_sibling(dep_node.nextSibling,tag='dependency')
        return None
    
    #~ Given a build action log, process into the corresponding test log and
    #~ specific test log sub-part.
    def x_build_action( self, node ):
        test_run = self.results.documentElement
        action_node = node
        name = self.get_child(action_node,tag='name')
        if name:
            name = self.get_data(name)
            #~ Based on the action, we decide what sub-section the log
            #~ should go into.
            action_type = None
            if re.match('[^%]+%[^.]+[.](compile)',name):
                action_type = 'compile'
            elif re.match('[^%]+%[^.]+[.](link|archive)',name):
                action_type = 'link'
            elif re.match('[^%]+%testing[.](capture-output)',name):
                action_type = 'run'
            elif re.match('[^%]+%testing[.](expect-failure|expect-success)',name):
                action_type = 'result'
            #~ print "+   [%s] %s %s :: %s" %(action_type,name,'','')
            if action_type:
                #~ Get the corresponding test.
                (target,test) = self.get_test(action_node,type=action_type)
                #~ Skip action that have no correspoding test as they are
                #~ regular build actions and don't need to show up in the
                #~ regression results.
                if not test:
                    return None
                #~ And the log node, which we will add the results to.
                log = self.get_log(action_node,test)
                #~ print "--- [%s] %s %s :: %s" %(action_type,name,target,test)
                #~ Collect some basic info about the action.
                result_data = "%(info)s\n\n%(command)s\n%(output)s\n" % {
                    'command' : self.get_action_command(action_node,action_type),
                    'output' : self.get_action_output(action_node,action_type),
                    'info' : self.get_action_info(action_node,action_type)
                    }
                #~ For the test result status we find the appropriate node
                #~ based on the type of test. Then adjust the result status
                #~ acorrdingly. This makes the result status reflect the
                #~ expectation as the result pages post processing does not
                #~ account for this inversion.
                action_tag = action_type
                if action_type == 'result':
                    if re.match(r'^compile',test['test-type']):
                        action_tag = 'compile'
                    elif re.match(r'^link',test['test-type']):
                        action_tag = 'link'
                    elif re.match(r'^run',test['test-type']):
                        action_tag = 'run'
                #~ The result sub-part we will add this result to.
                result_node = self.get_child(log,tag=action_tag)
                if action_node.getAttribute('status') == '0':
                    action_result = 'succeed'
                else:
                    action_result = 'fail'
                if not result_node:
                    #~ If we don't have one already, create it and add the result.
                    result_node = self.new_text(action_tag,result_data,
                        result = action_result,
                        timestamp = action_node.getAttribute('start'))
                    log.appendChild(self.results.createTextNode("\n"))
                    log.appendChild(result_node)
                else:
                    #~ For an existing result node we set the status to fail
                    #~ when any of the individual actions fail, except for result
                    #~ status.
                    if action_type != 'result':
                        result = result_node.getAttribute('result')
                        if action_node.getAttribute('status') != '0':
                            result = 'fail'
                    else:
                        result = action_result
                    result_node.setAttribute('result',result)
                    result_node.appendChild(self.results.createTextNode("\n"))
                    result_node.appendChild(self.results.createTextNode(result_data))
        return None
    
    #~ The command executed for the action. For run actions we omit the command
    #~ as it's just noise.
    def get_action_command( self, action_node, action_type ):
        if action_type != 'run':
            return self.get_child_data(action_node,tag='command')
        else:
            return ''
    
    #~ The command output.
    def get_action_output( self, action_node, action_type ):
        return self.get_child_data(action_node,tag='output',default='')
    
    #~ Some basic info about the action.
    def get_action_info( self, action_node, action_type ):
        info = ""
        #~ The jam action and target.
        info += "%s %s\n" %(self.get_child_data(action_node,tag='name'),
            self.get_child_data(action_node,tag='path'))
        #~ The timing of the action.
        info += "Time: (start) %s -- (end) %s -- (user) %s -- (system) %s\n" %(
            action_node.getAttribute('start'), action_node.getAttribute('end'),
            action_node.getAttribute('user'), action_node.getAttribute('system'))
        #~ And for compiles some context that may be hidden if using response files.
        if action_type == 'compile':
            define = self.get_child(self.get_child(action_node,tag='properties'),name='define')
            while define:
                info += "Define: %s\n" %(self.get_data(define,strip=True))
                define = self.get_sibling(define.nextSibling,name='define')
        return info
    
    #~ Find the test corresponding to an action. For testing targets these
    #~ are the ones pre-declared in the --dump-test option. For libraries
    #~ we create a dummy test as needed.
    def get_test( self, node, type = None ):
        jam_target = self.get_child_data(node,tag='jam-target')
        base = self.target[jam_target]['name']
        target = jam_target
        while target in self.parent:
            target = self.parent[target]
        #~ print "--- TEST: %s ==> %s" %(jam_target,target)
        #~ main-target-type is a precise indicator of what the build target is
        #~ proginally meant to be.
        main_type = self.get_child_data(self.get_child(node,tag='properties'),
            name='main-target-type',strip=True)
        if main_type == 'LIB' and type:
            lib = self.target[target]['name']
            if not lib in self.test:
                self.test[lib] = {
                    'library' : re.search(r'libs/([^/]+)',lib).group(1),
                    'test-name' : os.path.basename(lib),
                    'test-type' : 'lib',
                    'test-program' : os.path.basename(lib),
                    'target' : lib
                    }
            test = self.test[lib]
        else:
            target_name_ = self.target[target]['name']
            if self.target_to_test.has_key(target_name_):
                test = self.test[self.target_to_test[target_name_]]
            else:
                test = None
        return (base,test)
    
    #~ Find, or create, the test-log node to add results to.
    def get_log( self, node, test ):
        target_directory = os.path.dirname(self.get_child_data(
            node,tag='path',strip=True))
        target_directory = re.sub(r'.*[/\\]bin[.]v2[/\\]','',target_directory)
        target_directory = re.sub(r'[\\]','/',target_directory)
        if not target_directory in self.log:
            if 'info' in test and test['info'] == 'always_show_run_output':
                show_run_output = 'true'
            else:
                show_run_output = 'false'
            self.log[target_directory] = self.new_node('test-log',
                library=test['library'],
                test_name=test['test-name'],
                test_type=test['test-type'],
                test_program=test['test-program'],
                toolset=self.get_toolset(node),
                target_directory=target_directory,
                show_run_output=show_run_output)
        return self.log[target_directory]
    
    #~ The precise toolset from the build properties.
    def get_toolset( self, node ):
        toolset = self.get_child_data(self.get_child(node,tag='properties'),
            name='toolset',strip=True)
        toolset_version = self.get_child_data(self.get_child(node,tag='properties'),
            name='toolset-%s:version'%toolset,strip=True)
        return '%s-%s' %(toolset,toolset_version)
    
    #~ XML utilities...
    
    def get_sibling( self, sibling, tag = None, id = None, name = None, type = None ):
        n = sibling
        while n:
            found = True
            if type and found:
                found = found and type == n.nodeType
            if tag and found:
                found = found and tag == n.nodeName
            if (id or name) and found:
                found = found and n.nodeType == xml.dom.Node.ELEMENT_NODE
            if id and found:
                if n.hasAttribute('id'):
                    found = found and n.getAttribute('id') == id
                else:
                    found = found and n.hasAttribute('id') and n.getAttribute('id') == id
            if name and found:
                found = found and n.hasAttribute('name') and n.getAttribute('name') == name
            if found:
                return n
            n = n.nextSibling
        return None
    
    def get_child( self, root, tag = None, id = None, name = None, type = None ):
        return self.get_sibling(root.firstChild,tag=tag,id=id,name=name,type=type)
    
    def get_data( self, node, strip = False, default = None ):
        data = None
        if node:
            data_node = None
            if not data_node:
                data_node = self.get_child(node,tag='#text')
            if not data_node:
                data_node = self.get_child(node,tag='#cdata-section')
            data = ""
            while data_node:
                data += data_node.data
                data_node = data_node.nextSibling
                if data_node:
                    if data_node.nodeName != '#text' \
                        and data_node.nodeName != '#cdata-section':
                        data_node = None
        if not data:
            data = default
        else:
            if strip:
                data = data.strip()
        return data
    
    def get_child_data( self, root, tag = None, id = None, name = None, strip = False, default = None ):
        return self.get_data(self.get_child(root,tag=tag,id=id,name=name),strip=strip,default=default)
    
    def new_node( self, tag, *child, **kwargs ):
        result = self.results.createElement(tag)
        for k in kwargs.keys():
            if kwargs[k] != '':
                if k == 'id':
                    result.setAttribute('id',kwargs[k])
                elif k == 'klass':
                    result.setAttribute('class',kwargs[k])
                else:
                    result.setAttribute(k.replace('_','-'),kwargs[k])
        for c in child:
            if c:
                result.appendChild(c)
        return result
    
    def new_text( self, tag, data, **kwargs ):
        result = self.new_node(tag,**kwargs)
        data = data.strip()
        if len(data) > 0:
            result.appendChild(self.results.createTextNode(data))
        return result


if __name__ == '__main__': BJamLog2Results()

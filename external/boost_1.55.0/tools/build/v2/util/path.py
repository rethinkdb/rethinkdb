# Status: this module is ported on demand by however needs something
# from it.  Functionality that is not needed by Python port will
# be dropped.

#  Copyright (C) Vladimir Prus 2002. Permission to copy, use, modify, sell and
#  distribute this software is granted provided this copyright notice appears in
#  all copies. This software is provided "as is" without express or implied
#  warranty, and with no claim as to its suitability for any purpose.

#  Performs various path manipulations. Path are always in a 'normilized' 
#  representation. In it, a path may be either:
#
#     - '.', or
#
#     - ['/'] [ ( '..' '/' )*  (token '/')* token ]
# 
#   In plain english, path can be rooted, '..' elements are allowed only
#   at the beginning, and it never ends in slash, except for path consisting
#   of slash only.

import os.path
from utility import to_seq
from glob import glob as builtin_glob

from b2.util import bjam_signature

@bjam_signature((["path", "root"],))
def root (path, root):
    """ If 'path' is relative, it is rooted at 'root'. Otherwise, it's unchanged.
    """
    if os.path.isabs (path):
        return path
    else:
        return os.path.join (root, path)

@bjam_signature((["native"],))
def make (native):
    """ Converts the native path into normalized form.
    """
    # TODO: make os selection here.
    return make_UNIX (native)

def make_UNIX (native):

    # VP: I have no idea now 'native' can be empty here! But it can!
    assert (native)

    return os.path.normpath (native)

@bjam_signature((["path"],))
def native (path):
    """ Builds a native representation of the path.
    """
    # TODO: make os selection here.
    return native_UNIX (path)

def native_UNIX (path):
    return path


def pwd ():
    """ Returns the current working directory.
        # TODO: is it a good idea to use the current dir? Some use-cases 
                may not allow us to depend on the current dir.
    """
    return make (os.getcwd ())

def is_rooted (path):
    """ Tests if a path is rooted.
    """
    return path and path [0] == '/'


###################################################################
# Still to port.
# Original lines are prefixed with "#   "
#
#   #  Copyright (C) Vladimir Prus 2002. Permission to copy, use, modify, sell and
#   #  distribute this software is granted provided this copyright notice appears in
#   #  all copies. This software is provided "as is" without express or implied
#   #  warranty, and with no claim as to its suitability for any purpose.
#   
#   #  Performs various path manipulations. Path are always in a 'normilized' 
#   #  representation. In it, a path may be either:
#   #
#   #     - '.', or
#   #
#   #     - ['/'] [ ( '..' '/' )*  (token '/')* token ]
#   # 
#   #   In plain english, path can be rooted, '..' elements are allowed only
#   #   at the beginning, and it never ends in slash, except for path consisting
#   #   of slash only.
#   
#   import modules ;
#   import sequence ;
#   import regex ;
#   import errors : error ;
#   
#   
#   os = [ modules.peek : OS ] ;
#   if [ modules.peek : UNIX ] 
#   {    
#       local uname = [ modules.peek : JAMUNAME ] ;
#       switch $(uname)
#       {
#           case CYGWIN* :
#             os = CYGWIN ;
#           
#           case * :
#             os = UNIX ;
#       }        
#   }
#   
#   #
#   #    Tests if a path is rooted.
#   #
#   rule is-rooted ( path )
#   {
#       return [ MATCH "^(/)" : $(path) ] ;
#   }
#   
#   #
#   #    Tests if a path has a parent.
#   #
#   rule has-parent ( path )
#   {
#       if $(path) != / {
#           return 1 ;
#       } else {
#           return ;
#       }
#   }
#   
#   #
#   #    Returns the path without any directory components.
#   #
#   rule basename ( path )
#   {
#       return [ MATCH "([^/]+)$" : $(path) ] ;
#   }
#   
#   #
#   #    Returns parent directory of the path. If no parent exists, error is issued.
#   #
#   rule parent ( path )
#   {
#       if [ has-parent $(path) ] {
#   
#           if $(path) = . {
#               return .. ;
#           } else {
#   
#               # Strip everything at the end of path up to and including
#               # the last slash
#               local result = [ regex.match "((.*)/)?([^/]+)" : $(path) : 2 3 ] ;
#   
#               # Did we strip what we shouldn't?
#               if $(result[2]) = ".." {
#                   return $(path)/.. ;
#               } else {
#                   if ! $(result[1]) {
#                       if [ is-rooted $(path) ] {
#                           result = / ;
#                       } else {
#                           result = . ;
#                       }
#                   }
#                   return $(result[1]) ;
#               }
#           }
#       } else {
#           error "Path '$(path)' has no parent" ;
#       }
#   }
#   
#   #
#   #    Returns path2 such that "[ join path path2 ] = .".
#   #    The path may not contain ".." element or be rooted.
#   #
#   rule reverse ( path )
#   {
#       if $(path) = .
#       {
#           return $(path) ;
#       }
#       else
#       {
#           local tokens = [ regex.split $(path) "/" ] ;
#           local tokens2 ;
#           for local i in $(tokens) {
#               tokens2 += .. ;
#           }
#           return [ sequence.join $(tokens2) : "/" ] ;
#       }
#   }
#   
#   #
#   # Auxillary rule: does all the semantic of 'join', except for error cheching.
#   # The error checking is separated because this rule is recursive, and I don't
#   # like the idea of checking the same input over and over.
#   #
#   local rule join-imp ( elements + )
#   {
#       return [ NORMALIZE_PATH $(elements:J="/") ] ;
#   }
#   
#   #
#   #    Contanenates the passed path elements. Generates an error if
#   #    any element other than the first one is rooted.
#   #
#   rule join ( elements + )
#   {
#       if ! $(elements[2]) 
#       {
#           return $(elements[1]) ;
#       }
#       else
#       {        
#           for local e in $(elements[2-])
#           {
#               if [ is-rooted $(e) ]
#               {
#                   error only first element may be rooted ;
#               }
#           }
#           return [ join-imp $(elements) ] ;
#       }    
#   }


def glob (dirs, patterns):
    """ Returns the list of files matching the given pattern in the
    specified directory.  Both directories and patterns are 
    supplied as portable paths. Each pattern should be non-absolute
    path, and can't contain "." or ".." elements. Each slash separated
    element of pattern can contain the following special characters:
    -  '?', which match any character
    -  '*', which matches arbitrary number of characters.
    A file $(d)/e1/e2/e3 (where 'd' is in $(dirs)) matches pattern p1/p2/p3
    if and only if e1 matches p1, e2 matches p2 and so on.
    
    For example: 
        [ glob . : *.cpp ] 
        [ glob . : */build/Jamfile ] 
    """
#   {
#       local result ;
#       if $(patterns:D)
#       {
#           # When a pattern has a directory element, we first glob for
#           # directory, and then glob for file name is the found directories.
#           for local p in $(patterns)
#           {
#               # First glob for directory part.
#               local globbed-dirs = [ glob $(dirs) : $(p:D) ] ;
#               result += [ glob $(globbed-dirs) : $(p:D="") ] ;
#           }        
#       }
#       else
#       {        
#           # When a pattern has not directory, we glob directly.
#           # Take care of special ".." value. The "GLOB" rule simply ignores
#           # the ".." element (and ".") element in directory listings. This is
#           # needed so that 
#           #
#           #    [ glob libs/*/Jamfile ]
#           #
#           # don't return 
#           #
#           #    libs/../Jamfile (which is the same as ./Jamfile)
#           #
#           # On the other hand, when ".." is explicitly present in the pattern
#           # we need to return it.
#           # 
#           for local dir in $(dirs)
#           {
#               for local p in $(patterns)
#               {                
#                   if $(p) != ".."
#                   {                
#                       result += [ sequence.transform make 
#                           : [ GLOB [ native $(dir) ] : $(p) ] ] ;
#                   }            
#                   else
#                   {
#                       result += [ path.join $(dir) .. ] ;
#                   }            
#               }            
#           }
#       }    
#       return $(result) ;
#   }
#   

# TODO: (PF) I replaced the code above by this. I think it should work but needs to be tested.
    result = []
    dirs = to_seq (dirs)
    patterns = to_seq (patterns)

    splitdirs = []
    for dir in dirs:
        splitdirs += dir.split (os.pathsep)

    for dir in splitdirs:
        for pattern in patterns:
            p = os.path.join (dir, pattern)
            import glob
            result.extend (glob.glob (p))
    return result
    
#
#   Find out the absolute name of path and returns the list of all the parents,
#   starting with the immediate one. Parents are returned as relative names.
#   If 'upper_limit' is specified, directories above it will be pruned.
#
def all_parents(path, upper_limit=None, cwd=None):

    if not cwd:
        cwd = os.getcwd()

    path_abs = os.path.join(cwd, path)

    if upper_limit:
        upper_limit = os.path.join(cwd, upper_limit)

    result = []
    while path_abs and path_abs != upper_limit:
        (head, tail) = os.path.split(path)
        path = os.path.join(path, "..")        
        result.append(path)
        path_abs = head

    if upper_limit and path_abs != upper_limit:
        raise BaseException("'%s' is not a prefix of '%s'" % (upper_limit, path))

    return result
    
#  Search for 'pattern' in parent directories of 'dir', up till and including
#  'upper_limit', if it is specified, or till the filesystem root otherwise.
#
def glob_in_parents(dir, patterns, upper_limit=None):

    result = []
    parent_dirs = all_parents(dir, upper_limit)

    for p in parent_dirs:
        result = glob(p, patterns)
        if result: break

    return result

#   
#   #
#   # Assuming 'child' is a subdirectory of 'parent', return the relative
#   # path from 'parent' to 'child'
#   #
#   rule relative ( child parent )
#   {
#       if $(parent) = "." 
#       {
#           return $(child) ;
#       }
#       else 
#       {       
#           local split1 = [ regex.split $(parent) / ] ;
#           local split2 = [ regex.split $(child) / ] ;
#       
#           while $(split1)
#           {
#               if $(split1[1]) = $(split2[1])
#               {
#                   split1 = $(split1[2-]) ;
#                   split2 = $(split2[2-]) ;
#               }
#               else
#               {
#                   errors.error $(child) is not a subdir of $(parent) ;
#               }                
#           }    
#           return [ join $(split2) ] ;    
#       }    
#   }
#   
#   # Returns the minimal path to path2 that is relative path1.
#   #
#   rule relative-to ( path1 path2 )
#   {
#       local root_1 = [ regex.split [ reverse $(path1) ] / ] ;
#       local split1 = [ regex.split $(path1) / ] ;
#       local split2 = [ regex.split $(path2) / ] ;
#   
#       while $(split1) && $(root_1)
#       {
#           if $(split1[1]) = $(split2[1])
#           {
#               root_1 = $(root_1[2-]) ;
#               split1 = $(split1[2-]) ;
#               split2 = $(split2[2-]) ;
#           }
#           else
#           {
#               split1 = ;
#           }
#       }
#       return [ join . $(root_1) $(split2) ] ;
#   }

# Returns the list of paths which are used by the operating system
# for looking up programs
def programs_path ():
    raw = []
    names = ['PATH', 'Path', 'path']
    
    for name in names:
        raw.append(os.environ.get (name, ''))
    
    result = []
    for elem in raw:
        if elem:
            for p in elem.split(os.path.pathsep):
                result.append(make(p))

    return result

#   rule make-NT ( native )
#   {
#       local tokens = [ regex.split $(native) "[/\\]" ] ;
#       local result ;
#   
#       # Handle paths ending with slashes
#       if $(tokens[-1]) = ""
#       {
#           tokens = $(tokens[1--2]) ; # discard the empty element
#       }
#   
#       result = [ path.join $(tokens) ] ;
#   
#       if [ regex.match "(^.:)" : $(native)  ]
#       {
#           result = /$(result) ;
#       }
#       
#       if $(native) = ""
#       {
#           result = "." ;
#       }
#           
#       return $(result) ;
#   }
#   
#   rule native-NT ( path )
#   {
#       local result = [ MATCH "^/?(.*)" : $(path) ] ;
#       result = [ sequence.join [ regex.split $(result) "/" ] : "\\" ] ;
#       return $(result) ;
#   }
#   
#   rule make-CYGWIN ( path )
#   {
#       return [ make-NT $(path) ] ;
#   }
#   
#   rule native-CYGWIN ( path )
#   {
#       local result = $(path) ;
#       if [ regex.match "(^/.:)" : $(path)  ] # win absolute
#       {
#           result = [ MATCH "^/?(.*)" : $(path) ] ; # remove leading '/'
#       }
#       return [ native-UNIX $(result) ] ;
#   }
#   
#   #
#   # split-VMS: splits input native path into
#   # device dir file (each part is optional),
#   # example:
#   #
#   # dev:[dir]file.c => dev: [dir] file.c
#   #
#   rule split-path-VMS ( native )
#   {
#       local matches = [ MATCH ([a-zA-Z0-9_-]+:)?(\\[[^\]]*\\])?(.*)?$   : $(native) ] ;
#       local device = $(matches[1]) ;
#       local dir = $(matches[2]) ;
#       local file = $(matches[3]) ;
#   
#       return $(device) $(dir) $(file) ;
#   }
#   
#   #
#   # Converts a native VMS path into a portable path spec.
#   #
#   # Does not handle current-device absolute paths such
#   # as "[dir]File.c" as it is not clear how to represent
#   # them in the portable path notation.
#   #
#   # Adds a trailing dot (".") to the file part if no extension
#   # is present (helps when converting it back into native path).
#   #
#   rule make-VMS ( native )
#   {
#       if [ MATCH ^(\\[[a-zA-Z0-9]) : $(native) ]
#       {
#           errors.error "Can't handle default-device absolute paths: " $(native) ;
#       }
#           
#       local parts = [ split-path-VMS $(native) ] ;
#       local device = $(parts[1]) ;
#       local dir = $(parts[2]) ;
#       local file = $(parts[3]) ;
#       local elems ;
#       
#       if $(device)
#       {
#           #
#           # rooted
#           #
#           elems = /$(device) ;
#       }
#       
#       if $(dir) = "[]"
#       {
#           #
#           # Special case: current directory
#           #
#           elems = $(elems) "." ;
#       }
#       else if $(dir)
#       {
#           dir = [ regex.replace $(dir) "\\[|\\]" "" ] ;
#           local dir_parts = [ regex.split $(dir) \\. ]  ;
#       
#           if $(dir_parts[1]) = ""
#           {
#               #
#               # Relative path
#               #
#               dir_parts = $(dir_parts[2--1]) ;
#           }
#           
#           #
#           # replace "parent-directory" parts (- => ..)
#           #
#           dir_parts = [ regex.replace-list $(dir_parts) : - : .. ] ;
#           
#           elems = $(elems) $(dir_parts) ;
#       }
#       
#       if $(file)
#       {
#           if ! [ MATCH (\\.) : $(file) ]
#           {
#               #
#               # Always add "." to end of non-extension file
#               #
#               file = $(file). ;
#           }
#           elems = $(elems) $(file) ;
#       }
#   
#       local portable = [ path.join $(elems) ] ;
#   
#       return $(portable) ;
#   }
#   
#   #
#   # Converts a portable path spec into a native VMS path.
#   #
#   # Relies on having at least one dot (".") included in the file
#   # name to be able to differentiate it ftom the directory part.
#   #
#   rule native-VMS ( path )
#   {
#       local device = "" ;
#       local dir = $(path) ;
#       local file = "" ;
#       local native ;
#       local split ;
#   
#       #
#       # Has device ?
#       #
#       if [ is-rooted $(dir) ]
#       {
#           split = [ MATCH ^/([^:]+:)/?(.*) : $(dir) ] ;
#           device = $(split[1]) ;
#           dir = $(split[2]) ;
#       }
#   
#       #
#       # Has file ?
#       #
#       # This is no exact science, just guess work:
#       #
#       # If the last part of the current path spec
#       # includes some chars, followed by a dot,
#       # optionally followed by more chars -
#       # then it is a file (keep your fingers crossed).
#       #
#       split = [ regex.split $(dir) / ] ;
#       local maybe_file = $(split[-1]) ;
#   
#       if [ MATCH ^([^.]+\\..*) : $(maybe_file) ]
#       {
#           file = $(maybe_file) ;
#           dir = [ sequence.join $(split[1--2]) : / ] ;
#       }
#       
#       #
#       # Has dir spec ?
#       #
#       if $(dir) = "."
#       {
#           dir = "[]" ;
#       }
#       else if $(dir)
#       {
#           dir = [ regex.replace $(dir) \\.\\. - ] ;
#           dir = [ regex.replace $(dir) / . ] ;
#   
#           if $(device) = ""
#           {
#               #
#               # Relative directory
#               # 
#               dir = "."$(dir) ;
#           }
#           dir = "["$(dir)"]" ;
#       }
#       
#       native = [ sequence.join $(device) $(dir) $(file) ] ;
#   
#       return $(native) ;
#   }
#   
#   
#   rule __test__ ( ) {
#   
#       import assert ;
#       import errors : try catch ;
#   
#       assert.true is-rooted "/" ;
#       assert.true is-rooted "/foo" ;
#       assert.true is-rooted "/foo/bar" ;
#       assert.result : is-rooted "." ;
#       assert.result : is-rooted "foo" ;
#       assert.result : is-rooted "foo/bar" ;
#   
#       assert.true has-parent "foo" ;
#       assert.true has-parent "foo/bar" ;
#       assert.true has-parent "." ;
#       assert.result : has-parent "/" ;
#   
#       assert.result "." : basename "." ;
#       assert.result ".." : basename ".." ;
#       assert.result "foo" : basename "foo" ;
#       assert.result "foo" : basename "bar/foo" ;
#       assert.result "foo" : basename "gaz/bar/foo" ;
#       assert.result "foo" : basename "/gaz/bar/foo" ;
#   
#       assert.result "." : parent "foo" ;
#       assert.result "/" : parent "/foo" ;
#       assert.result "foo/bar" : parent "foo/bar/giz" ;
#       assert.result ".." : parent "." ;
#       assert.result ".." : parent "../foo" ;
#       assert.result "../../foo" : parent "../../foo/bar" ;
#   
#   
#       assert.result "." : reverse "." ;
#       assert.result ".." : reverse "foo" ;
#       assert.result "../../.." : reverse "foo/bar/giz" ;
#   
#       assert.result "foo" : join "foo" ;
#       assert.result "/foo" : join "/" "foo" ;
#       assert.result "foo/bar" : join "foo" "bar" ;
#       assert.result "foo/bar" : join "foo/giz" "../bar" ;
#       assert.result "foo/giz" : join "foo/bar/baz" "../../giz" ;
#       assert.result ".." : join "." ".." ;
#       assert.result ".." : join "foo" "../.." ;
#       assert.result "../.." : join "../foo" "../.." ;
#       assert.result "/foo" : join "/bar" "../foo" ;
#       assert.result "foo/giz" : join "foo/giz" "." ;
#       assert.result "." : join lib2 ".." ;
#       assert.result "/" : join "/a" ".." ;
#   
#       assert.result /a/b : join /a/b/c .. ;
#   
#       assert.result "foo/bar/giz" : join "foo" "bar" "giz" ;
#       assert.result "giz" : join "foo" ".." "giz" ;
#       assert.result "foo/giz" : join "foo" "." "giz" ;
#   
#       try ;
#       {
#           join "a" "/b" ;
#       }
#       catch only first element may be rooted ;
#   
#       local CWD = "/home/ghost/build" ;
#       assert.result : all-parents . : . : $(CWD) ;
#       assert.result . .. ../.. ../../..  : all-parents "Jamfile" : "" : $(CWD) ;
#       assert.result foo . .. ../.. ../../.. : all-parents "foo/Jamfile" : "" : $(CWD) ;
#       assert.result ../Work .. ../.. ../../.. : all-parents "../Work/Jamfile" : "" : $(CWD) ;
#   
#       local CWD = "/home/ghost" ;
#       assert.result . .. : all-parents "Jamfile" : "/home" : $(CWD) ;
#       assert.result . : all-parents "Jamfile" : "/home/ghost" : $(CWD) ;
#       
#       assert.result "c/d" : relative "a/b/c/d" "a/b" ;
#       assert.result "foo" : relative "foo" "." ;
#   
#       local save-os = [ modules.peek path : os ] ;
#       modules.poke path : os : NT ;
#   
#       assert.result "foo/bar/giz" : make "foo/bar/giz" ;
#       assert.result "foo/bar/giz" : make "foo\\bar\\giz" ;
#       assert.result "foo" : make "foo/." ;
#       assert.result "foo" : make "foo/bar/.." ;
#       assert.result "/D:/My Documents" : make "D:\\My Documents" ;
#       assert.result "/c:/boost/tools/build/new/project.jam" : make "c:\\boost\\tools\\build\\test\\..\\new\\project.jam" ;
#   
#       assert.result "foo\\bar\\giz" : native "foo/bar/giz" ;
#       assert.result "foo" : native "foo" ;
#       assert.result "D:\\My Documents\\Work" : native "/D:/My Documents/Work" ;
#   
#       modules.poke path : os : UNIX ;
#   
#       assert.result "foo/bar/giz" : make "foo/bar/giz" ;
#       assert.result "/sub1" : make "/sub1/." ;
#       assert.result "/sub1" : make "/sub1/sub2/.." ;    
#       assert.result "sub1" : make "sub1/." ;
#       assert.result "sub1" : make "sub1/sub2/.." ;
#       assert.result "/foo/bar" : native "/foo/bar" ;
#   
#       modules.poke path : os : VMS ;
#   
#       #
#       # Don't really need to poke os before these
#       #
#       assert.result "disk:" "[dir]"  "file" : split-path-VMS "disk:[dir]file" ;
#       assert.result "disk:" "[dir]"  ""     : split-path-VMS "disk:[dir]" ;
#       assert.result "disk:" ""     ""       : split-path-VMS "disk:" ;
#       assert.result "disk:" ""     "file"   : split-path-VMS "disk:file" ;
#       assert.result ""      "[dir]"  "file" : split-path-VMS "[dir]file" ;
#       assert.result ""      "[dir]"  ""     : split-path-VMS "[dir]" ;
#       assert.result ""      ""     "file"   : split-path-VMS "file" ;
#       assert.result ""      ""     ""       : split-path-VMS "" ;
#   
#       #
#       # Special case: current directory
#       #
#       assert.result ""      "[]"     ""     : split-path-VMS "[]" ;
#       assert.result "disk:" "[]"     ""     : split-path-VMS "disk:[]" ;
#       assert.result ""      "[]"     "file" : split-path-VMS "[]file" ;
#       assert.result "disk:" "[]"     "file" : split-path-VMS "disk:[]file" ;
#   
#       #
#       # Make portable paths
#       #
#       assert.result "/disk:" : make "disk:" ;
#       assert.result "foo/bar/giz" : make "[.foo.bar.giz]" ;
#       assert.result "foo" : make "[.foo]" ;
#       assert.result "foo" : make "[.foo.bar.-]" ;
#       assert.result ".." : make "[.-]" ;
#       assert.result ".." : make "[-]" ;
#       assert.result "." : make "[]" ;
#       assert.result "giz.h" : make "giz.h" ;
#       assert.result "foo/bar/giz.h" : make "[.foo.bar]giz.h" ;
#       assert.result "/disk:/my_docs" : make "disk:[my_docs]" ;
#       assert.result "/disk:/boost/tools/build/new/project.jam" : make "disk:[boost.tools.build.test.-.new]project.jam" ;
#   
#       #
#       # Special case (adds '.' to end of file w/o extension to
#       # disambiguate from directory in portable path spec).
#       #
#       assert.result "Jamfile." : make "Jamfile" ;
#       assert.result "dir/Jamfile." : make "[.dir]Jamfile" ;
#       assert.result "/disk:/dir/Jamfile." : make "disk:[dir]Jamfile" ;
#   
#       #
#       # Make native paths
#       #
#       assert.result "disk:" : native "/disk:" ;
#       assert.result "[.foo.bar.giz]" : native "foo/bar/giz" ;
#       assert.result "[.foo]" : native "foo" ;
#       assert.result "[.-]" : native ".." ;
#       assert.result "[.foo.-]" : native "foo/.." ;
#       assert.result "[]" : native "." ;
#       assert.result "disk:[my_docs.work]" : native "/disk:/my_docs/work" ;
#       assert.result "giz.h" : native "giz.h" ;
#       assert.result "disk:Jamfile." : native "/disk:Jamfile." ;
#       assert.result "disk:[my_docs.work]Jamfile." : native "/disk:/my_docs/work/Jamfile." ;
#   
#       modules.poke path : os : $(save-os) ;
#   
#   }

#


#def glob(dir, patterns):
#    result = []
#    for pattern in patterns:
#        result.extend(builtin_glob(os.path.join(dir, pattern)))
#    return result

def glob(dirs, patterns, exclude_patterns=None):
    """Returns the list of files matching the given pattern in the
    specified directory.  Both directories and patterns are 
    supplied as portable paths. Each pattern should be non-absolute
    path, and can't contain '.' or '..' elements. Each slash separated
    element of pattern can contain the following special characters:
    -  '?', which match any character
    -  '*', which matches arbitrary number of characters.
    A file $(d)/e1/e2/e3 (where 'd' is in $(dirs)) matches pattern p1/p2/p3
    if and only if e1 matches p1, e2 matches p2 and so on.
    For example: 
        [ glob . : *.cpp ] 
        [ glob . : */build/Jamfile ]
    """

    assert(isinstance(patterns, list))
    assert(isinstance(dirs, list))

    if not exclude_patterns:
        exclude_patterns = []
    else:
       assert(isinstance(exclude_patterns, list))

    real_patterns = [os.path.join(d, p) for p in patterns for d in dirs]    
    real_exclude_patterns = [os.path.join(d, p) for p in exclude_patterns
                             for d in dirs]

    inc = [os.path.normpath(name) for p in real_patterns
           for name in builtin_glob(p)]
    exc = [os.path.normpath(name) for p in real_exclude_patterns
           for name in builtin_glob(p)]
    return [x for x in inc if x not in exc]

def glob_tree(roots, patterns, exclude_patterns=None):
    """Recursive version of GLOB. Builds the glob of files while
    also searching in the subdirectories of the given roots. An
    optional set of exclusion patterns will filter out the
    matching entries from the result. The exclusions also apply
    to the subdirectory scanning, such that directories that
    match the exclusion patterns will not be searched."""

    if not exclude_patterns:
        exclude_patterns = []

    result = glob(roots, patterns, exclude_patterns)
    subdirs = [s for s in glob(roots, ["*"]) if s != "." and s != ".." and os.path.isdir(s)]
    if subdirs:
        result.extend(glob_tree(subdirs, patterns, exclude_patterns))
        
    return result

def glob_in_parents(dir, patterns, upper_limit=None):
    """Recursive version of GLOB which glob sall parent directories
    of dir until the first match is found. Returns an empty result if no match
    is found"""    
    
    assert(isinstance(dir, str))
    assert(isinstance(patterns, list))

    result = []

    absolute_dir = os.path.join(os.getcwd(), dir)
    absolute_dir = os.path.normpath(absolute_dir)
    while absolute_dir:
        new_dir = os.path.split(absolute_dir)[0]
        if new_dir == absolute_dir:
            break
        result = glob([new_dir], patterns)
        if result:
            break
        absolute_dir = new_dir

    return result


# The relpath functionality is written by
# Cimarron Taylor
def split(p, rest=[]):
    (h,t) = os.path.split(p)
    if len(h) < 1: return [t]+rest
    if len(t) < 1: return [h]+rest
    return split(h,[t]+rest)

def commonpath(l1, l2, common=[]):
    if len(l1) < 1: return (common, l1, l2)
    if len(l2) < 1: return (common, l1, l2)
    if l1[0] != l2[0]: return (common, l1, l2)
    return commonpath(l1[1:], l2[1:], common+[l1[0]])

def relpath(p1, p2):
    (common,l1,l2) = commonpath(split(p1), split(p2))
    p = []
    if len(l1) > 0:
        p = [ '../' * len(l1) ]
    p = p + l2
    if p:
        return os.path.join( *p )
    else:
        return "."

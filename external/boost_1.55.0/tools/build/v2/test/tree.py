# Copyright 2003 Dave Abrahams
# Copyright 2001, 2002 Vladimir Prus
# Copyright 2012 Jurko Gospodnetic
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

###############################################################################
#
# Based in part on an old Subversion tree.py source file (tools for comparing
# directory trees). See http://subversion.tigris.org for more information.
#
# Copyright (c) 2001 Sam Tobin-Hochstadt.  All rights reserved.
#
# This software is licensed as described in the file COPYING, which you should
# have received as part of this distribution. The terms are also available at
# http://subversion.tigris.org/license-1.html. If newer versions of this
# license are posted there, you may use a newer version instead, at your
# option.
#
###############################################################################

import os
import os.path
import stat
import sys


class TreeNode:
    """
      Fundamental data type used to build file system tree structures.

      If CHILDREN is None, then the node represents a file. Otherwise, CHILDREN
    is a list of the nodes representing that directory's children.

      NAME is simply the name of the file or directory. CONTENTS is a string
    holding the file's contents (if a file).

    """

    def __init__(self, name, children=None, contents=None):
        assert children is None or contents is None
        self.name = name
        self.mtime = 0
        self.children = children
        self.contents = contents
        self.path = name

    def add_child(self, newchild):
        assert not self.is_file()
        for a in self.children:
            if a.name == newchild.name:
                if newchild.is_file():
                    a.contents = newchild.contents
                    a.path = os.path.join(self.path, newchild.name)
                else:
                    for i in newchild.children:
                        a.add_child(i)
                break
        else:
            self.children.append(newchild)
            newchild.path = os.path.join(self.path, newchild.name)

    def get_child(self, name):
        """
          If the given TreeNode directory NODE contains a child named NAME,
        return the child; else, return None.

        """
        for n in self.children:
            if n.name == name:
                return n

    def is_file(self):
        return self.children is None

    def pprint(self):
        print(" * Node name: %s" % self.name)
        print("    Path:     %s" % self.path)
        print("    Contents: %s" % self.contents)
        if self.is_file():
            print("    Children: is a file.")
        else:
            print("    Children: %d" % len(self.children))


class TreeDifference:
    def __init__(self):
        self.added_files = []
        self.removed_files = []
        self.modified_files = []
        self.touched_files = []

    def append(self, other):
        self.added_files.extend(other.added_files)
        self.removed_files.extend(other.removed_files)
        self.modified_files.extend(other.modified_files)
        self.touched_files.extend(other.touched_files)

    def ignore_directories(self):
        """Removes directories from our lists of found differences."""
        not_dir = lambda x : x[-1] != "/"
        self.added_files = filter(not_dir, self.added_files)
        self.removed_files = filter(not_dir, self.removed_files)
        self.modified_files = filter(not_dir, self.modified_files)
        self.touched_files = filter(not_dir, self.touched_files)

    def pprint(self, file=sys.stdout):
        file.write("Added files   : %s\n" % self.added_files)
        file.write("Removed files : %s\n" % self.removed_files)
        file.write("Modified files: %s\n" % self.modified_files)
        file.write("Touched files : %s\n" % self.touched_files)

    def empty(self):
        return not (self.added_files or self.removed_files or
            self.modified_files or self.touched_files)


def build_tree(path):
    """
      Takes PATH as the folder path, walks the file system below that path, and
    creates a tree structure based on any files and folders found there.
    Returns the prepared tree structure plus the maximum file modification
    timestamp under the given folder.

    """
    return _handle_dir(os.path.normpath(path))


def tree_difference(a, b):
    """Compare TreeNodes A and B, and create a TreeDifference instance."""
    return _do_tree_difference(a, b, "", True)


def _do_tree_difference(a, b, parent_path, root=False):
    """Internal recursive worker function for tree_difference()."""

    # We do not want to list root node names.
    if root:
        assert not parent_path
        assert not a.is_file()
        assert not b.is_file()
        full_path = ""
    else:
        assert a.name == b.name
        full_path = parent_path + a.name
    result = TreeDifference()

    # A and B are both files.
    if a.is_file() and b.is_file():
        if a.contents != b.contents:
            result.modified_files.append(full_path)
        elif a.mtime != b.mtime:
            result.touched_files.append(full_path)
        return result

    # Directory converted to file.
    if not a.is_file() and b.is_file():
        result.removed_files.extend(_traverse_tree(a, parent_path))
        result.added_files.append(full_path)

    # File converted to directory.
    elif a.is_file() and not b.is_file():
        result.removed_files.append(full_path)
        result.added_files.extend(_traverse_tree(b, parent_path))

    # A and B are both directories.
    else:
        if full_path:
            full_path += "/"
        accounted_for = []  # Children present in both trees.
        for a_child in a.children:
            b_child = b.get_child(a_child.name)
            if b_child:
                accounted_for.append(b_child)
                result.append(_do_tree_difference(a_child, b_child, full_path))
            else:
                result.removed_files.append(full_path + a_child.name)
        for b_child in b.children:
            if b_child not in accounted_for:
                result.added_files.extend(_traverse_tree(b_child, full_path))

    return result


def _traverse_tree(t, parent_path):
    """Returns a list of all names in a tree."""
    assert not parent_path or parent_path[-1] == "/"
    full_node_name = parent_path + t.name
    if t.is_file():
        result = [full_node_name]
    else:
        name_prefix = full_node_name + "/"
        result = [name_prefix]
        for i in t.children:
            result.extend(_traverse_tree(i, name_prefix))
    return result


def _get_text(path):
    """Return a string with the textual contents of a file at PATH."""
    fp = open(path, 'r')
    try:
        return fp.read()
    finally:
        fp.close()


def _handle_dir(path):
    """
      Main recursive worker function for build_tree(). Returns a newly created
    tree node representing the given normalized folder path as well as the
    maximum file/folder modification time detected under the same path.

    """
    files = []
    dirs = []
    node = TreeNode(os.path.basename(path), children=[])
    max_mtime = node.mtime = os.stat(path).st_mtime

    # List files & folders.
    for f in os.listdir(path):
        f = os.path.join(path, f)
        if os.path.isdir(f):
            dirs.append(f)
        elif os.path.isfile(f):
            files.append(f)

    # Add a child node for each file.
    for f in files:
        fcontents = _get_text(f)
        new_file_node = TreeNode(os.path.basename(f), contents=fcontents)
        new_file_node.mtime = os.stat(f).st_mtime
        max_mtime = max(max_mtime, new_file_node.mtime)
        node.add_child(new_file_node)

    # For each subdir, create a node, walk its tree, add it as a child.
    for d in dirs:
        new_dir_node, new_max_mtime = _handle_dir(d)
        max_mtime = max(max_mtime, new_max_mtime)
        node.add_child(new_dir_node)

    return node, max_mtime

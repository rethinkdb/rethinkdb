#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Traverses the source tree, parses all found DEPS files, and constructs
a dependency rule table to be used by subclasses.

The format of the deps file:

First you have the normal module-level deps. These are the ones used by
gclient. An example would be:

  deps = {
    "base":"http://foo.bar/trunk/base"
  }

DEPS files not in the top-level of a module won't need this. Then you
have any additional include rules. You can add (using "+") or subtract
(using "-") from the previously specified rules (including
module-level deps). You can also specify a path that is allowed for
now but that we intend to remove, using "!"; this is treated the same
as "+" when check_deps is run by our bots, but a presubmit step will
show a warning if you add a new include of a file that is only allowed
by "!".

Note that for .java files, there is currently no difference between
"+" and "!", even in the presubmit step.

  include_rules = [
    # Code should be able to use base (it's specified in the module-level
    # deps above), but nothing in "base/evil" because it's evil.
    "-base/evil",

    # But this one subdirectory of evil is OK.
    "+base/evil/not",

    # And it can include files from this other directory even though there is
    # no deps rule for it.
    "+tools/crime_fighter",

    # This dependency is allowed for now but work is ongoing to remove it,
    # so you shouldn't add further dependencies on it.
    "!base/evil/ok_for_now.h",
  ]

If you have certain include rules that should only be applied for some
files within this directory and subdirectories, you can write a
section named specific_include_rules that is a hash map of regular
expressions to the list of rules that should apply to files matching
them.  Note that such rules will always be applied before the rules
from 'include_rules' have been applied, but the order in which rules
associated with different regular expressions is applied is arbitrary.

  specific_include_rules = {
    ".*_(unit|browser|api)test\.cc": [
      "+libraries/testsupport",
    ],
  }

DEPS files may be placed anywhere in the tree. Each one applies to all
subdirectories, where there may be more DEPS files that provide additions or
subtractions for their own sub-trees.

There is an implicit rule for the current directory (where the DEPS file lives)
and all of its subdirectories. This prevents you from having to explicitly
allow the current directory everywhere.  This implicit rule is applied first,
so you can modify or remove it using the normal include rules.

The rules are processed in order. This means you can explicitly allow a higher
directory and then take away permissions from sub-parts, or the reverse.

Note that all directory separators must be slashes (Unix-style) and not
backslashes. All directories should be relative to the source root and use
only lowercase.
"""

import copy
import os.path
import posixpath
import subprocess

from rules import Rule, Rules


# Variable name used in the DEPS file to add or subtract include files from
# the module-level deps.
INCLUDE_RULES_VAR_NAME = 'include_rules'

# Variable name used in the DEPS file to add or subtract include files
# from module-level deps specific to files whose basename (last
# component of path) matches a given regular expression.
SPECIFIC_INCLUDE_RULES_VAR_NAME = 'specific_include_rules'

# Optionally present in the DEPS file to list subdirectories which should not
# be checked. This allows us to skip third party code, for example.
SKIP_SUBDIRS_VAR_NAME = 'skip_child_includes'


def NormalizePath(path):
  """Returns a path normalized to how we write DEPS rules and compare paths."""
  return os.path.normcase(path).replace(os.path.sep, posixpath.sep)


def _GitSourceDirectories(base_directory):
  """Returns set of normalized paths to subdirectories containing sources
  managed by git."""
  if not os.path.exists(os.path.join(base_directory, '.git')):
    return set()

  base_dir_norm = NormalizePath(base_directory)
  git_source_directories = set([base_dir_norm])

  git_cmd = 'git.bat' if os.name == 'nt' else 'git'
  git_ls_files_cmd = [git_cmd, 'ls-files']
  # FIXME: Use a context manager in Python 3.2+
  popen = subprocess.Popen(git_ls_files_cmd,
                           stdout=subprocess.PIPE,
                           bufsize=1,  # line buffering, since read by line
                           cwd=base_directory)
  try:
    try:
      for line in popen.stdout:
        dir_path = os.path.join(base_directory, os.path.dirname(line))
        dir_path_norm = NormalizePath(dir_path)
        # Add the directory as well as all the parent directories,
        # stopping once we reach an already-listed directory.
        while dir_path_norm not in git_source_directories:
          git_source_directories.add(dir_path_norm)
          dir_path_norm = posixpath.dirname(dir_path_norm)
    finally:
      popen.stdout.close()
  finally:
    popen.wait()

  return git_source_directories


class DepsBuilder(object):
  """Parses include_rules from DEPS files."""

  def __init__(self,
               base_directory=None,
               verbose=False,
               being_tested=False,
               ignore_temp_rules=False,
               ignore_specific_rules=False):
    """Creates a new DepsBuilder.

    Args:
      base_directory: local path to root of checkout, e.g. C:\chr\src.
      verbose: Set to True for debug output.
      being_tested: Set to True to ignore the DEPS file at tools/checkdeps/DEPS.
      ignore_temp_rules: Ignore rules that start with Rule.TEMP_ALLOW ("!").
    """
    base_directory = (base_directory or
                      os.path.join(os.path.dirname(__file__),
                      os.path.pardir, os.path.pardir))
    self.base_directory = os.path.abspath(base_directory)  # Local absolute path
    self.verbose = verbose
    self._under_test = being_tested
    self._ignore_temp_rules = ignore_temp_rules
    self._ignore_specific_rules = ignore_specific_rules

    # Set of normalized paths
    self.git_source_directories = _GitSourceDirectories(self.base_directory)

    # Map of normalized directory paths to rules to use for those
    # directories, or None for directories that should be skipped.
    # Normalized is: absolute, lowercase, / for separator.
    self.directory_rules = {}
    self._ApplyDirectoryRulesAndSkipSubdirs(Rules(), self.base_directory)

  def _ApplyRules(self, existing_rules, includes, specific_includes,
                  cur_dir_norm):
    """Applies the given include rules, returning the new rules.

    Args:
      existing_rules: A set of existing rules that will be combined.
      include: The list of rules from the "include_rules" section of DEPS.
      specific_includes: E.g. {'.*_unittest\.cc': ['+foo', '-blat']} rules
                         from the "specific_include_rules" section of DEPS.
      cur_dir_norm: The current directory, normalized path. We will create an
                    implicit rule that allows inclusion from this directory.

    Returns: A new set of rules combining the existing_rules with the other
             arguments.
    """
    rules = copy.deepcopy(existing_rules)

    # First apply the implicit "allow" rule for the current directory.
    base_dir_norm = NormalizePath(self.base_directory)
    if not cur_dir_norm.startswith(base_dir_norm):
      raise Exception(
          'Internal error: base directory is not at the beginning for\n'
          '  %s and base dir\n'
          '  %s' % (cur_dir_norm, base_dir_norm))
    relative_dir = posixpath.relpath(cur_dir_norm, base_dir_norm)

    # Make the help string a little more meaningful.
    source = relative_dir or 'top level'
    rules.AddRule('+' + relative_dir,
                  relative_dir,
                  'Default rule for ' + source)

    def ApplyOneRule(rule_str, dependee_regexp=None):
      """Deduces a sensible description for the rule being added, and
      adds the rule with its description to |rules|.

      If we are ignoring temporary rules, this function does nothing
      for rules beginning with the Rule.TEMP_ALLOW character.
      """
      if self._ignore_temp_rules and rule_str.startswith(Rule.TEMP_ALLOW):
        return

      rule_block_name = 'include_rules'
      if dependee_regexp:
        rule_block_name = 'specific_include_rules'
      if relative_dir:
        rule_description = relative_dir + "'s %s" % rule_block_name
      else:
        rule_description = 'the top level %s' % rule_block_name
      rules.AddRule(rule_str, relative_dir, rule_description, dependee_regexp)

    # Apply the additional explicit rules.
    for rule_str in includes:
      ApplyOneRule(rule_str)

    # Finally, apply the specific rules.
    if self._ignore_specific_rules:
      return rules

    for regexp, specific_rules in specific_includes.iteritems():
      for rule_str in specific_rules:
        ApplyOneRule(rule_str, regexp)

    return rules

  def _ApplyDirectoryRules(self, existing_rules, dir_path_local_abs):
    """Combines rules from the existing rules and the new directory.

    Any directory can contain a DEPS file. Top-level DEPS files can contain
    module dependencies which are used by gclient. We use these, along with
    additional include rules and implicit rules for the given directory, to
    come up with a combined set of rules to apply for the directory.

    Args:
      existing_rules: The rules for the parent directory. We'll add-on to these.
      dir_path_local_abs: The directory path that the DEPS file may live in (if
                          it exists). This will also be used to generate the
                          implicit rules. This is a local path.

    Returns: A 2-tuple of:
      (1) the combined set of rules to apply to the sub-tree,
      (2) a list of all subdirectories that should NOT be checked, as specified
          in the DEPS file (if any).
          Subdirectories are single words, hence no OS dependence.
    """
    dir_path_norm = NormalizePath(dir_path_local_abs)

    # Check for a .svn directory in this directory or that this directory is
    # contained in git source directories. This will tell us if it's a source
    # directory and should be checked.
    if not (os.path.exists(os.path.join(dir_path_local_abs, '.svn')) or
            dir_path_norm in self.git_source_directories):
      return None, []

    # Check the DEPS file in this directory.
    if self.verbose:
      print 'Applying rules from', dir_path_local_abs
    def FromImpl(*_):
      pass  # NOP function so "From" doesn't fail.

    def FileImpl(_):
      pass  # NOP function so "File" doesn't fail.

    class _VarImpl:
      def __init__(self, local_scope):
        self._local_scope = local_scope

      def Lookup(self, var_name):
        """Implements the Var syntax."""
        try:
          return self._local_scope['vars'][var_name]
        except KeyError:
          raise Exception('Var is not defined: %s' % var_name)

    local_scope = {}
    global_scope = {
      'File': FileImpl,
      'From': FromImpl,
      'Var': _VarImpl(local_scope).Lookup,
    }
    deps_file_path = os.path.join(dir_path_local_abs, 'DEPS')

    # The second conditional here is to disregard the
    # tools/checkdeps/DEPS file while running tests.  This DEPS file
    # has a skip_child_includes for 'testdata' which is necessary for
    # running production tests, since there are intentional DEPS
    # violations under the testdata directory.  On the other hand when
    # running tests, we absolutely need to verify the contents of that
    # directory to trigger those intended violations and see that they
    # are handled correctly.
    if os.path.isfile(deps_file_path) and not (
        self._under_test and
        os.path.basename(dir_path_local_abs) == 'checkdeps'):
      execfile(deps_file_path, global_scope, local_scope)
    elif self.verbose:
      print '  No deps file found in', dir_path_local_abs

    # Even if a DEPS file does not exist we still invoke ApplyRules
    # to apply the implicit "allow" rule for the current directory
    include_rules = local_scope.get(INCLUDE_RULES_VAR_NAME, [])
    specific_include_rules = local_scope.get(SPECIFIC_INCLUDE_RULES_VAR_NAME,
                                             {})
    skip_subdirs = local_scope.get(SKIP_SUBDIRS_VAR_NAME, [])

    return (self._ApplyRules(existing_rules, include_rules,
                             specific_include_rules, dir_path_norm),
            skip_subdirs)

  def _ApplyDirectoryRulesAndSkipSubdirs(self, parent_rules,
                                         dir_path_local_abs):
    """Given |parent_rules| and a subdirectory |dir_path_local_abs| of the
    directory that owns the |parent_rules|, add |dir_path_local_abs|'s rules to
    |self.directory_rules|, and add None entries for any of its
    subdirectories that should be skipped.
    """
    directory_rules, excluded_subdirs = self._ApplyDirectoryRules(
        parent_rules, dir_path_local_abs)
    dir_path_norm = NormalizePath(dir_path_local_abs)
    self.directory_rules[dir_path_norm] = directory_rules
    for subdir in excluded_subdirs:
      subdir_path_norm = posixpath.join(dir_path_norm, subdir)
      self.directory_rules[subdir_path_norm] = None

  def GetDirectoryRules(self, dir_path_local):
    """Returns a Rules object to use for the given directory, or None
    if the given directory should be skipped.

    Also modifies |self.directory_rules| to store the Rules.
    This takes care of first building rules for parent directories (up to
    |self.base_directory|) if needed, which may add rules for skipped
    subdirectories.

    Args:
      dir_path_local: A local path to the directory you want rules for.
        Can be relative and unnormalized.
    """
    if os.path.isabs(dir_path_local):
      dir_path_local_abs = dir_path_local
    else:
      dir_path_local_abs = os.path.join(self.base_directory, dir_path_local)
    dir_path_norm = NormalizePath(dir_path_local_abs)

    if dir_path_norm in self.directory_rules:
      return self.directory_rules[dir_path_norm]

    parent_dir_local_abs = os.path.dirname(dir_path_local_abs)
    parent_rules = self.GetDirectoryRules(parent_dir_local_abs)
    # We need to check for an entry for our dir_path again, since
    # GetDirectoryRules can modify entries for subdirectories, namely setting
    # to None if they should be skipped, via _ApplyDirectoryRulesAndSkipSubdirs.
    # For example, if dir_path == 'A/B/C' and A/B/DEPS specifies that the C
    # subdirectory be skipped, GetDirectoryRules('A/B') will fill in the entry
    # for 'A/B/C' as None.
    if dir_path_norm in self.directory_rules:
      return self.directory_rules[dir_path_norm]

    if parent_rules:
      self._ApplyDirectoryRulesAndSkipSubdirs(parent_rules, dir_path_local_abs)
    else:
      # If the parent directory should be skipped, then the current
      # directory should also be skipped.
      self.directory_rules[dir_path_norm] = None
    return self.directory_rules[dir_path_norm]

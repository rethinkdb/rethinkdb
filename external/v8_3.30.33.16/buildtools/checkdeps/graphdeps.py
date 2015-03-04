#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Dumps a graph of allowed and disallowed inter-module dependencies described
by the DEPS files in the source tree. Supports DOT and PNG as the output format.

Enables filtering and differential highlighting of parts of the graph based on
the specified criteria. This allows for a much easier visual analysis of the
dependencies, including answering questions such as "if a new source must
depend on modules A, B, and C, what valid options among the existing modules
are there to put it in."

See builddeps.py for a detailed description of the DEPS format.
"""

import os
import optparse
import pipes
import re
import sys

from builddeps import DepsBuilder
from rules import Rule


class DepsGrapher(DepsBuilder):
  """Parses include_rules from DEPS files and outputs a DOT graph of the
  allowed and disallowed dependencies between directories and specific file
  regexps. Can generate only a subgraph of the whole dependency graph
  corresponding to the provided inclusion and exclusion regexp filters.
  Also can highlight fanins and/or fanouts of certain nodes matching the
  provided regexp patterns.
  """

  def __init__(self,
               base_directory,
               verbose,
               being_tested,
               ignore_temp_rules,
               ignore_specific_rules,
               hide_disallowed_deps,
               out_file,
               out_format,
               layout_engine,
               unflatten_graph,
               incl,
               excl,
               hilite_fanins,
               hilite_fanouts):
    """Creates a new DepsGrapher.

    Args:
      base_directory: OS-compatible path to root of checkout, e.g. C:\chr\src.
      verbose: Set to true for debug output.
      being_tested: Set to true to ignore the DEPS file at tools/graphdeps/DEPS.
      ignore_temp_rules: Ignore rules that start with Rule.TEMP_ALLOW ("!").
      ignore_specific_rules: Ignore rules from specific_include_rules sections.
      hide_disallowed_deps: Hide disallowed dependencies from the output graph.
      out_file: Output file name.
      out_format: Output format (anything GraphViz dot's -T option supports).
      layout_engine: Layout engine for formats other than 'dot'
                     (anything that GraphViz dot's -K option supports).
      unflatten_graph: Try to reformat the output graph so it is narrower and
                       taller. Helps fight overly flat and wide graphs, but
                       sometimes produces a worse result.
      incl: Include only nodes matching this regexp; such nodes' fanin/fanout
            is also included.
      excl: Exclude nodes matching this regexp; such nodes' fanin/fanout is
            processed independently.
      hilite_fanins: Highlight fanins of nodes matching this regexp with a
                     different edge and node color.
      hilite_fanouts: Highlight fanouts of nodes matching this regexp with a
                      different edge and node color.
    """
    DepsBuilder.__init__(
        self,
        base_directory,
        verbose,
        being_tested,
        ignore_temp_rules,
        ignore_specific_rules)

    self.ignore_temp_rules = ignore_temp_rules
    self.ignore_specific_rules = ignore_specific_rules
    self.hide_disallowed_deps = hide_disallowed_deps
    self.out_file = out_file
    self.out_format = out_format
    self.layout_engine = layout_engine
    self.unflatten_graph = unflatten_graph
    self.incl = incl
    self.excl = excl
    self.hilite_fanins = hilite_fanins
    self.hilite_fanouts = hilite_fanouts

    self.deps = set()

  def DumpDependencies(self):
    """ Builds a dependency rule table and dumps the corresponding dependency
    graph to all requested formats."""
    self._BuildDepsGraph(self.base_directory)
    self._DumpDependencies()

  def _BuildDepsGraph(self, full_path):
    """Recursively traverses the source tree starting at the specified directory
    and builds a dependency graph representation in self.deps."""
    rel_path = os.path.relpath(full_path, self.base_directory)
    #if re.search(self.incl, rel_path) and not re.search(self.excl, rel_path):
    rules = self.GetDirectoryRules(full_path)
    if rules:
      deps = rules.AsDependencyTuples(
          include_general_rules=True,
          include_specific_rules=not self.ignore_specific_rules)
      self.deps.update(deps)

    for item in sorted(os.listdir(full_path)):
      next_full_path = os.path.join(full_path, item)
      if os.path.isdir(next_full_path):
        self._BuildDepsGraph(next_full_path)

  def _DumpDependencies(self):
    """Dumps the built dependency graph to the specified file with specified
    format."""
    if self.out_format == 'dot' and not self.layout_engine:
      if self.unflatten_graph:
        pipe = pipes.Template()
        pipe.append('unflatten -l 2 -c 3', '--')
        out = pipe.open(self.out_file, 'w')
      else:
        out = open(self.out_file, 'w')
    else:
      pipe = pipes.Template()
      if self.unflatten_graph:
        pipe.append('unflatten -l 2 -c 3', '--')
      dot_cmd = 'dot -T' + self.out_format
      if self.layout_engine:
        dot_cmd += ' -K' + self.layout_engine
      pipe.append(dot_cmd, '--')
      out = pipe.open(self.out_file, 'w')

    self._DumpDependenciesImpl(self.deps, out)
    out.close()

  def _DumpDependenciesImpl(self, deps, out):
    """Computes nodes' and edges' properties for the dependency graph |deps| and
    carries out the actual dumping to a file/pipe |out|."""
    deps_graph = dict()
    deps_srcs = set()

    # Pre-initialize the graph with src->(dst, allow) pairs.
    for (allow, src, dst) in deps:
      if allow == Rule.TEMP_ALLOW and self.ignore_temp_rules:
        continue

      deps_srcs.add(src)
      if src not in deps_graph:
        deps_graph[src] = []
      deps_graph[src].append((dst, allow))

      # Add all hierarchical parents too, in case some of them don't have their
      # own DEPS, and therefore are missing from the list of rules. Those will
      # be recursively populated with their parents' rules in the next block.
      parent_src = os.path.dirname(src)
      while parent_src:
        if parent_src not in deps_graph:
          deps_graph[parent_src] = []
        parent_src = os.path.dirname(parent_src)

    # For every node, propagate its rules down to all its children.
    deps_srcs = list(deps_srcs)
    deps_srcs.sort()
    for src in deps_srcs:
      parent_src = os.path.dirname(src)
      if parent_src:
        # We presort the list, so parents are guaranteed to precede children.
        assert parent_src in deps_graph,\
               "src: %s, parent_src: %s" % (src, parent_src)
        for (dst, allow) in deps_graph[parent_src]:
          # Check that this node does not explicitly override a rule from the
          # parent that we're about to add.
          if ((dst, Rule.ALLOW) not in deps_graph[src]) and \
             ((dst, Rule.TEMP_ALLOW) not in deps_graph[src]) and \
             ((dst, Rule.DISALLOW) not in deps_graph[src]):
            deps_graph[src].append((dst, allow))

    node_props = {}
    edges = []

    # 1) Populate a list of edge specifications in DOT format;
    # 2) Populate a list of computed raw node attributes to be output as node
    #    specifications in DOT format later on.
    # Edges and nodes are emphasized with color and line/border weight depending
    # on how many of incl/excl/hilite_fanins/hilite_fanouts filters they hit,
    # and in what way.
    for src in deps_graph.keys():
      for (dst, allow) in deps_graph[src]:
        if allow == Rule.DISALLOW and self.hide_disallowed_deps:
          continue

        if allow == Rule.ALLOW and src == dst:
          continue

        edge_spec = "%s->%s" % (src, dst)
        if not re.search(self.incl, edge_spec) or \
               re.search(self.excl, edge_spec):
          continue

        if src not in node_props:
          node_props[src] = {'hilite': None, 'degree': 0}
        if dst not in node_props:
          node_props[dst] = {'hilite': None, 'degree': 0}

        edge_weight = 1

        if self.hilite_fanouts and re.search(self.hilite_fanouts, src):
          node_props[src]['hilite'] = 'lightgreen'
          node_props[dst]['hilite'] = 'lightblue'
          node_props[dst]['degree'] += 1
          edge_weight += 1

        if self.hilite_fanins and re.search(self.hilite_fanins, dst):
          node_props[src]['hilite'] = 'lightblue'
          node_props[dst]['hilite'] = 'lightgreen'
          node_props[src]['degree'] += 1
          edge_weight += 1

        if allow == Rule.ALLOW:
          edge_color = (edge_weight > 1) and 'blue' or 'green'
          edge_style = 'solid'
        elif allow == Rule.TEMP_ALLOW:
          edge_color = (edge_weight > 1) and 'blue' or 'green'
          edge_style = 'dashed'
        else:
          edge_color = 'red'
          edge_style = 'dashed'
        edges.append('    "%s" -> "%s" [style=%s,color=%s,penwidth=%d];' % \
            (src, dst, edge_style, edge_color, edge_weight))

    # Reformat the computed raw node attributes into a final DOT representation.
    nodes = []
    for (node, attrs) in node_props.iteritems():
      attr_strs = []
      if attrs['hilite']:
        attr_strs.append('style=filled,fillcolor=%s' % attrs['hilite'])
      attr_strs.append('penwidth=%d' % (attrs['degree'] or 1))
      nodes.append('    "%s" [%s];' % (node, ','.join(attr_strs)))

    # Output nodes and edges to |out| (can be a file or a pipe).
    edges.sort()
    nodes.sort()
    out.write('digraph DEPS {\n'
              '    fontsize=8;\n')
    out.write('\n'.join(nodes))
    out.write('\n\n')
    out.write('\n'.join(edges))
    out.write('\n}\n')
    out.close()


def PrintUsage():
  print """Usage: python graphdeps.py [--root <root>]

  --root ROOT Specifies the repository root. This defaults to "../../.."
              relative to the script file. This will be correct given the
              normal location of the script in "<root>/tools/graphdeps".

  --(others)  There are a few lesser-used options; run with --help to show them.

Examples:
  Dump the whole dependency graph:
    graphdeps.py
  Find a suitable place for a new source that must depend on /apps and
  /content/browser/renderer_host. Limit potential candidates to /apps,
  /chrome/browser and content/browser, and descendants of those three.
  Generate both DOT and PNG output. The output will highlight the fanins
  of /apps and /content/browser/renderer_host. Overlapping nodes in both fanins
  will be emphasized by a thicker border. Those nodes are the ones that are
  allowed to depend on both targets, therefore they are all legal candidates
  to place the new source in:
    graphdeps.py \
      --root=./src \
      --out=./DEPS.svg \
      --format=svg \
      --incl='^(apps|chrome/browser|content/browser)->.*' \
      --excl='.*->third_party' \
      --fanin='^(apps|content/browser/renderer_host)$' \
      --ignore-specific-rules \
      --ignore-temp-rules"""


def main():
  option_parser = optparse.OptionParser()
  option_parser.add_option(
      "", "--root",
      default="", dest="base_directory",
      help="Specifies the repository root. This defaults "
           "to '../../..' relative to the script file, which "
           "will normally be the repository root.")
  option_parser.add_option(
      "-f", "--format",
      dest="out_format", default="dot",
      help="Output file format. "
           "Can be anything that GraphViz dot's -T option supports. "
           "The most useful ones are: dot (text), svg (image), pdf (image)."
           "NOTES: dotty has a known problem with fonts when displaying DOT "
           "files on Ubuntu - if labels are unreadable, try other formats.")
  option_parser.add_option(
      "-o", "--out",
      dest="out_file", default="DEPS",
      help="Output file name. If the name does not end in an extension "
           "matching the output format, that extension is automatically "
           "appended.")
  option_parser.add_option(
      "-l", "--layout-engine",
      dest="layout_engine", default="",
      help="Layout rendering engine. "
           "Can be anything that GraphViz dot's -K option supports. "
           "The most useful are in decreasing order: dot, fdp, circo, osage. "
           "NOTE: '-f dot' and '-f dot -l dot' are different: the former "
           "will dump a raw DOT graph and stop; the latter will further "
           "filter it through 'dot -Tdot -Kdot' layout engine.")
  option_parser.add_option(
      "-i", "--incl",
      default="^.*$", dest="incl",
      help="Include only edges of the graph that match the specified regexp. "
           "The regexp is applied to edges of the graph formatted as "
           "'source_node->target_node', where the '->' part is vebatim. "
           "Therefore, a reliable regexp should look like "
           "'^(chrome|chrome/browser|chrome/common)->content/public/browser$' "
           "or similar, with both source and target node regexps present, "
           "explicit ^ and $, and otherwise being as specific as possible.")
  option_parser.add_option(
      "-e", "--excl",
      default="^$", dest="excl",
      help="Exclude dependent nodes that match the specified regexp. "
           "See --incl for details on the format.")
  option_parser.add_option(
      "", "--fanin",
      default="", dest="hilite_fanins",
      help="Highlight fanins of nodes matching the specified regexp.")
  option_parser.add_option(
      "", "--fanout",
      default="", dest="hilite_fanouts",
      help="Highlight fanouts of nodes matching the specified regexp.")
  option_parser.add_option(
      "", "--ignore-temp-rules",
      action="store_true", dest="ignore_temp_rules", default=False,
      help="Ignore !-prefixed (temporary) rules in DEPS files.")
  option_parser.add_option(
      "", "--ignore-specific-rules",
      action="store_true", dest="ignore_specific_rules", default=False,
      help="Ignore specific_include_rules section of DEPS files.")
  option_parser.add_option(
      "", "--hide-disallowed-deps",
      action="store_true", dest="hide_disallowed_deps", default=False,
      help="Hide disallowed dependencies in the output graph.")
  option_parser.add_option(
      "", "--unflatten",
      action="store_true", dest="unflatten_graph", default=False,
      help="Try to reformat the output graph so it is narrower and taller. "
           "Helps fight overly flat and wide graphs, but sometimes produces "
           "inferior results.")
  option_parser.add_option(
      "-v", "--verbose",
      action="store_true", default=False,
      help="Print debug logging")
  options, args = option_parser.parse_args()

  if not options.out_file.endswith(options.out_format):
    options.out_file += '.' + options.out_format

  deps_grapher = DepsGrapher(
      base_directory=options.base_directory,
      verbose=options.verbose,
      being_tested=False,

      ignore_temp_rules=options.ignore_temp_rules,
      ignore_specific_rules=options.ignore_specific_rules,
      hide_disallowed_deps=options.hide_disallowed_deps,

      out_file=options.out_file,
      out_format=options.out_format,
      layout_engine=options.layout_engine,
      unflatten_graph=options.unflatten_graph,

      incl=options.incl,
      excl=options.excl,
      hilite_fanins=options.hilite_fanins,
      hilite_fanouts=options.hilite_fanouts)

  if len(args) > 0:
    PrintUsage()
    return 1

  print 'Using base directory: ', deps_grapher.base_directory
  print 'include nodes       : ', options.incl
  print 'exclude nodes       : ', options.excl
  print 'highlight fanins of : ', options.hilite_fanins
  print 'highlight fanouts of: ', options.hilite_fanouts

  deps_grapher.DumpDependencies()
  return 0


if '__main__' == __name__:
  sys.exit(main())

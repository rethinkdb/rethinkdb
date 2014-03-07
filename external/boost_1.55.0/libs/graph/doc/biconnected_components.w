\documentclass[11pt]{report}


\usepackage[leqno]{amsmath}
\usepackage{amsfonts}
\usepackage{amssymb}
\usepackage{amsthm}
\usepackage{latexsym}     
\usepackage{jweb}
\usepackage{times}
\usepackage{graphicx}
\usepackage[nolineno]{lgrind}

\newif\ifpdf
\ifx\pdfoutput\undefined
   \pdffalse
\else
   \pdfoutput=1
   \pdftrue
\fi

\ifpdf
  \usepackage[
              pdftex,
              colorlinks=true, % change to true for the electronic version
              linkcolor=blue,filecolor=blue,pagecolor=blue,urlcolor=blue
              ]{hyperref}
  \newcommand{\myhyperref}[2]{\hyperref[#1]{#2}}
\fi

\newcommand{\mtlfig}[2]{\centerline{\includegraphics*[#2]{#1.pdf}}}

\newcommand{\Path}{\rightsquigarrow}
\newcommand{\ancestor}{\overset{T}{\rightsquigarrow}}
\newcommand{\descendant}{\ancestor^{-1}}
\newcommand{\backedge}{\overset{B}{\rightarrow}}
\newcommand{\edge}{\rightarrow}
\DeclareMathOperator{\suchthat}{s.t.}

\newcommand{\code}[1]{{\small{\em \textbf{#1}}}}
\newcommand{\concept}[1]{\textsf{#1}}

\begin{document}

\title{An Implementation of Biconnected Components}
\author{Jeremy Siek}

\maketitle

\section{Introduction}

This paper documents the implementation of the
\code{biconnected\_components()} function of the Boost Graph
Library. The function was implemented by Jeremy Siek.

The algorithm used to implement the \code{biconnected\_components()}
function is the one based on depth-first search described
by Tarjan~\cite{tarjan72:dfs_and_linear_algo}.

An undirected graph $G = (V,E)$ is \emph{biconnected} if for each
triple of distinct vertices $v, w, a \in V$ there is a path $p : v
\Path w$ such that $a$ is not on the path $p$.  An \emph{articulation
point} of $G = (V,E)$ is a vertex $a \in V$ where there are two other
distinct vertices $v,w \in V$ such that $a$ is on any path $p:v \Path
w$ and there is at least one such path. If $a$ were to be removed from
$G$, then $v$ and $w$ would no longer be reachable from each other.
So articulation points act as bridges between biconnected components;
the only path from one biconnected component to another is through an
articulation point.

The algorithm finds articulation points based on information provided
by depth-first search. During a DFS, we label each vertex $v \in G$
with its discover time, denoted $d[v]$.  During the DFS we also
compute the $lowpt(v)$, which is the smallest (in terms of discover
time) vertex reachable from $v$ by traversing zero or more DFS-tree
edges followed by at most one back edge. Tree edges and back edges can
be identified based on discover time because for tree edge $(u,v)$ we
have $d[u] < d[v]$ and for back edge $(u,v)$ we have $d[u] >
d[v]$. The $lowpt(v)$ is computed for $v$ by taking the minimum
$lowpt$ of the vertices to which $v$ is adjacent.  The $lowpt(v)$ is
computed after the recursive DFS call so the $lowpt$ has already been
computed for the adjacent vertices by the time $lowpt(v)$ is computed.

Now it turns out that $lowpt$ can be used to identify articulation
points. Suppose $a,v,w$ are distinct vertices in $G$ such that $(a,v)$
is a tree edge and $w$ is not a descendant of $v$. If $d[lowpt(v)]
\geq d[a]$, then $a$ is an articulation point and removal of $a$
disconnects $v$ and $w$. The reason this works is that if $d[lowpt(v)]
\geq d[a]$, then we know all paths starting from $v$ stay within the
sub-tree $T_v$ rooted at $v$. If a path were to escape from the
sub-tree, then consider the first vertex $w$ in that path outside of
$T_v$.  $v \Path w$ must be considered for $lowpt(v)$, so $d[lowpt(v)]
< d[w]$. Now $d[w] < d[a]$ due the structure of the DFS, so
transitively $d[lowpt(v)] < d[a]$.

\section{The Implementation}

The implementation consists of a recursive DFS-like function named
\code{biconnect()} and the public interface function
\code{biconnected\-\_components()}. The \code{Graph} type must be a
model of \concept{VertexListGraph} and of \concept{IncidenceGraph}.
The result of the algorithm is recorded in the \code{ComponentMap},
which maps edges to the biconnected component that they belong to
(components are labeled with integers from zero on up).  The
\code{ComponentMap} type must be a model of
\concept{WritablePropertyMap}, which the graph's
\code{edge\-\_descriptor} type as the key type and an unsigned integer
type as the value type. We do not record which component each vertex
belongs to because vertices that are articulation points belong to
multiple biconnected components.  The number of biconnected components
is returned in the \code{num\_components} parameter. The
\code{discover\_time} parameter is used internally to keep track of
the DFS discover time for each vertex. It must be a
\concept{ReadWritePropertyMap} with the graph's
\code{vertex\_\-descriptor} type as the key type and an unsigned
integer as the value type. The \code{lowpt} parameter is used
internally to keep track of the $d[lowpt(v)]$ for each vertex $v$.  It
must be a \concept{ReadWritePropertyMap} with the graph's
\code{vertex\_\-descriptor} type is the key type and the value type is
the same unsigned integer type as the value type in the
\code{discover\-\_time} map.

@d Biconnected Components Algorithm
@{
namespace detail {
  @<Recursive Biconnect Function@>
}

template <typename Graph, typename ComponentMap,
  typename DiscoverTimeMap, typename LowPointMap>
void biconnected_components
  (typename graph_traits<Graph>::vertex_descriptor v,
   typename graph_traits<Graph>::vertex_descriptor u,
   const Graph& g,
   ComponentMap comp,
   std::size_t& num_components,
   DiscoverTimeMap discover_time,
   LowPointMap lowpt)
{
  typedef graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef graph_traits<Graph>::edge_descriptor edge_t;
  @<Concept checking of type parameters@>
  typedef typename property_traits<DiscoverTimeMap>::value_type D;
  num_components = 0;
  std::size_t dfs_time = 0;
  std::stack<edge_t> S;
  @<Initialize discover times to infinity@>
  @<Process each connected component@>
}
@}

\noindent In the following code we use the Boost Concept Checking
Library to provide better error messages in the event that the user
makes a mistake in the kind of parameter used in the function
template.

@d Concept checking of type parameters
@{
BOOST_CONCEPT_ASSERT(( VertexListGraphConcept<Graph> ));
BOOST_CONCEPT_ASSERT(( IncidenceGraphConcept<Graph> ));
BOOST_CONCEPT_ASSERT(( WritablePropertyMapConcept<ComponentMap, edge_t> ));
BOOST_CONCEPT_ASSERT(( ReadWritePropertyMapConcept<DiscoverTimeMap, vertex_t> ));
BOOST_CONCEPT_ASSERT(( ReadWritePropertyMapConcept<LowPointMap, vertex_t> ));
@}

The first step of the algorithm is to initialize the discover times of
all the vertices to infinity. This marks the vertices as undiscovered.

@d Initialize discover times to infinity
@{
typename graph_traits<Graph>::vertex_iterator wi, wi_end;
std::size_t infinity = std::numeric_limits<std::size_t>::max();
for (tie(wi, wi_end) = vertices(g); wi != wi_end; ++wi)
  put(discover_time, *wi, infinity);
@}

\noindent Next we invoke \code{biconnect()} on every vertex in the
graph, making sure that all connected components within the graph are
searched (\code{biconnect()} only processes a single connected
component).

@d Process each connected component
@{
for (tie(wi, wi_end) = vertices(g); wi != wi_end; ++wi)
  if (get(discover_time, *wi) == std::numeric_limits<D>::max())
    detail::biconnect(*wi, *wi, true,
                      g, comp, num_components,
                      discover_time, dfs_time, lowpt, S);
@}

The recursive \code{biconnect()} function is shown below.  The
\code{v} parameter is where the DFS is started.  The \code{u}
parameter is the parent of \code{v} in the DFS-tree if \code{at\_top
== false} or if \code{at\_top == true} the \code{u} is not used.
\code{S} is a stack of edges that is used to collect all edges in a
biconnected component. The way this works is that on ``the way down''
edges are pushed into the stack.  ``On the way back up'', when an
articulation point $v$ is found (identified because $d[lowpt(w)] \geq
d[v]$) we know that a contiguous portion of the stack (starting at the
top) contains the edges in the sub-tree $T_v$ which is the biconnected
component.  We therefore pop these edges off of the stack (until we
find an edge $e$ where $d[lowpt(source(e))] < d[w]$) and mark them as
belonging to the same component. The code below also includes the
bookkeeping details such as recording the discover times and computing
$lowpt$. When a back edge $(v,w)$ is encountered, we do not use
$lowpt(w)$ in calculating $lowpt(v)$ since $lowpt(w)$ has not yet been
computed. Also, we ignore the edge $(v,w)$ if $w$ is the parent of $v$
in the DFS-tree, meaning that $(w,v)$ has already been examined and
categorized as a tree edge (not a back edge).

@d Recursive Biconnect Function
@{
template <typename Graph, typename ComponentMap,
  typename DiscoverTimeMap, typename LowPointMap, typename Stack>
void biconnect(typename graph_traits<Graph>::vertex_descriptor v,
              typename graph_traits<Graph>::vertex_descriptor u,
              bool at_top,
              const Graph& g,
              ComponentMap comp,
              std::size_t& c,
              DiscoverTimeMap d,
              std::size_t& dfs_time,
              LowPointMap lowpt,
              Stack& S)
{
  typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;
  typedef typename property_traits<DiscoverTimeMap>::value_type D;
  D infinity = std::numeric_limits<D>::max();
  put(d, v, ++dfs_time);
  put(lowpt, v, get(d, v));
  typename graph_traits<Graph>::out_edge_iterator ei, ei_end;
  for (tie(ei, ei_end) = out_edges(v, g); ei != ei_end; ++ei) {
    vertex_t w = target(*ei, g);
    if (get(d, w) == infinity) {
      S.push(*ei);
      biconnect(w, v, false, g, comp, c, d, dfs_time, lowpt, S);
      put(lowpt, v, std::min(get(lowpt, v), get(lowpt, w)));
      if (get(lowpt, w) >= get(d, v)) {
        @<Record the biconnected component@>
      }
    } else if (get(d, w) < get(d, v) && (!at_top && w != u)) {
      S.push(*ei);
      put(lowpt, v, std::min(get(lowpt, v), get(d, w)));
    }
  }
}
@}

\noindent The following is the code for popping the edges of sub-tree
$T_v$ off of the stack and recording them as being in the same
biconnected component.

@d Record the biconnected component
@{
while (d[source(S.top(), g)] >= d[w]) {
  put(comp, S.top(), c);
  S.pop();
}
put(comp, S.top(), c);
S.pop();
++c;
@}


\section{Appendix}

@o biconnected-components.hpp
@{
// Copyright (c) Jeremy Siek 2001
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appears in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  Silicon Graphics makes no
// representations about the suitability of this software for any
// purpose.  It is provided "as is" without express or implied warranty.

// NOTE: this final is generated by libs/graph/doc/biconnected_components.w

#ifndef BOOST_GRAPH_BICONNECTED_COMPONENTS_HPP
#define BOOST_GRAPH_BICONNECTED_COMPONENTS_HPP

#include <stack>
#include <boost/limits.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/concept/assert.hpp>

namespace boost {
  @<Biconnected Components Algorithm@>
} // namespace boost

#endif BOOST_GRAPH_BICONNECTED_COMPONENTS_HPP
@}

Figure~\ref{fig:bcc} shows the graph used in the following example and
the edges are labeled by biconnected component number, as computed by
the algorithm.


\begin{figure}[htbp]
  \mtlfig{bcc}{width=3.0in}
  \caption{The biconnected components.}
  \label{fig:bcc}
\end{figure}


@o biconnected-components.cpp
@{
#include <vector>
#include <list>
#include <boost/graph/biconnected_components.hpp>
#include <boost/graph/adjacency_list.hpp>

namespace boost {
  struct edge_component_t {
    enum { num = 555 };
    typedef edge_property_tag kind;
  } edge_component;
}

int main()
{
  using namespace boost;
  typedef adjacency_list<vecS, vecS, undirectedS,
    no_property, property<edge_component_t, std::size_t> > graph_t;
  typedef graph_traits<graph_t>::vertex_descriptor vertex_t;
  graph_t g(9);
  add_edge(0, 5, g); add_edge(0, 1, g); add_edge(0, 6, g);
  add_edge(1, 2, g); add_edge(1, 3, g); add_edge(1, 4, g);
  add_edge(2, 3, g);
  add_edge(4, 5, g);
  add_edge(6, 8, g); add_edge(6, 7, g);
  add_edge(7, 8, g);

  std::size_t c = 0;
  std::vector<std::size_t> discover_time(num_vertices(g));
  std::vector<vertex_t> lowpt(num_vertices(g));
  property_map<graph_t, edge_component_t>::type
    component = get(edge_component, g);
  biconnected_components(0, 8, g, component,
                         c, &discover_time[0], &lowpt[0]);

  std::cout << "graph A {\n"
            << "  node[shape=\"circle\"]\n";

  graph_traits<graph_t>::edge_iterator ei, ei_end;
  for (tie(ei, ei_end) = edges(g); ei != ei_end; ++ei)
    std::cout << source(*ei, g) << " -- " << target(*ei, g)
              << "[label=\"" << component[*ei] << "\"]\n";
  std::cout << "}\n";

  return 0;
}
@}



% \paragraph{Definition.} A \emph{palm tree} $P$ is a directed graph that 
% consists of two disjoint sets of edges, denoted by $v \rightarrow w$
% and $v \backedge w$ respectively, with the following properties:

% \begin{enumerate}

% \item The subgraph $T$ containing the edges $v \rightarrow w$ is a
%   spanning tree of $P$.

% \item $\backedge \; \subseteq \descendant$. That is, each edge of $P$
% that is not in $T$ connects a vertex to one of its ancestors in $T$.
% \end{enumerate}



\bibliographystyle{abbrv}
\bibliography{jtran,ggcl,optimization,generic-programming,cad}

\end{document}

% LocalWords:  Biconnected Siek biconnected Tarjan undirected DFS lowpt num dfs
% LocalWords:  biconnect VertexListGraph IncidenceGraph ComponentMap namespace
% LocalWords:  WritablePropertyMap ReadWritePropertyMap typename LowPointMap wi
% LocalWords:  DiscoverTimeMap const comp typedef VertexListGraphConcept max ei
% LocalWords:  IncidenceGraphConcept WritablePropertyMapConcept iterator bool
% LocalWords:  ReadWritePropertyMapConcept hpp ifndef endif bcc cpp struct enum
% LocalWords:  adjacency vecS undirectedS jtran ggcl

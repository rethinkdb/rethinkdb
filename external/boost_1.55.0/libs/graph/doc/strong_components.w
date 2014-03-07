\documentclass[11pt]{report}

\input{defs}


\setlength\overfullrule{5pt}
\tolerance=10000
\sloppy
\hfuzz=10pt

\makeindex

\begin{document}

\title{A Generic Programming Implementation of Strongly Connected Components}
\author{Jeremy G. Siek}

\maketitle

\section{Introduction}

This paper describes the implementation of the
\code{strong\_components()} function of the Boost Graph Library.  The
function computes the strongly connects components of a directed graph
using Tarjan's DFS-based
algorithm~\cite{tarjan72:dfs_and_linear_algo}.

A \keyword{strongly connected component} (SCC) of a directed graph
$G=(V,E)$ is a maximal set of vertices $U$ which is in $V$ such that
for every pair of vertices $u$ and $v$ in $U$, we have both a path
from $u$ to $v$ and path from $v$ to $u$. That is to say that $u$ and
$v$ are reachable from each other.

cross edge (u,v) is an edge from one subtree to another subtree
 -> discover_time[u] > discover_time[v]

Lemma 10.  Let $v$ and $w$ be vertices in $G$ which are in the same
SCC and let $F$ be any depth-first forest of $G$. Then $v$ and $w$
have a common ancestor in $F$. Also, if $u$ is the common ancestor of
$u$ and $v$ with the latest discover time then $w$ is also in the same
SCC as $u$ and $v$.

Proof. 

If there is a path from $v$ to $w$ and if they are in different DFS
trees, then the discover time for $w$ must be earlier than for $v$.
Otherwise, the tree that contains $v$ would have extended along the
path to $w$, putting $v$ and $w$ in the same tree.


The following is an informal description of Tarjan's algorithm for
computing strongly connected components. It is basically a variation
on depth-first search, with extra actions being taken at the
``discover vertex'' and ``finish vertex'' event points. It may help to
think of the actions taken at the ``discover vertex'' event point as
occuring ``on the way down'' a DFS-tree (from the root towards the
leaves), and actions taken a the ``finish vertex'' event point as
occuring ``on the way back up''.

There are three things that need to happen on the way down. For each
vertex $u$ visited we record the discover time $d[u]$, push vertex $u$
onto a auxiliary stack, and set $root[u] = u$.  The root field will
end up mapping each vertex to the topmost vertex in the same strongly
connected component.  By setting $root[u] = u$ we are starting with
each vertex in a component by itself.

Now to describe what happens on the way back up. Suppose we have just
finished visiting all of the vertices adjacent to some vertex $u$.  We
then scan each of the adjacent vertices again, checking the root of
each for which one has the earliest discover time, which we will call
root $a$. We then compare $a$ with vertex $u$ and consider the
following cases:

\begin{enumerate}
\item If $d[a] < d[u]$ then we know that $a$ is really an ancestor of
  $u$ in the DFS tree and therefore we have a cycle and $u$ must be in
  a SCC with $a$. We then set $root[u] = a$ and continue our way back up
  the DFS.
  
\item If $a = u$ then we know that $u$ must be the topmost vertex of a
  subtree that defines a SCC.  All of the vertices in this subtree are
  further down on the stack than vertex $u$ so we pop the vertices off
  of the stack until we reach $u$ and mark each one as being in the
  same component.
  
\item If $d[a] > d[u]$ then the adjacent vertices are in different
  strongly connected components. We continue our way back up the
  DFS.

\end{enumerate}



@d Build a list of vertices for each strongly connected component
@{
template <typename Graph, typename ComponentMap, typename ComponentLists>
void build_component_lists
  (const Graph& g,
   typename graph_traits<Graph>::vertices_size_type num_scc,
   ComponentMap component_number,
   ComponentLists& components)
{
  components.resize(num_scc);
  typename graph_traits<Graph>::vertex_iterator vi, vi_end;
  for (tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    components[component_number[*vi]].push_back(*vi);
}
@}


\bibliographystyle{abbrv}
\bibliography{jtran,ggcl,optimization,generic-programming,cad}

\end{document}

\documentclass[11pt]{report}

%\input{defs}
\usepackage{math}
\usepackage{jweb}
\usepackage{lgrind}
\usepackage{times}
\usepackage{fullpage}
\usepackage{graphicx}

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
              colorlinks=true, %change to true for the electronic version
              linkcolor=blue,filecolor=blue,pagecolor=blue,urlcolor=blue
              ]{hyperref}
\fi

\ifpdf
  \newcommand{\stlconcept}[1]{\href{http://www.sgi.com/tech/stl/#1.html}{{\small \textsf{#1}}}}
  \newcommand{\bglconcept}[1]{\href{http://www.boost.org/libs/graph/doc/#1.html}{{\small \textsf{#1}}}}
  \newcommand{\pmconcept}[1]{\href{http://www.boost.org/libs/property_map/#1.html}{{\small \textsf{#1}}}}
  \newcommand{\myhyperref}[2]{\hyperref[#1]{#2}}
  \newcommand{\vizfig}[2]{\begin{figure}[htbp]\centerline{\includegraphics*{#1.pdf}}\caption{#2}\label{fig:#1}\end{figure}}
\else
  \newcommand{\myhyperref}[2]{#2}
  \newcommand{\bglconcept}[1]{{\small \textsf{#1}}}
  \newcommand{\pmconcept}[1]{{\small \textsf{#1}}}
  \newcommand{\stlconcept}[1]{{\small \textsf{#1}}}
  \newcommand{\vizfig}[2]{\begin{figure}[htbp]\centerline{\includegraphics*{#1.eps}}\caption{#2}\label{fig:#1}\end{figure}}
\fi

\newcommand{\code}[1]{{\small{\em \textbf{#1}}}}


\newcommand{\isomorphic}{\cong}

\begin{document}

\title{An Implementation of Isomorphism Testing}
\author{Jeremy G. Siek}

\maketitle

\section{Introduction}

This paper documents the implementation of the \code{isomorphism()}
function of the Boost Graph Library.  The implementation was by Jeremy
Siek with algorithmic improvements and test code from Douglas Gregor
and Brian Osman.  The \code{isomorphism()} function answers the
question, ``are these two graphs equal?''  By \emph{equal} we mean
the two graphs have the same structure---the vertices and edges are
connected in the same way. The mathematical name for this kind of
equality is \emph{isomorphism}.

More precisely, an \emph{isomorphism} is a one-to-one mapping of the
vertices in one graph to the vertices of another graph such that
adjacency is preserved. Another words, given graphs $G_{1} =
(V_{1},E_{1})$ and $G_{2} = (V_{2},E_{2})$, an isomorphism is a
function $f$ such that for all pairs of vertices $a,b$ in $V_{1}$,
edge $(a,b)$ is in $E_{1}$ if and only if edge $(f(a),f(b))$ is in
$E_{2}$.

Both graphs must be the same size, so let $N = |V_1| = |V_2|$. The
graph $G_1$ is \emph{isomorphic} to $G_2$ if an isomorphism exists
between the two graphs, which we denote by $G_1 \isomorphic G_2$.

In the following discussion we will need to use several notions from
graph theory. The graph $G_s=(V_s,E_s)$ is a \emph{subgraph} of graph
$G=(V,E)$ if $V_s \subseteq V$ and $E_s \subseteq E$.  An
\emph{induced subgraph}, denoted by $G[V_s]$, of a graph $G=(V,E)$
consists of the vertices in $V_s$, which is a subset of $V$, and every
edge $(u,v)$ in $E$ such that both $u$ and $v$ are in $V_s$.  We use
the notation $E[V_s]$ to mean the edges in $G[V_s]$.

In some places we express a function as a set of pairs, so the set $f
= \{ \pair{a_1}{b_1}, \ldots, \pair{a_n}{b_n} \}$
means $f(a_i) = b_i$ for $i=1,\ldots,n$.

\section{Exhaustive Backtracking Search}
\label{sec:backtracking}

The algorithm used by the \code{isomorphism()} function is, at
first approximation, an exhaustive search implemented via
backtracking.  The backtracking algorithm is a recursive function. At
each stage we will try to extend the match that we have found so far.
So suppose that we have already determined that some subgraph of $G_1$
is isomorphic to a subgraph of $G_2$.  We then try to add a vertex to
each subgraph such that the new subgraphs are still isomorphic to one
another. At some point we may hit a dead end---there are no vertices
that can be added to extend the isomorphic subgraphs. We then
backtrack to previous smaller matching subgraphs, and try extending
with a different vertex choice. The process ends by either finding a
complete mapping between $G_1$ and $G_2$ and return true, or by
exhausting all possibilities and returning false.

We consider the vertices of $G_1$ for addition to the matched subgraph
in a specific order, so assume that the vertices of $G_1$ are labeled
$1,\ldots,N$ according to that order. As we will see later, a good
ordering of the vertices is by DFS discover time.  Let $G_1[k]$ denote
the subgraph of $G_1$ induced by the first $k$ vertices, with $G_1[0]$
being an empty graph. We also consider the edges of $G_1$ in a
specific order. We always examine edges in the current subgraph
$G_1[k]$ first, that is, edges $(u,v)$ where both $u \leq k$ and $v
\leq k$. This ordering of edges can be acheived by sorting the edges
according to number of the larger of the source and target vertex.

Each step of the backtracking search examines an edge $(u,v)$ of $G_1$
and decides whether to continue or go back. There are three cases to
consider:

\begin{enumerate}

\item $i \leq k \Land j \leq k$. Both $i$ and $j$ are in $G_1[k]$.  We
check to make sure the $(f(i),f(j)) \in E_2[S]$.

\item $i \leq k \Land j > k$. $i$ is in the matched subgraph $G_1[k]$,
but $j$ is not. We are about to increment $k$ try to grow the matched
subgraph to include $j$. However, first we need to finalize our check
of the isomorphism between subgraphs $G_1[k]$ and $G_2[S]$.  At this
point we are guaranteed to have seen all the edges to and from vertex $k$
(because the edges are sorted), and in previous steps we have checked
that for each edge incident on $k$ in $G_1[k]$ there is a matching
edge in $G_2[S]$.  However we have not checked that for each edge
incident on $f(k)$ in $E_2[S]$, there is a matching edge in $E_1[k]$
(we need to check the ``only if'' part of the ``if and only if'').
Therefore we scan through all the edges $(u,v)$ incident on $f(k)$ and
make sure that $(f^{-1}(u),f^{-1}(v)) \in E_1[k]$. Once this check has
been performed, we add $f(k)$ to $S$, we increment $k$ (so now $k=j$),
and then try assigning the new $k$ to any of the eligible vertices in
$V_2 - S$. More about what ``eligible'' means later.

\item $i > k \Land j \leq k$. This case will not occur due to the DFS
numbering of the vertices. There is an edge $(i,j)$ so $i$ must be
less than $j$.

\item $i > k \Land j > k$. Neither $i$ or $j$ is in the matched
subgraph $G_1[k]$. This situation only happens at the very beginning
of the search, or when $i$ and $j$ are not reachable from any of the
vertices in $G_1[k]$. This means the smaller of $i$ and $j$ must be
the root of a DFS tree. We assign $r$ to any of the eligible vertices
in $V_2 - S$, and then proceed as if we were in Case 2.

\end{enumerate}



@d Match function
@{
bool match(edge_iter iter)
{
if (iter != ordered_edges.end()) {
    ordered_edge edge = *iter;
    size_type k_num = edge.k_num;
    vertex1_t k = dfs_vertices[k_num];
    vertex1_t u;
    if (edge.source != -1) // might be a ficticious edge
        u = dfs_vertices[edge.source];
    vertex1_t v = dfs_vertices[edge.target];
    if (edge.source == -1) { // root node
        @<$v$ is a DFS tree root@>
    } else if (f_assigned[v] == false) {
        @<$v$ is an unmatched vertex, $(u,v)$ is a tree edge@>
    } else {
        @<Check to see if there is an edge in $G_2$ to match $(u,v)$@>
    }
} else 
    return true;
return false;
} // match()
@}






The basic idea will be to examine $G_1$ one edge at a time, trying to
create a vertex mapping such that each edge matches one in $G_2$.  We
are going to consider the edges of $G_1$ in a specific order, so we
will label the edges $0,\ldots,|E_1|-1$.

At each stage of the recursion we
start with an isomorphism $f_{k-1}$ between $G_1[k-1]$ and a subgraph
of $G_2$, which we denote by $G_2[S]$, so $G_1[k-1] \isomorphic
G_2[S]$. The vertex set $S$ is the subset of $V_2$ that corresponds
via $f_{k-1}$ to the first $k-1$ vertices in $G_1$.

We also order the edges of $G_1$



We try to extend the isomorphism by finding a vertex $v \in V_2 - S$
that matches with vertex $k$. If a matching vertex is found, we have a
new isomorphism $f_k$ with $G_1[k] \isomorphic G_2[S \union \{ v \}]$.




\begin{tabbing}
IS\=O\=M\=O\=RPH($k$, $S$, $f_{k-1}$) $\equiv$ \\
\>\textbf{if} ($k = |V_1|+1$) \\
\>\>\textbf{return} true \\
\>\textbf{for} each vertex $v \in V_2 - S$ \\
\>\>\textbf{if} (MATCH($k$, $v$)) \\
\>\>\>$f_k = f_{k-1} \union \pair{k}{v}$ \\
\>\>\>ISOMORPH($k+1$, $S \union \{ v \}$, $f_k$)\\
\>\>\textbf{else}\\
\>\>\>\textbf{return} false \\
\\
ISOMORPH($0$, $G_1$, $\emptyset$, $G_2$)
\end{tabbing}

The basic idea of the match operation is to check whether $G_1[k]$ is
isomorphic to $G_2[S \union \{ v \}]$. We already know that $G_1[k-1]
\isomorphic G_2[S]$ with the mapping $f_{k-1}$, so all we need to do
is verify that the edges in $E_1[k] - E_1[k-1]$ connect vertices that
correspond to the vertices connected by the edges in $E_2[S \union \{
v \}] - E_2[S]$. The edges in $E_1[k] - E_1[k-1]$ are all the
out-edges $(k,j)$ and in-edges $(j,k)$ of $k$ where $j$ is less than
or equal to $k$ according to the ordering.  The edges in $E_2[S \union
\{ v \}] - E_2[S]$ consists of all the out-edges $(v,u)$ and
in-edges $(u,v)$ of $v$ where $u \in S$.

\begin{tabbing}
M\=ATCH($k$, $v$) $\equiv$ \\
\>$out_k \leftarrow \forall (k,j) \in E_1[k] - E_1[k-1] \Big( (v,f(j)) \in E_2[S \union \{ v \}] - E_2[S] \Big)$ \\
\>$in_k \leftarrow \forall (j,k) \in E_1[k] - E_1[k-1] \Big( (f(j),v) \in E_2[S \union \{ v \}] - E_2[S] \Big)$ \\
\>$out_v \leftarrow \forall (v,u) \in E_2[S \union \{ v \}] - E_2[S] \Big( (k,f^{-1}(u)) \in E_1[k] - E_1[k-1] \Big)$ \\
\>$in_v \leftarrow \forall (u,v) \in E_2[S \union \{ v \}] - E_2[S] \Big( (f^{-1}(u),k) \in E_1[k] - E_1[k-1] \Big)$ \\
\>\textbf{return} $out_k \Land in_k \Land out_v \Land in_v$ 
\end{tabbing}

The problem with the exhaustive backtracking algorithm is that there
are $N!$ possible vertex mappings, and $N!$ gets very large as $N$
increases, so we need to prune the search space. We use the pruning
techniques described in
\cite{deo77:_new_algo_digraph_isomorph,fortin96:_isomorph,reingold77:_combin_algo}
that originated in
\cite{sussenguth65:_isomorphism,unger64:_isomorphism}.

\section{Vertex Invariants}
\label{sec:vertex-invariants}

One way to reduce the search space is through the use of \emph{vertex
invariants}. The idea is to compute a number for each vertex $i(v)$
such that $i(v) = i(v')$ if there exists some isomorphism $f$ where
$f(v) = v'$. Then when we look for a match to some vertex $v$, we only
need to consider those vertices that have the same vertex invariant
number. The number of vertices in a graph with the same vertex
invariant number $i$ is called the \emph{invariant multiplicity} for
$i$.  In this implementation, by default we use the out-degree of the
vertex as the vertex invariant, though the user can also supply there
own invariant function. The ability of the invariant function to prune
the search space varies widely with the type of graph.

As a first check to rule out graphs that have no possibility of
matching, one can create a list of computed vertex invariant numbers
for the vertices in each graph, sort the two lists, and then compare
them.  If the two lists are different then the two graphs are not
isomorphic.  If the two lists are the same then the two graphs may be
isomorphic.

Also, we extend the MATCH operation to use the vertex invariants to
help rule out vertices.

\section{Vertex Order}

A good choice of the labeling for the vertices (which determines the
order in which the subgraph $G_1[k]$ is grown) can also reduce the
search space. In the following we discuss two labeling heuristics.

\subsection{Most Constrained First}

Consider the most constrained vertices first.  That is, examine
lower-degree vertices before higher-degree vertices. This reduces the
search space because it chops off a trunk before the trunk has a
chance to blossom out. We can generalize this to use vertex
invariants. We examine vertices with low invariant multiplicity
before examining vertices with high invariant multiplicity.

\subsection{Adjacent First}

The MATCH operation only considers edges when the other vertex already
has a mapping defined. This means that the MATCH operation can only
weed out vertices that are adjacent to vertices that have already been
matched. Therefore, when choosing the next vertex to examine, it is
desirable to choose one that is adjacent a vertex already in $S_1$.

\subsection{DFS Order, Starting with Lowest Multiplicity}

For this implementation, we combine the above two heuristics in the
following way. To implement the ``adjacent first'' heuristic we apply
DFS to the graph, and use the DFS discovery order as our vertex
order. To comply with the ``most constrained first'' heuristic we
order the roots of our DFS trees by invariant multiplicity.



\section{Implementation}

The following is the public interface for the \code{isomorphism}
function. The input to the function is the two graphs $G_1$ and $G_2$,
mappings from the vertices in the graphs to integers (in the range
$[0,|V|)$), and a vertex invariant function object. The output of the
function is an isomorphism $f$ if there is one. The \code{isomorphism}
function returns true if the graphs are isomorphic and false
otherwise. The invariant parameters are function objects that compute
the vertex invariants for vertices of the two graphs.  The
\code{max\_invariant} parameter is to specify one past the largest
integer that a vertex invariant number could be (the invariants
numbers are assumed to span from zero to the number).  The
requirements on type template parameters are described below in the
``Concept checking'' part.


@d Isomorphism function interface
@{
template <typename Graph1, typename Graph2, typename IsoMapping, 
          typename Invariant1, typename Invariant2,
          typename IndexMap1, typename IndexMap2>
bool isomorphism(const Graph1& G1, const Graph2& G2, IsoMapping f, 
                 Invariant1 invariant1, Invariant2 invariant2, 
                 std::size_t max_invariant,
                 IndexMap1 index_map1, IndexMap2 index_map2)
@}


The function body consists of the concept checks followed by a quick
check for empty graphs or graphs of different size and then construct
an algorithm object. We then call the \code{test\_isomorphism} member
function, which runs the algorithm.  The reason that we implement the
algorithm using a class is that there are a fair number of internal
data structures required, and it is easier to make these data members
of a class and make each section of the algorithm a member
function. This relieves us from the burden of passing lots of
arguments to each function, while at the same time avoiding the evils
of global variables (non-reentrant, etc.).


@d Isomorphism function body
@{
{
    @<Concept checking@>
    @<Quick return based on size@>
    detail::isomorphism_algo<Graph1, Graph2, IsoMapping, Invariant1, Invariant2, 
        IndexMap1, IndexMap2> 
        algo(G1, G2, f, invariant1, invariant2, max_invariant, 
             index_map1, index_map2);
    return algo.test_isomorphism();
}
@}


\noindent If there are no vertices in either graph, then they are
trivially isomorphic. If the graphs have different numbers of vertices
then they are not isomorphic.

@d Quick return based on size
@{
if (num_vertices(G1) != num_vertices(G2))
    return false;
if (num_vertices(G1) == 0 && num_vertices(G2) == 0)
    return true;
@}

We use the Boost Concept Checking Library to make sure that the type
arguments to the function fulfill there requirements. The graph types
must model the \bglconcept{VertexListGraph} and
\bglconcept{AdjacencyGraph} concepts. The vertex invariants must model
the \stlconcept{AdaptableUnaryFunction} concept, with a vertex as
their argument and an integer return type.  The \code{IsoMapping} type
that represents the isomorphism $f$ must be a
\pmconcept{ReadWritePropertyMap} that maps from vertices in $G_1$ to
vertices in $G_2$. The two other index maps are
\pmconcept{ReadablePropertyMap}s from vertices in $G_1$ and $G_2$ to
unsigned integers.


@d Concept checking
@{
// Graph requirements
BOOST_CONCEPT_ASSERT(( VertexListGraphConcept<Graph1> ));
BOOST_CONCEPT_ASSERT(( EdgeListGraphConcept<Graph1> ));
BOOST_CONCEPT_ASSERT(( VertexListGraphConcept<Graph2> ));
BOOST_CONCEPT_ASSERT(( BidirectionalGraphConcept<Graph2> ));

typedef typename graph_traits<Graph1>::vertex_descriptor vertex1_t;
typedef typename graph_traits<Graph2>::vertex_descriptor vertex2_t;
typedef typename graph_traits<Graph1>::vertices_size_type size_type;

// Vertex invariant requirement
BOOST_CONCEPT_ASSERT(( AdaptableUnaryFunctionConcept<Invariant1,
  size_type, vertex1_t> ));
BOOST_CONCEPT_ASSERT(( AdaptableUnaryFunctionConcept<Invariant2,
  size_type, vertex2_t> ));

// Property map requirements
BOOST_CONCEPT_ASSERT(( ReadWritePropertyMapConcept<IsoMapping, vertex1_t> ));
typedef typename property_traits<IsoMapping>::value_type IsoMappingValue;
BOOST_STATIC_ASSERT((is_same<IsoMappingValue, vertex2_t>::value));

BOOST_CONCEPT_ASSERT(( ReadablePropertyMapConcept<IndexMap1, vertex1_t> ));
typedef typename property_traits<IndexMap1>::value_type IndexMap1Value;
BOOST_STATIC_ASSERT((is_convertible<IndexMap1Value, size_type>::value));

BOOST_CONCEPT_ASSERT(( ReadablePropertyMapConcept<IndexMap2, vertex2_t> ));
typedef typename property_traits<IndexMap2>::value_type IndexMap2Value;
BOOST_STATIC_ASSERT((is_convertible<IndexMap2Value, size_type>::value));
@}

The following is the outline of the isomorphism algorithm class.  The
class is templated on all of the same parameters of the
\code{isomorphism} function, and all of the parameter values are
stored in the class as data members, in addition to the internal data
structures.

@d Isomorphism algorithm class
@{
template <typename Graph1, typename Graph2, typename IsoMapping,
  typename Invariant1, typename Invariant2,
  typename IndexMap1, typename IndexMap2>
class isomorphism_algo
{
    @<Typedefs for commonly used types@>
    @<Data members for the parameters@>
    @<Ordered edge class@>
    @<Internal data structures@>
    friend struct compare_multiplicity;
    @<Invariant multiplicity comparison functor@>
    @<DFS visitor to record vertex and edge order@>
public:
    @<Isomorphism algorithm constructor@>
    @<Test isomorphism member function@>
private:
    @<Match function@>
};
@}

The interesting parts of this class are the \code{test\_isomorphism}
function, and the \code{match} function. We focus on those in in the
following sections, and mention the other parts of the class when
needed (and a few are left to the appendix).

The \code{test\_isomorphism} function does all of the setup required
of the algorithm. This consists of sorting the vertices according to
invariant multiplicity, and then by DFS order.  The edges are then
sorted by the DFS order of vertices incident on the edges. More
details about this to come. The last step of this function is to
invoke the recursive \code{match} function which performs the
backtracking search.


@d Test isomorphism member function
@{
bool test_isomorphism()
{
    @<Quick return if the vertex invariants do not match up@>
    @<Sort vertices according to invariant multiplicity@>
    @<Order vertices and edges by DFS@>
    @<Sort edges according to vertex DFS order@>

    return this->match(ordered_edges.begin());
}
@}

As discussed in \S\ref{sec:vertex-invariants}, we can quickly rule out
the possibility of any isomorphism between two graphs by checking to
see if the vertex invariants can match up. We sort both vectors of vertex
invariants, and then check to see if they are equal.

@d Quick return if the vertex invariants do not match up
@{
{
    std::vector<invar1_value> invar1_array;
    BGL_FORALL_VERTICES_T(v, G1, Graph1)
        invar1_array.push_back(invariant1(v));
    std::sort(invar1_array.begin(), invar1_array.end());

    std::vector<invar2_value> invar2_array;
    BGL_FORALL_VERTICES_T(v, G2, Graph2)
        invar2_array.push_back(invariant2(v));
    std::sort(invar2_array.begin(), invar2_array.end());

    if (!std::equal(invar1_array.begin(), invar1_array.end(), invar2_array.begin()))
        return false;
}
@}

Next we compute the invariant multiplicity, the number of vertices
with the same invariant number. The \code{invar\_mult} vector is
indexed by invariant number. We loop through all the vertices in the
graph to record the multiplicity. We then order the vertices by their
invariant multiplicity.  This will allow us to search the more
constrained vertices first.

@d Sort vertices according to invariant multiplicity
@{
std::vector<vertex1_t> V_mult;
BGL_FORALL_VERTICES_T(v, G1, Graph1)
    V_mult.push_back(v);
{
    std::vector<size_type> multiplicity(max_invariant, 0);
    BGL_FORALL_VERTICES_T(v, G1, Graph1)
        ++multiplicity[invariant1(v)];

    std::sort(V_mult.begin(), V_mult.end(), compare_multiplicity(*this, &multiplicity[0]));
}
@}

\noindent The definition of the \code{compare\_multiplicity} predicate
is shown below. This predicate provides the glue that binds
\code{std::sort} to our current purpose.

@d Invariant multiplicity comparison functor
@{
struct compare_multiplicity
{
    compare_multiplicity(isomorphism_algo& algo, size_type* multiplicity)
        : algo(algo), multiplicity(multiplicity) { }
    bool operator()(const vertex1_t& x, const vertex1_t& y) const {
        return multiplicity[algo.invariant1(x)] < multiplicity[algo.invariant1(y)];
    }
    isomorphism_algo& algo;
    size_type* multiplicity;
};
@}

\subsection{Backtracking Search and Matching}






\subsection{Ordering by DFS Discover Time}

To implement the ``visit adjacent vertices first'' heuristic, we order
the vertices according to DFS discover time. This will give us the
order that the subgraph $G_1[k]$ will be expanded. As described in
\S\ref{sec:backtracking}, when trying to match $k$ with some vertex
$v$ in $V_2 - S$, we need to examine the edges in $E_1[k] -
E_1[k-1]$. It would be nice if we had the edges of $G_1$ arranged so
that when we are interested in vertex $k$, the edges in $E_1[k] -
E_1[k-1]$ are easy to find. This can be achieved by creating an array
of edges sorted by the DFS number of the larger of the source and
target vertex. The following array of ordered edges corresponds
to the graph in Figure~\ref{fig:edge-order}.

\begin{tabular}{cccccccccc}
      &0&1&2&3&4&5&6&7&8\\ \hline
source&0&1&1&3&3&4&4&5&6\\
target&1&2&3&1&2&3&5&6&4
\end{tabular}

The backtracking algorithm will scan through the edge array from left
to right to extend isomorphic subgraphs, and move back to the right
when a match fails. We will want to 










For example, suppose we have already matched the vertices
\{0,1,2\}, and 



\vizfig{edge-order}{Vertices with DFS numbering. The DFS trees are the solid edges.}

@c edge-order.dot
@{
digraph G {
size="3,2"
ratio=fill
node[shape=circle]
0 -> 1[style=bold]
1 -> 2[style=bold]
1 -> 3[style=bold]
3 -> 1[style=dashed]
3 -> 2[style=dashed]
4 -> 3[style=dashed]
4 -> 5[style=bold]
5 -> 6[style=bold]
6 -> 4[style=dashed]
}
@}




We implement the outer-loop of the DFS here, instead of calling the
\code{depth\_first\_search} function, because we want the roots of the
DFS tree's to be ordered by invariant multiplicity. We call
\code{depth\_\-first\_\-visit} to implement the recursive portion of
the DFS. The \code{record\_dfs\_order} adapts the DFS to record the
order in which DFS discovers the vertices, storing the results in in
the \code{dfs\_vertices} and \code{ordered\_edges} arrays. We then
create the \code{dfs\_number} array which provides a mapping from
vertex to DFS number, and renumber the edges with the DFS numbers.

@d Order vertices and edges by DFS
@{
std::vector<default_color_type> color_vec(num_vertices(G1));
safe_iterator_property_map<std::vector<default_color_type>::iterator, IndexMap1>
     color_map(color_vec.begin(), color_vec.size(), index_map1);
record_dfs_order dfs_visitor(dfs_vertices, ordered_edges);
typedef color_traits<default_color_type> Color;
for (vertex_iter u = V_mult.begin(); u != V_mult.end(); ++u) {
    if (color_map[*u] == Color::white()) {
	dfs_visitor.start_vertex(*u, G1);
	depth_first_visit(G1, *u, dfs_visitor, color_map);
    }
}
// Create the dfs_number array and dfs_number_map
dfs_number_vec.resize(num_vertices(G1));
dfs_number = make_safe_iterator_property_map(dfs_number_vec.begin(),
	                  dfs_number_vec.size(), index_map1);
size_type n = 0;
for (vertex_iter v = dfs_vertices.begin(); v != dfs_vertices.end(); ++v)
    dfs_number[*v] = n++;

// Renumber ordered_edges array according to DFS number
for (edge_iter e = ordered_edges.begin(); e != ordered_edges.end(); ++e) {
    if (e->source >= 0)
      e->source = dfs_number_vec[e->source];
    e->target = dfs_number_vec[e->target];
}
@}

\noindent The definition of the \code{record\_dfs\_order} visitor
class is as follows. EXPLAIN ficticious edges

@d DFS visitor to record vertex and edge order
@{
struct record_dfs_order : default_dfs_visitor
{
    record_dfs_order(std::vector<vertex1_t>& v, std::vector<ordered_edge>& e) 
	: vertices(v), edges(e) { }

    void start_vertex(vertex1_t v, const Graph1&) const {
        edges.push_back(ordered_edge(-1, v));
    }
    void discover_vertex(vertex1_t v, const Graph1&) const {
        vertices.push_back(v);
    }
    void examine_edge(edge1_t e, const Graph1& G1) const {
        edges.push_back(ordered_edge(source(e, G1), target(e, G1)));
    }
    std::vector<vertex1_t>& vertices;
    std::vector<ordered_edge>& edges;
};
@}


Reorder the edges so that all edges belonging to $G_1[k]$
appear before any edges not in $G_1[k]$, for $k=1,...,n$.

The order field needs a better name. How about k?

@d Sort edges according to vertex DFS order
@{
std::stable_sort(ordered_edges.begin(), ordered_edges.end());
// Fill in i->k_num field
if (!ordered_edges.empty()) {
  ordered_edges[0].k_num = 0;
  for (edge_iter i = next(ordered_edges.begin()); i != ordered_edges.end(); ++i)
      i->k_num = std::max(prior(i)->source, prior(i)->target);
}
@}






@d Typedefs for commonly used types
@{
typedef typename graph_traits<Graph1>::vertex_descriptor vertex1_t;
typedef typename graph_traits<Graph2>::vertex_descriptor vertex2_t;
typedef typename graph_traits<Graph1>::edge_descriptor edge1_t;
typedef typename graph_traits<Graph1>::vertices_size_type size_type;
typedef typename Invariant1::result_type invar1_value;
typedef typename Invariant2::result_type invar2_value;
@}

@d Data members for the parameters
@{
const Graph1& G1;
const Graph2& G2;
IsoMapping f;
Invariant1 invariant1;
Invariant2 invariant2;
std::size_t max_invariant;
IndexMap1 index_map1;
IndexMap2 index_map2;
@}

@d Internal data structures
@{
std::vector<vertex1_t> dfs_vertices;
typedef std::vector<vertex1_t>::iterator vertex_iter;
std::vector<size_type> dfs_number_vec;
safe_iterator_property_map<typename std::vector<size_type>::iterator, IndexMap1>
   dfs_number;
std::vector<ordered_edge> ordered_edges;
typedef std::vector<ordered_edge>::iterator edge_iter;

std::vector<vertex1_t> f_inv_vec;
safe_iterator_property_map<typename std::vector<vertex1_t>::iterator,
    IndexMap2> f_inv;

std::vector<char> f_assigned_vec;
safe_iterator_property_map<typename std::vector<char>::iterator,
    IndexMap1> f_assigned;

std::vector<char> f_inv_assigned_vec;
safe_iterator_property_map<typename std::vector<char>::iterator,
    IndexMap2> f_inv_assigned;

int num_edges_incident_on_k;
@}

@d Isomorphism algorithm constructor
@{
isomorphism_algo(const Graph1& G1, const Graph2& G2, IsoMapping f,
		 Invariant1 invariant1, Invariant2 invariant2, std::size_t max_invariant,
		 IndexMap1 index_map1, IndexMap2 index_map2)
    : G1(G1), G2(G2), f(f), invariant1(invariant1), invariant2(invariant2),
      max_invariant(max_invariant),
      index_map1(index_map1), index_map2(index_map2)
{
    f_assigned_vec.resize(num_vertices(G1));
    f_assigned = make_safe_iterator_property_map
	(f_assigned_vec.begin(), f_assigned_vec.size(), index_map1);
    f_inv_vec.resize(num_vertices(G1));
    f_inv = make_safe_iterator_property_map
	(f_inv_vec.begin(), f_inv_vec.size(), index_map2);

    f_inv_assigned_vec.resize(num_vertices(G1));
    f_inv_assigned = make_safe_iterator_property_map
	(f_inv_assigned_vec.begin(), f_inv_assigned_vec.size(), index_map2);
}
@}




@d Degree vertex invariant functor
@{
template <typename InDegreeMap, typename Graph>
class degree_vertex_invariant
{
    typedef typename graph_traits<Graph>::vertex_descriptor vertex_t;
    typedef typename graph_traits<Graph>::degree_size_type size_type;
public:
    typedef vertex_t argument_type;
    typedef size_type result_type;

    degree_vertex_invariant(const InDegreeMap& in_degree_map, const Graph& g)
        : m_in_degree_map(in_degree_map), m_g(g) { }

    size_type operator()(vertex_t v) const {
        return (num_vertices(m_g) + 1) * out_degree(v, m_g)
            + get(m_in_degree_map, v);
    }
    // The largest possible vertex invariant number
    size_type max() const { 
        return num_vertices(m_g) * num_vertices(m_g) + num_vertices(m_g);
    }
private:
    InDegreeMap m_in_degree_map;
    const Graph& m_g;
};
@}



ficticiuos edges for the DFS tree roots
Use \code{ordered\_edge} instead of \code{edge1\_t} so that we can create ficticious
edges for the DFS tree roots.

@d Ordered edge class
@{
struct ordered_edge {
    ordered_edge(int s, int t) : source(s), target(t) { }

    bool operator<(const ordered_edge& e) const {
        using namespace std;
        int m1 = max(source, target);
        int m2 = max(e.source, e.target);
        // lexicographical comparison of (m1,source,target) and (m2,e.source,e.target)
        return make_pair(m1, make_pair(source, target)) < make_pair(m2, make_pair(e.source, e.target));
    }
    int source;
    int target;
    int k_num;
};
@}






\subsection{Recursive Match Function}





@d $v$ is a DFS tree root
@{
// Try all possible mappings
BGL_FORALL_VERTICES_T(y, G2, Graph2) {
    if (invariant1(v) == invariant2(y) && f_inv_assigned[y] == false) {
        f[v] = y; f_assigned[v] = true;
        f_inv[y] = v; f_inv_assigned[y] = true;
        num_edges_incident_on_k = 0;
        if (match(next(iter)))
            return true;
        f_assigned[v] = false;
        f_inv_assigned[y] = false;
    }
}
@}

Growing the subgraph.

@d $v$ is an unmatched vertex, $(u,v)$ is a tree edge
@{
@<Count out-edges of $f(k)$ in $G_2[S]$@>
@<Count in-edges of $f(k)$ in $G_2[S]$@>
if (num_edges_incident_on_k != 0)
    return false;
@<Assign $v$ to some vertex in $V_2 - S$@>
@}
@d Count out-edges of $f(k)$ in $G_2[S]$
@{
BGL_FORALL_ADJACENT_T(f[k], w, G2, Graph2)
    if (f_inv_assigned[w] == true)
        --num_edges_incident_on_k;
@}

@d Count in-edges of $f(k)$ in $G_2[S]$
@{
for (std::size_t jj = 0; jj < k_num; ++jj) {
    vertex1_t j = dfs_vertices[jj];
    BGL_FORALL_ADJACENT_T(f[j], w, G2, Graph2)
        if (w == f[k])
            --num_edges_incident_on_k;
}
@}

@d Assign $v$ to some vertex in $V_2 - S$
@{
BGL_FORALL_ADJACENT_T(f[u], y, G2, Graph2)
    if (invariant1(v) == invariant2(y) && f_inv_assigned[y] == false) {
        f[v] = y; f_assigned[v] = true;
        f_inv[y] = v; f_inv_assigned[y] = true;
        num_edges_incident_on_k = 1;
        if (match(next(iter)))
            return true;
        f_assigned[v] = false;
        f_inv_assigned[y] = false;
    }
@}



@d Check to see if there is an edge in $G_2$ to match $(u,v)$
@{
bool verify = false;
assert(f_assigned[u] == true);
BGL_FORALL_ADJACENT_T(f[u], y, G2, Graph2) {
    if (y == f[v]) {    
        verify = true;
        break;
    }
}
if (verify == true) {
    ++num_edges_incident_on_k;
    if (match(next(iter)))
         return true;
}
@}



@o isomorphism-v2.hpp
@{
// Copyright (C) 2001 Jeremy Siek, Douglas Gregor, Brian Osman
//
// Permission to copy, use, sell and distribute this software is granted
// provided this copyright notice appears in all copies.
// Permission to modify the code and to distribute modified code is granted
// provided this copyright notice appears in all copies, and a notice
// that the code was modified is included with the copyright notice.
//
// This software is provided "as is" without express or implied warranty,
// and with no claim as to its suitability for any purpose.
#ifndef BOOST_GRAPH_ISOMORPHISM_HPP
#define BOOST_GRAPH_ISOMORPHISM_HPP

#include <utility>
#include <vector>
#include <iterator>
#include <algorithm>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/utility.hpp>
#include <boost/tuple/tuple.hpp>

namespace boost {

namespace detail {

@<Isomorphism algorithm class@>
    
template <typename Graph, typename InDegreeMap>
void compute_in_degree(const Graph& g, InDegreeMap in_degree_map)
{
    BGL_FORALL_VERTICES_T(v, g, Graph)
        put(in_degree_map, v, 0);

    BGL_FORALL_VERTICES_T(u, g, Graph)
      BGL_FORALL_ADJACENT_T(u, v, g, Graph)
        put(in_degree_map, v, get(in_degree_map, v) + 1);
}

} // namespace detail


@<Degree vertex invariant functor@>

@<Isomorphism function interface@>
@<Isomorphism function body@>

namespace detail {
  
template <typename Graph1, typename Graph2, 
          typename IsoMapping, 
          typename IndexMap1, typename IndexMap2,
          typename P, typename T, typename R>
bool isomorphism_impl(const Graph1& G1, const Graph2& G2, 
                      IsoMapping f, IndexMap1 index_map1, IndexMap2 index_map2,
		      const bgl_named_params<P,T,R>& params)
{
  std::vector<std::size_t> in_degree1_vec(num_vertices(G1));
  typedef safe_iterator_property_map<std::vector<std::size_t>::iterator, IndexMap1> InDeg1;
  InDeg1 in_degree1(in_degree1_vec.begin(), in_degree1_vec.size(), index_map1);
  compute_in_degree(G1, in_degree1);

  std::vector<std::size_t> in_degree2_vec(num_vertices(G2));
  typedef safe_iterator_property_map<std::vector<std::size_t>::iterator, IndexMap2> InDeg2;
  InDeg2 in_degree2(in_degree2_vec.begin(), in_degree2_vec.size(), index_map2);
  compute_in_degree(G2, in_degree2);

  degree_vertex_invariant<InDeg1, Graph1> invariant1(in_degree1, G1);
  degree_vertex_invariant<InDeg2, Graph2> invariant2(in_degree2, G2);

  return isomorphism(G1, G2, f,
        choose_param(get_param(params, vertex_invariant1_t()), invariant1),
        choose_param(get_param(params, vertex_invariant2_t()), invariant2),
        choose_param(get_param(params, vertex_max_invariant_t()), invariant2.max()),
	index_map1, index_map2
	);  
}  
   
} // namespace detail


// Named parameter interface
template <typename Graph1, typename Graph2, class P, class T, class R>
bool isomorphism(const Graph1& g1,
		 const Graph2& g2,
		 const bgl_named_params<P,T,R>& params)
{
  typedef typename graph_traits<Graph2>::vertex_descriptor vertex2_t;
  typename std::vector<vertex2_t>::size_type n = num_vertices(g1);
  std::vector<vertex2_t> f(n);
  return detail::isomorphism_impl
    (g1, g2, 
     choose_param(get_param(params, vertex_isomorphism_t()),
          make_safe_iterator_property_map(f.begin(), f.size(),
                  choose_const_pmap(get_param(params, vertex_index1),
		                    g1, vertex_index), vertex2_t())),
     choose_const_pmap(get_param(params, vertex_index1), g1, vertex_index),
     choose_const_pmap(get_param(params, vertex_index2), g2, vertex_index),
     params
     );
}

// All defaults interface
template <typename Graph1, typename Graph2>
bool isomorphism(const Graph1& g1, const Graph2& g2)
{
  return isomorphism(g1, g2,
    bgl_named_params<int, buffer_param_t>(0));// bogus named param
}


// Verify that the given mapping iso_map from the vertices of g1 to the
// vertices of g2 describes an isomorphism.
// Note: this could be made much faster by specializing based on the graph
// concepts modeled, but since we're verifying an O(n^(lg n)) algorithm,
// O(n^4) won't hurt us.
template<typename Graph1, typename Graph2, typename IsoMap>
inline bool verify_isomorphism(const Graph1& g1, const Graph2& g2, IsoMap iso_map)
{
  if (num_vertices(g1) != num_vertices(g2) || num_edges(g1) != num_edges(g2))
    return false;
  
  for (typename graph_traits<Graph1>::edge_iterator e1 = edges(g1).first;
       e1 != edges(g1).second; ++e1) {
    bool found_edge = false;
    for (typename graph_traits<Graph2>::edge_iterator e2 = edges(g2).first;
         e2 != edges(g2).second && !found_edge; ++e2) {
      if (source(*e2, g2) == get(iso_map, source(*e1, g1)) &&
          target(*e2, g2) == get(iso_map, target(*e1, g1))) {
        found_edge = true;
      }
    }
    
    if (!found_edge)
      return false;
  }
  
  return true;
}

} // namespace boost

#include <boost/graph/iteration_macros_undef.hpp>

#endif // BOOST_GRAPH_ISOMORPHISM_HPP
@}

\bibliographystyle{abbrv}
\bibliography{ggcl}

\end{document}
% LocalWords:  Isomorphism Siek isomorphism adjacency subgraph subgraphs OM DFS
% LocalWords:  ISOMORPH Invariants invariants typename IsoMapping bool const
% LocalWords:  VertexInvariant VertexIndexMap iterator typedef VertexG Idx num
% LocalWords:  InvarValue struct invar vec iter tmp_matches mult inserter permute ui
% LocalWords:  dfs cmp isomorph VertexIter edge_iter_t IndexMap desc RPH ATCH pre

% LocalWords:  iterators VertexListGraph EdgeListGraph BidirectionalGraph tmp
% LocalWords:  ReadWritePropertyMap VertexListGraphConcept EdgeListGraphConcept
% LocalWords:  BidirectionalGraphConcept ReadWritePropertyMapConcept indices ei
% LocalWords:  IsoMappingValue ReadablePropertyMapConcept namespace InvarFun
% LocalWords:  MultMap vip inline bitset typedefs fj hpp ifndef adaptor params
% LocalWords:  bgl param pmap endif

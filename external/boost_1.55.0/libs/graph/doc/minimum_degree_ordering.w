\documentclass[11pt]{report}

\input{defs}


\setlength\overfullrule{5pt}
\tolerance=10000
\sloppy
\hfuzz=10pt

\makeindex

\begin{document}

\title{An Implementation of the Multiple Minimum Degree Algorithm}
\author{Jeremy G. Siek and Lie Quan Lee}

\maketitle

\section{Introduction}

The minimum degree ordering algorithm \cite{LIU:MMD,George:evolution}
reorders a matrix to reduce fill-in. Fill-in is the term used to refer
to the zero elements of a matrix that become non-zero during the
gaussian elimination process.  Fill-in is bad because it makes the
matrix less sparse and therefore consume more time and space in later
stages of the elimintation and in computations that use the resulting
factorization. Reordering the rows of a matrix can have a dramatic
affect on the amount of fill-in that occurs. So instead of solving
\begin{eqnarray}
A x = b
\end{eqnarray}
we instead solve the reordered (but equivalent) system
\begin{eqnarray}
(P A P^T)(P x) = P b.
\end{eqnarray}
where $P$ is a permutation matrix.

Finding the optimal ordering is an NP-complete problem (e.i., it can
not be solved in a reasonable amount of time) so instead we just find
an ordering that is ``good enough'' using heuristics. The minimum
degree ordering algorithm is one such approach.

A symmetric matrix $A$ is typically represented with an undirected
graph, however for this function we require the input to be a directed
graph.  Each nonzero matrix entry $A_{ij}$ must be represented by two
directed edges, $(i,j)$ and $(j,i)$, in $G$.

An \keyword{elimination graph} $G_v$ is formed by removing vertex $v$
and all of its incident edges from graph $G$ and then adding edges to
make the vertices adjacent to $v$ into a clique\footnote{A clique is a
complete subgraph. That is, it is a subgraph where each vertex is
adjacent to every other vertex in the subgraph}.


quotient graph 
set of cliques in the graph


Mass elmination: if $y$ is selected as the minimum degree node then
the the vertices adjacent to $y$ with degree equal to $degree(y) - 1$
can be selected next (in any order).

Two nodes are \keyword{indistinguishable} if they have identical
neighborhood sets. That is,
\begin{eqnarray}
Adj[u] \cup \{ u \} = Adj[v] \bigcup \{ v \}
\end{eqnarray}
Nodes that are indistiguishable can be eliminated at the same time.

A representative for a set of indistiguishable nodes is called a
\keyword{supernode}.

incomplete degree update
external degree




\section{Algorithm Overview}

@d MMD Algorithm Overview @{
  @<Eliminate isolated nodes@>
  
@}




@d Set up a mapping from integers to vertices @{
size_type vid = 0;
typename graph_traits<Graph>::vertex_iterator v, vend;
for (tie(v, vend) = vertices(G); v != vend; ++v, ++vid)
  index_vertex_vec[vid] = *v;
index_vertex_map = IndexVertexMap(&index_vertex_vec[0]);
@}


@d Insert vertices into bucket sorter (bucket number equals degree) @{
for (tie(v, vend) = vertices(G); v != vend; ++v) {
  put(degree, *v, out_degree(*v, G));
  degreelists.push(*v);
}
@}


@d Eliminate isolated nodes (nodes with zero degree) @{
typename DegreeLists::stack list_isolated = degreelists[0];
while (!list_isolated.empty()) {
  vertex_t node = list_isolated.top();
  marker.mark_done(node);
  numbering(node);
  numbering.increment();
  list_isolated.pop();
}
@}


@d Find first non-empty degree bucket
@{
size_type min_degree = 1;
typename DegreeLists::stack list_min_degree = degreelists[min_degree];
while (list_min_degree.empty()) {
  ++min_degree;
  list_min_degree = degreelists[min_degree];
}
@}



@d Main Loop
@{
while (!numbering.all_done()) {
  eliminated_nodes = work_space.make_stack();
  if (delta >= 0)
    while (true) {
      @<Find next non-empty degree bucket@>
      @<Select a node of minimum degree for elimination@>
      eliminate(node);
    }
  if (numbering.all_done())
    break;
  update(eliminated_nodes, min_degree);
}
@}


@d Elimination Function
@{
void eliminate(vertex_t node)
{
  remove out-edges from the node if the target vertex was
    tagged or if it is numbered
  add vertices adjacent to node to the clique
  put all numbered adjacent vertices into the temporary neighbors stack
  
  @<Perform element absorption optimization@>
}
@}


@d
@{
bool remove_out_edges_predicate::operator()(edge_t e)
{
  vertex_t v = target(e, *g);
  bool is_tagged = marker->is_tagged(dist);
  bool is_numbered = numbering.is_numbered(v);
  if (is_numbered)
    neighbors->push(id[v]);
  if (!is_tagged)
    marker->mark_tagged(v);
  return is_tagged || is_numbered;
}
@}



How does this work????

Does \code{is\_tagged} mean in the clique??

@d Perform element absorption optimization
@{
while (!neighbors.empty()) {
  size_type n_id = neighbors.top();
  vertex_t neighbor = index_vertex_map[n_id];
  adjacency_iterator i, i_end;
  for (tie(i, i_end) = adjacent_vertices(neighbor, G); i != i_end; ++i) {
    vertex_t i_node = *i;
    if (!marker.is_tagged(i_node) && !numbering.is_numbered(i_node)) {
      marker.mark_tagged(i_node);
      add_edge(node, i_node, G);
    }
  }
  neighbors.pop();
}
@}



@d
@{
predicateRemoveEdge1<Graph, MarkerP, NumberingD, 
                     typename Workspace::stack, VertexIndexMap>
  p(G, marker, numbering, element_neighbor, vertex_index_map);

remove_out_edge_if(node, p, G);
@}


\section{The Interface}


@d Interface of the MMD function @{
template<class Graph, class DegreeMap, 
         class InversePermutationMap, 
         class PermutationMap,
         class SuperNodeMap, class VertexIndexMap>
void minimum_degree_ordering
  (Graph& G, 
   DegreeMap degree, 
   InversePermutationMap inverse_perm, 
   PermutationMap perm, 
   SuperNodeMap supernode_size, 
   int delta,
   VertexIndexMap vertex_index_map)
@}



\section{Representing Disjoint Stacks}

Given a set of $N$ integers (where the integer values range from zero
to $N-1$), we want to keep track of a collection of stacks of
integers. It so happens that an integer will appear in at most one
stack at a time, so the stacks form disjoint sets.  Because of these
restrictions, we can use one big array to store all the stacks,
intertwined with one another.  No allocation/deallocation happens in
the \code{push()}/\code{pop()} methods so this is faster than using
\code{std::stack}.


\section{Bucket Sorting}



@d Bucket Sorter Class Interface @{
template <typename BucketMap, typename ValueIndexMap>
class bucket_sorter {
public:
  typedef typename property_traits<BucketMap>::value_type bucket_type;
  typedef typename property_traits<ValueIndex>::key_type value_type;
  typedef typename property_traits<ValueIndex>::value_type size_type;

  bucket_sorter(size_type length, bucket_type max_bucket, 
                const BucketMap& bucket = BucketMap(), 
                const ValueIndexMap& index_map = ValueIndexMap());
  void remove(const value_type& x);
  void push(const value_type& x);
  void update(const value_type& x);

  class stack {
  public:
    void push(const value_type& x);
    void pop();
    value_type& top();
    const value_type& top() const;
    bool empty() const;
  private:
    @<Bucket Stack Constructor@>
    @<Bucket Stack Data Members@>
  };
  stack operator[](const bucket_type& i);
private:
  @<Bucket Sorter Data Members@>
};
@}

\code{BucketMap} is a \concept{LvaluePropertyMap} that converts a
value object to a bucket number (an integer). The range of the mapping
must be finite. \code{ValueIndexMap} is a \concept{LvaluePropertyMap}
that maps from value objects to a unique integer. At the top of the
definition of \code{bucket\_sorter} we create some typedefs for the
bucket number type, the value type, and the index type.

@d Bucket Sorter Data Members @{
std::vector<size_type>   head;
std::vector<size_type>   next;
std::vector<size_type>   prev;
std::vector<value_type>  id_to_value;
BucketMap bucket_map;
ValueIndexMap index_map;
@}


\code{N} is the maximum integer that the \code{index\_map} will map a
value object to (the minimum is assumed to be zero).

@d Bucket Sorter Constructor @{
bucket_sorter::bucket_sorter
  (size_type N, bucket_type max_bucket,
   const BucketMap& bucket_map = BucketMap(), 
   const ValueIndexMap& index_map = ValueIndexMap())
  : head(max_bucket, invalid_value()),
    next(N, invalid_value()), 
    prev(N, invalid_value()),
    id_to_value(N),
    bucket_map(bucket_map), index_map(index_map) { }
@}




\bibliographystyle{abbrv}
\bibliography{jtran,ggcl,optimization,generic-programming,cad}

\end{document}


//          Copyright W.P. McNeill 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


// This program uses the A-star search algorithm in the Boost Graph Library to
// solve a maze.  It is an example of how to apply Boost Graph Library
// algorithms to implicit graphs.
//
// This program generates a random maze and then tries to find the shortest
// path from the lower left-hand corner to the upper right-hand corner.  Mazes
// are represented by two-dimensional grids where a cell in the grid may
// contain a barrier.  You may move up, down, right, or left to any adjacent
// cell that does not contain a barrier.
//
// Once a maze solution has been attempted, the maze is printed.  If a
// solution was found it will be shown in the maze printout and its length
// will be returned.  Note that not all mazes have solutions.
//
// The default maze size is 20x10, though different dimensions may be
// specified on the command line.


#include <boost/graph/astar_search.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/grid_graph.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <ctime>
#include <iostream>

boost::mt19937 random_generator;

// Distance traveled in the maze
typedef double distance;

#define GRID_RANK 2
typedef boost::grid_graph<GRID_RANK> grid;
typedef boost::graph_traits<grid>::vertex_descriptor vertex_descriptor;
typedef boost::graph_traits<grid>::vertices_size_type vertices_size_type;

// A hash function for vertices.
struct vertex_hash:std::unary_function<vertex_descriptor, std::size_t> {
  std::size_t operator()(vertex_descriptor const& u) const {
    std::size_t seed = 0;
    boost::hash_combine(seed, u[0]);
    boost::hash_combine(seed, u[1]);
    return seed;
  }
};

typedef boost::unordered_set<vertex_descriptor, vertex_hash> vertex_set;
typedef boost::vertex_subset_complement_filter<grid, vertex_set>::type
        filtered_grid;

// A searchable maze
//
// The maze is grid of locations which can either be empty or contain a
// barrier.  You can move to an adjacent location in the grid by going up,
// down, left and right.  Moving onto a barrier is not allowed.  The maze can
// be solved by finding a path from the lower-left-hand corner to the
// upper-right-hand corner.  If no open path exists between these two
// locations, the maze is unsolvable.
//
// The maze is implemented as a filtered grid graph where locations are
// vertices.  Barrier vertices are filtered out of the graph.
//
// A-star search is used to find a path through the maze. Each edge has a
// weight of one, so the total path length is equal to the number of edges
// traversed.
class maze {
public:
  friend std::ostream& operator<<(std::ostream&, const maze&);
  friend maze random_maze(std::size_t, std::size_t);

  maze():m_grid(create_grid(0, 0)),m_barrier_grid(create_barrier_grid()) {};
  maze(std::size_t x, std::size_t y):m_grid(create_grid(x, y)),
       m_barrier_grid(create_barrier_grid()) {};

  // The length of the maze along the specified dimension.
  vertices_size_type length(std::size_t d) const {return m_grid.length(d);}

  bool has_barrier(vertex_descriptor u) const {
    return m_barriers.find(u) != m_barriers.end();
  }

  // Try to find a path from the lower-left-hand corner source (0,0) to the
  // upper-right-hand corner goal (x-1, y-1).
  vertex_descriptor source() const {return vertex(0, m_grid);}
  vertex_descriptor goal() const {
    return vertex(num_vertices(m_grid)-1, m_grid);
  }

  bool solve();
  bool solved() const {return !m_solution.empty();}
  bool solution_contains(vertex_descriptor u) const {
    return m_solution.find(u) != m_solution.end();
  }

private:
  // Create the underlying rank-2 grid with the specified dimensions.
  grid create_grid(std::size_t x, std::size_t y) {
    boost::array<std::size_t, GRID_RANK> lengths = { {x, y} };
    return grid(lengths);
  }

  // Filter the barrier vertices out of the underlying grid.
  filtered_grid create_barrier_grid() {
    return boost::make_vertex_subset_complement_filter(m_grid, m_barriers);
  }

  // The grid underlying the maze
  grid m_grid;
  // The underlying maze grid with barrier vertices filtered out
  filtered_grid m_barrier_grid;
  // The barriers in the maze
  vertex_set m_barriers;
  // The vertices on a solution path through the maze
  vertex_set m_solution;
  // The length of the solution path
  distance m_solution_length;
};


// Euclidean heuristic for a grid
//
// This calculates the Euclidean distance between a vertex and a goal
// vertex.
class euclidean_heuristic:
      public boost::astar_heuristic<filtered_grid, double>
{
public:
  euclidean_heuristic(vertex_descriptor goal):m_goal(goal) {};

  double operator()(vertex_descriptor v) {
    return sqrt(pow(double(m_goal[0] - v[0]), 2) + pow(double(m_goal[1] - v[1]), 2));
  }

private:
  vertex_descriptor m_goal;
};

// Exception thrown when the goal vertex is found
struct found_goal {};

// Visitor that terminates when we find the goal vertex
struct astar_goal_visitor:public boost::default_astar_visitor {
  astar_goal_visitor(vertex_descriptor goal):m_goal(goal) {};

  void examine_vertex(vertex_descriptor u, const filtered_grid&) {
    if (u == m_goal)
      throw found_goal();
  }

private:
  vertex_descriptor m_goal;
};

// Solve the maze using A-star search.  Return true if a solution was found.
bool maze::solve() {
  boost::static_property_map<distance> weight(1);
  // The predecessor map is a vertex-to-vertex mapping.
  typedef boost::unordered_map<vertex_descriptor,
                               vertex_descriptor,
                               vertex_hash> pred_map;
  pred_map predecessor;
  boost::associative_property_map<pred_map> pred_pmap(predecessor);
  // The distance map is a vertex-to-distance mapping.
  typedef boost::unordered_map<vertex_descriptor,
                               distance,
                               vertex_hash> dist_map;
  dist_map distance;
  boost::associative_property_map<dist_map> dist_pmap(distance);

  vertex_descriptor s = source();
  vertex_descriptor g = goal();
  euclidean_heuristic heuristic(g);
  astar_goal_visitor visitor(g);

  try {
    astar_search(m_barrier_grid, s, heuristic,
                 boost::weight_map(weight).
                 predecessor_map(pred_pmap).
                 distance_map(dist_pmap).
                 visitor(visitor) );
  } catch(found_goal fg) {
    // Walk backwards from the goal through the predecessor chain adding
    // vertices to the solution path.
    for (vertex_descriptor u = g; u != s; u = predecessor[u])
      m_solution.insert(u);
    m_solution.insert(s);
    m_solution_length = distance[g];
    return true;
  }

  return false;
}


#define BARRIER "#"
// Print the maze as an ASCII map.
std::ostream& operator<<(std::ostream& output, const maze& m) {
  // Header
  for (vertices_size_type i = 0; i < m.length(0)+2; i++)
    output << BARRIER;
  output << std::endl;
  // Body
  for (int y = m.length(1)-1; y >= 0; y--) {
    // Enumerate rows in reverse order and columns in regular order so that
    // (0,0) appears in the lower left-hand corner.  This requires that y be
    // int and not the unsigned vertices_size_type because the loop exit
    // condition is y==-1.
    for (vertices_size_type x = 0; x < m.length(0); x++) {
      // Put a barrier on the left-hand side.
      if (x == 0)
        output << BARRIER;
      // Put the character representing this point in the maze grid.
      vertex_descriptor u = {{x, vertices_size_type(y)}};
      if (m.solution_contains(u))
        output << ".";
      else if (m.has_barrier(u))
        output << BARRIER;
      else
        output << " ";
      // Put a barrier on the right-hand side.
      if (x == m.length(0)-1)
        output << BARRIER;
    }
    // Put a newline after every row except the last one.
    output << std::endl;
  }
  // Footer
  for (vertices_size_type i = 0; i < m.length(0)+2; i++)
    output << BARRIER;
  if (m.solved())
    output << std::endl << "Solution length " << m.m_solution_length;
  return output;
}

// Return a random integer in the interval [a, b].
std::size_t random_int(std::size_t a, std::size_t b) {
  if (b < a)
    b = a;
  boost::uniform_int<> dist(a, b);
  boost::variate_generator<boost::mt19937&, boost::uniform_int<> >
  generate(random_generator, dist);
  return generate();
}

// Generate a maze with a random assignment of barriers.
maze random_maze(std::size_t x, std::size_t y) {
  maze m(x, y);
  vertices_size_type n = num_vertices(m.m_grid);
  vertex_descriptor s = m.source();
  vertex_descriptor g = m.goal();
  // One quarter of the cells in the maze should be barriers.
  int barriers = n/4;
  while (barriers > 0) {
    // Choose horizontal or vertical direction.
    std::size_t direction = random_int(0, 1);
    // Walls range up to one quarter the dimension length in this direction.
    vertices_size_type wall = random_int(1, m.length(direction)/4);
    // Create the wall while decrementing the total barrier count.
    vertex_descriptor u = vertex(random_int(0, n-1), m.m_grid);
    while (wall) {
      // Start and goal spaces should never be barriers.
      if (u != s && u != g) {
        wall--;
        if (!m.has_barrier(u)) {
          m.m_barriers.insert(u);
          barriers--;
        }
      }
      vertex_descriptor v = m.m_grid.next(u, direction);
      // Stop creating this wall if we reached the maze's edge.
      if (u == v)
        break;
      u = v;
    }
  }
  return m;
}


int main (int argc, char const *argv[]) {
  // The default maze size is 20x10.  A different size may be specified on
  // the command line.
  std::size_t x = 20;
  std::size_t y = 10;

  if (argc == 3) {
    x = boost::lexical_cast<std::size_t>(argv[1]);
    y = boost::lexical_cast<std::size_t>(argv[2]);
  }

  random_generator.seed(std::time(0));
  maze m = random_maze(x, y);

  if (m.solve())
    std::cout << "Solved the maze." << std::endl;
  else
    std::cout << "The maze is not solvable." << std::endl;
  std::cout << m << std::endl;
  return 0;
}

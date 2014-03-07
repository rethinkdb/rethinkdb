What's In This Directory
======================== 

* player1.cpp - this is exactly what's covered in C++ Template
  Metaprogramming (http://www.boost-consulting.com/mplbook); in fact,
  it was auto-extracted from the examples as shown in the book.  The
  state machine framework and its use are together in one .cpp file;
  normally they would be separated. You can think of the framework as
  ending with the definition of the generate_dispatcher class
  template.

  You can ignore the typedef called "dummy;" that was included in order to
  test an intermediate example that appears in the book.

* player2.cpp - this example demonstrates that the abstraction of the
  framework is complete by replacing its implementation with one that
  dispatches using O(1) table lookups, while still using the same code
  to describe the particular FSM.  Look at this one if you want to see
  how to generate a static lookup table that's initialized at dynamic
  initialization time.

* player.cpp, state_machine.hpp - This example predates the book, and
  is more sophisticated in some ways than what we cover in the other
  examples.  In particular, it supports state invariants, and it
  maintains an internal event queue, which requires an additional
  layer of runtime dispatching to sort out the next event to be
  processed.


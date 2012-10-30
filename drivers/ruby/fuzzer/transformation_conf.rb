# Copyright 2010-2012 RethinkDB, all rights reserved.
{ One_Byte_Transformation =>
  # PRIORITY, PROC
  [[100, :mangle_byte],
   [100, :zero_byte],
   [100, :swap_byte]],

  Multi_Byte_Transformation =>
  # PRIORITY, PROB, PROC
  [[ 10, 0.01, :mangle_byte],
   [100, 0.1,  :mangle_byte],
   [ 50, 0.3,  :mangle_byte],
   [ 10, 0.95, :mangle_byte],

   [ 10, 0.01, :zero_byte],
   [100, 0.1,  :zero_byte],
   [ 50, 0.3,  :zero_byte],
   [  5, 0.95, :zero_byte],

   [ 10, 0.01, :swap_byte],
   [100, 0.1,  :swap_byte],
   [100, 0.3,  :swap_byte],
   [ 20, 0.95, :swap_byte]],

  Simple_Sexp_Op =>
  # PRIORITY, PROC
  [[50, :var_collapse_all],
   [50, :var_collapse_positional],
   [50, :var_collapse_rand_3]],

  Random_Sexp_Op =>
  # PRIORITY, PROB, PROC
  [[100, 0.1,  :num_edge_cases],
   [200, 0.5,  :num_edge_cases],
   [100, 0.95, :num_edge_cases]],

  Random_Sexp_Pairing =>
  # PRIORITY, PROB, PROC
  [[100, 0.1, :crossover],
   [200, 0.3, :crossover],
   [200, 0.5, :crossover],
   [100, 0.9, :crossover],

   [ 20, 0.1, :single_crossover],
   [ 20, 0.3, :single_crossover],
   [100, 0.5, :single_crossover],
   [ 20, 0.9, :single_crossover]]
}

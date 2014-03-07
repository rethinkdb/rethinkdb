//=======================================================================
// Copyright 1997, 1998, 1999, 2000 University of Notre Dame.
// Authors: Andrew Lumsdaine, Lie-Quan Lee, Jeremy G. Siek
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
#include <boost/config.hpp>
#include <vector>
#include <iostream>

#include <stdlib.h>
#include <boost/pending/bucket_sorter.hpp>

template <class Integral>
struct trivial_id {
  std::size_t operator[](Integral i) {
    return i;
  }
  std::size_t operator[](Integral i) const {
    return i;
  }
};


int main() {
  using namespace std;
  using boost::bucket_sorter;
  
  const std::size_t N = 10;

  vector<std::size_t> bucket(N);
  for (std::size_t i=0; i<N; i++) {
    bucket[i] = rand() % N;
    cout.width(6);
    cout << "Number " << i << " has its bucket "  << bucket[i] << endl;
  }

  typedef trivial_id<int> ID;
  typedef bucket_sorter<std::size_t, int, 
    vector<std::size_t>::iterator, ID> BS;
  BS my_bucket_sorter(N, N, bucket.begin());

  for (std::size_t ii=0; ii<N; ii++)
    my_bucket_sorter.push(ii);
    
  std::size_t j;
  for (j=0; j<N; j++) {
    cout << "The bucket " << j;
    if ( ! my_bucket_sorter[j].empty() ) {
      cout << " has number ";
      do {
        int v = my_bucket_sorter[j].top();
        my_bucket_sorter[j].pop();
        cout << v << " ";
      } while ( ! my_bucket_sorter[j].empty() );
      cout << endl;
    } else {
      cout << " has no number associated with." << endl;
    }
  }

  for (std::size_t k=0; k<N; k++)
    my_bucket_sorter.push(k);

  my_bucket_sorter.remove(5);
  my_bucket_sorter.remove(7);

  cout << "Afer remove number 5 and 7, check correctness again." << endl;

  for (j=0; j<N; j++) {
    cout << "The bucket " << j;
    if ( ! my_bucket_sorter[j].empty() ) {
      cout << " has number ";
      do {
        int v = my_bucket_sorter[j].top();
        my_bucket_sorter[j].pop();
        cout << v << " ";
      } while ( ! my_bucket_sorter[j].empty() );
      cout << endl;
    } else {
      cout << " has no number associated with." << endl;
    }
  }

  std::size_t iii;
  for (iii=0; iii<N; iii++) {
    std::size_t current = rand() % N;
    if ( ! my_bucket_sorter[current].empty() ) {
      int v = my_bucket_sorter[current].top();
      my_bucket_sorter[current].pop();
      bucket[v] = rand() % N;
      my_bucket_sorter.push(v);
    }
  }

  for (iii=0; iii<N; iii++) {
    std::size_t current = rand() % N;
    if ( ! my_bucket_sorter[current].empty() ) {
      int v = my_bucket_sorter[current].top();
      bucket[v] = rand() % N;
      my_bucket_sorter.update(v);
    }
  }

    return 0;
}

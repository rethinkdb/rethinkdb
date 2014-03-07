.. Copyright David Abrahams 2006. Distributed under the Boost
.. Software License, Version 1.0. (See accompanying
.. file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

::

    using namespace boost;
    int i = 0;

    typedef std::vector< int > element_range_type;
    typedef std::list< int > index_type;

    static const int element_range_size = 10;
    static const int index_size = 4;

    element_range_type elements( element_range_size );
    for(element_range_type::iterator el_it = elements.begin() ; el_it != elements.end() ; ++el_it)
      *el_it = std::distance(elements.begin(), el_it);

    index_type indices( index_size );
    for(index_type::iterator i_it = indices.begin() ; i_it != indices.end() ; ++i_it ) 
      *i_it = element_range_size - index_size + std::distance(indices.begin(), i_it);
    std::reverse( indices.begin(), indices.end() );

    typedef permutation_iterator< element_range_type::iterator, index_type::iterator > permutation_type;
    permutation_type begin = make_permutation_iterator( elements.begin(), indices.begin() );
    permutation_type it = begin;
    permutation_type end = make_permutation_iterator( elements.begin(), indices.end() );

    std::cout << "The original range is : ";
    std::copy( elements.begin(), elements.end(), std::ostream_iterator< int >( std::cout, " " ) );
    std::cout << "\n";

    std::cout << "The reindexing scheme is : ";
    std::copy( indices.begin(), indices.end(), std::ostream_iterator< int >( std::cout, " " ) );
    std::cout << "\n";

    std::cout << "The permutated range is : ";
    std::copy( begin, end, std::ostream_iterator< int >( std::cout, " " ) );
    std::cout << "\n";

    std::cout << "Elements at even indices in the permutation : ";
    it = begin;
    for(i = 0; i < index_size / 2 ; ++i, it+=2 ) std::cout << *it << " ";
    std::cout << "\n";

    std::cout << "Permutation backwards : ";
    it = begin + (index_size);
    assert( it != begin );
    for( ; it-- != begin ; ) std::cout << *it << " ";
    std::cout << "\n";

    std::cout << "Iterate backward with stride 2 : ";
    it = begin + (index_size - 1);
    for(i = 0 ; i < index_size / 2 ; ++i, it-=2 ) std::cout << *it << " ";
    std::cout << "\n";


The output is::

    The original range is : 0 1 2 3 4 5 6 7 8 9 
    The reindexing scheme is : 9 8 7 6 
    The permutated range is : 9 8 7 6 
    Elements at even indices in the permutation : 9 7 
    Permutation backwards : 6 7 8 9 
    Iterate backward with stride 2 : 6 8 


The source code for this example can be found `here`__.

__ ../example/permutation_iter_example.cpp

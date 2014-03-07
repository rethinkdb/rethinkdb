++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++
 
.. |Boost| image:: boost.png

Insert Iterators
----------------

When you work with normal value-based containers and algorithms, you often
use insert iterators ::

       std::list<int> coll1;
       // ...
       std::vector<int> coll2;
       std::copy( coll1.begin(), coll1.end(),  
                  back_inserter(coll2) ); 

With the special insert iterators for pointer containers, 
you can do exactly the same ::
            
       boost::ptr_list<Base> coll1;
       // ...
       boost::ptr_vector<Base> coll2;
       std::copy( coll1.begin(), coll1.end(),  
                  boost::ptr_container::ptr_back_inserter(coll2) ); 

Each element is cloned and inserted into the container. Furthermore, 
if the source range iterates over pointers 
instead of references, ``NULL`` pointers
can be transfered as well.
 
**Navigate**

- `home <ptr_container.html>`_
- `reference <reference.html>`_
    
**Synopsis:**

::  
           
        namespace boost
        {      
            namespace ptr_container
            {
            
                template< class PtrContainer >
                class ptr_back_insert_iterator;
                
                template< class PtrContainer >
                class ptr_front_insert_iterator;
                
                template< class PtrContainer >
                class ptr_insert_iterator;
                
                template< class PtrContainer >
                ptr_back_insert_iterator<PtrContainer> 
                ptr_back_inserter( PtrContainer& cont );
                
                template< class PtrContainer >
                ptr_front_insert_iterator<PtrContainer> 
                ptr_front_inserter( PtrContainer& cont );
                
                template< class PtrContainer >
                ptr_insert_iterator<PtrContainer> 
                ptr_inserter( PtrContainer& cont, typename PtrContainer::iterator before );
                 
            } // namespace 'ptr_container'
        } // namespace 'boost'  
        
.. raw:: html 

        <hr>

:Copyright:     Thorsten Ottosen 2008. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt



++++++++++++++++++++++++++++++++++
 |Boost| Pointer Container Library
++++++++++++++++++++++++++++++++++
 
.. |Boost| image:: boost.png

Indirected functions
--------------------

It is quite common that we have two pointers and what to compare the
pointed to objects. Also, we have usually already defined how
to compare the objects. So to avoid some tedious boiler-plate code
this library defines predicates that apply an indirection before comparing.

When the container uses ``void*`` internally, we can use the
class ``void_ptr_indirect_fun``; otherwise we use the class
``indirect_fun``. 

**Example:** ::

    std::string* bar = new std::string("bar");
    std::string* foo = new std::string("foo");
    BOOST_ASSERT( indirect_fun< std::less<std::string> >()( bar, foo ) == true );
    BOOST_ASSERT( make_indirect_fun( std::less<std::string>() )( foo, bar ) == false );   

    void*       vptr1  = ptr1;
    void*       vptr2  = ptr2;
    void_ptr_indirect_fun< std::less<std::string>, std::string> cast_fun;
    BOOST_CHECK( cast_fun( vptr1, vptr2 ) == true );
 
**See also:**

- `result_of <http://www.boost.org/libs/utility/utility.htm#result_of>`_
- `pointee <http://www.boost.org/libs/iterator/doc/pointee.html>`_
- `ptr_set <ptr_set.html>`_
- `ptr_multiset <ptr_multiset.html>`_

**Navigate**

- `home <ptr_container.html>`_
- `reference <reference.html>`_
    
**Remarks:**

The class ``indirect_fun`` will work with smart pointers such as `boost::shared_ptr<T> <http://www.boost.org/libs/smart_ptr/shared_ptr.htm>`_
because of the type traits ``pointee<T>::type`` from the header ``<boost/pointee.hpp>``.
  
**Synopsis:**

Since the definition of the predicates is somewhat trivial, only the
first operation is expanded inline.

::  
           
        namespace boost
        {      

            template< class Fun >
            struct indirect_fun
            {
                indirect_fun() : fun(Fun())
                { }
                
                indirect_fun( Fun f ) : fun(f)
                { }
            
                template< class T >
                typename result_of< Fun( typename pointee<T>::type ) >::type 
                operator()( const T& r ) const
                { 
                    return fun( *r );
                }
            
                template< class T, class U >
                typename result_of< Fun( typename pointee<T>::type, 
                                         typename pointee<U>::type ) >::type 
                operator()( const T& r, const U& r2 ) const
                { 
                    return fun( *r, *r2 );
                }
            
            private:
                Fun fun;
            };
        
            template< class Fun >
            inline indirect_fun<Fun> make_indirect_fun( Fun f )
            {
                return indirect_fun<Fun>( f );
            }        



            template< class Fun, class Arg1, class Arg2 = Arg1 >
            struct void_ptr_indirect_fun
            {
                void_ptr_indirect_fun() : fun(Fun())
                { }
        
                void_ptr_indirect_fun( Fun f ) : fun(f)
                { }
        
                typename result_of< Fun( Arg1 ) >::type 
                operator()( const void* r ) const
                { 
                    return fun( * static_cast<const Arg1*>( r ) );
                }
        
                typename result_of< Fun( Arg1, Arg2 ) >::type 
                operator()( const void* l, const void* r ) const
                { 
                    return fun( * static_cast<const Arg1*>( l ), * static_cast<const Arg2*>( r ) );
                }
                
            private:
                Fun fun;   
            };
        
            template< class Fun, class Arg >
            inline void_ptr_indirect_fun<Fun,Arg> 
            make_void_ptr_indirect_fun( Fun f )
            {
                return void_ptr_indirect_fun<Fun,Arg>( f );
            }
                 
        } // namespace 'boost'  
        
.. raw:: html 

        <hr>

:Copyright:     Thorsten Ottosen 2004-2006. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt



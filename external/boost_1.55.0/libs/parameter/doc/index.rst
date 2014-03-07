+++++++++++++++++++++++++++++++++++++++++++++++++
 The Boost Parameter Library 
+++++++++++++++++++++++++++++++++++++++++++++++++

|(logo)|__

.. |(logo)| image:: ../../../../boost.png
   :alt: Boost

__ ../../../../index.htm

-------------------------------------

:Abstract: Use this library to write functions and class templates
  that can accept arguments by name:

  .. parsed-literal::

    new_window("alert", **_width=10**, **_titlebar=false**);

    smart_ptr<
       Foo 
     , **deleter<Deallocate<Foo> >**
     , **copy_policy<DeepCopy>** > p(new Foo);
    
  Since named arguments can be passed in any order, they are
  especially useful when a function or template has more than one
  parameter with a useful default value.  The library also supports
  *deduced* parameters; that is to say, parameters whose identity
  can be deduced from their types.

.. @jam_prefix.append('''
        project test : requirements <include>. <implicit-dependency>/boost//headers ;''')

.. @example.prepend('''
   #include <boost/parameter.hpp>
   
   namespace test
   {
     BOOST_PARAMETER_NAME(title)
     BOOST_PARAMETER_NAME(width)
     BOOST_PARAMETER_NAME(titlebar)
   
     BOOST_PARAMETER_FUNCTION(
        (int), new_window, tag, (required (title,*)(width,*)(titlebar,*)))
     {
        return 0;
     }
     
     BOOST_PARAMETER_TEMPLATE_KEYWORD(deleter)
     BOOST_PARAMETER_TEMPLATE_KEYWORD(copy_policy)

     template <class T> struct Deallocate {};
     struct DeepCopy {};

     namespace parameter = boost::parameter;
     
     struct Foo {};
     template <class T, class A0, class A1>
     struct smart_ptr
     {
         smart_ptr(Foo*);
     };
   }
   using namespace test;
   int x = ''');

.. @test('compile')


-------------------------------------

:Authors:       David Abrahams, Daniel Wallin
:Contact:       dave@boost-consulting.com, daniel@boostpro.com
:organization:  `BoostPro Computing`_
:date:          $Date: 2005/07/17 19:53:01 $

:copyright:     Copyright David Abrahams, Daniel Wallin
                2005-2009. Distributed under the Boost Software License,
                Version 1.0. (See accompanying file LICENSE_1_0.txt
                or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _`BoostPro Computing`: http://www.boostpro.com

.. _concepts: http://www.boost.org/more/generic_programming.html#concept

-------------------------------------

[Note: this tutorial does not cover all details of the library.  Please see also the `reference documentation`__\ ]

__ reference.html

.. contents:: **Table of Contents**
   :depth: 2

.. role:: concept
   :class: concept

.. role:: vellipsis
   :class: vellipsis

.. section-numbering::

-------------------------------------

============
 Motivation
============

In C++, arguments_ are normally given meaning by their positions
with respect to a parameter_ list: the first argument passed maps
onto the first parameter in a function's definition, and so on.
That protocol is fine when there is at most one parameter with a
default value, but when there are even a few useful defaults, the
positional interface becomes burdensome:

* .. compound::

    Since an argument's meaning is given by its position, we have to
    choose an (often arbitrary) order for parameters with default
    values, making some combinations of defaults unusable:

    .. parsed-literal::

      window* new_window(
         char const* name, 
         **int border_width = default_border_width,**
         bool movable = true,
         bool initially_visible = true
         );

      const bool movability = false;
      window* w = new_window("alert box", movability);

    In the example above we wanted to make an unmoveable window
    with a default ``border_width``, but instead we got a moveable
    window with a ``border_width`` of zero.  To get the desired
    effect, we'd need to write:

    .. parsed-literal::

       window* w = new_window(
          "alert box", **default_border_width**, movability);

* .. compound::

    It can become difficult for readers to understand the meaning of
    arguments at the call site::

      window* w = new_window("alert", 1, true, false);

    Is this window moveable and initially invisible, or unmoveable
    and initially visible?  The reader needs to remember the order
    of arguments to be sure.  

* The author of the call may not remember the order of the
  arguments either, leading to hard-to-find bugs.

.. @ignore(3)

-------------------------
Named Function Parameters
-------------------------

.. compound::

  This library addresses the problems outlined above by associating
  each parameter name with a keyword object.  Now users can identify
  arguments by name, rather than by position:

  .. parsed-literal::

    window* w = new_window("alert box", **movable_=**\ false); // OK!

.. @ignore()

---------------------------
Deduced Function Parameters
---------------------------

.. compound::

  A **deduced parameter** can be passed in any position *without*
  supplying an explicit parameter name.  It's not uncommon for a
  function to have parameters that can be uniquely identified based
  on the types of arguments passed.  The ``name`` parameter to
  ``new_window`` is one such example.  None of the other arguments,
  if valid, can reasonably be converted to a ``char const*``.  With
  a deduced parameter interface, we could pass the window name in
  *any* argument position without causing ambiguity:

  .. parsed-literal::

    window* w = new_window(movable_=false, **"alert box"**); // OK!
    window* w = new_window(**"alert box"**, movable_=false); // OK!

  Appropriately used, a deduced parameter interface can free the
  user of the burden of even remembering the formal parameter
  names.

.. @ignore()

--------------------------------
Class Template Parameter Support
--------------------------------

.. compound::

  The reasoning we've given for named and deduced parameter
  interfaces applies equally well to class templates as it does to
  functions.  Using the Parameter library, we can create interfaces
  that allow template arguments (in this case ``shared`` and
  ``Client``) to be explicitly named, like this:

  .. parsed-literal::

    smart_ptr<**ownership<shared>**, **value_type<Client>** > p;

  The syntax for passing named template arguments is not quite as
  natural as it is for function arguments (ideally, we'd be able to
  write ``smart_ptr<ownership=shared,…>``).  This small syntactic
  deficiency makes deduced parameters an especially big win when
  used with class templates:

  .. parsed-literal::

    // *p and q could be equivalent, given a deduced*
    // *parameter interface.*
    smart_ptr<**shared**, **Client**> p;
    smart_ptr<**Client**, **shared**> q;

.. @ignore(2)

==========
 Tutorial
==========

This tutorial shows all the basics—how to build both named- and deduced-parameter
interfaces to function templates and class templates—and several
more advanced idioms as well.

---------------------------
Parameter-Enabled Functions
---------------------------

In this section we'll show how the Parameter library can be used to
build an expressive interface to the `Boost Graph library`__\ 's
|dfs|_ algorithm. [#old_interface]_ 

.. Revisit this

  After laying some groundwork
  and describing the algorithm's abstract interface, we'll show you
  how to build a basic implementation with keyword support.  Then
  we'll add support for default arguments and we'll gradually refine the
  implementation with syntax improvements.  Finally we'll show how to
  streamline the implementation of named parameter interfaces,
  improve their participation in overload resolution, and optimize
  their runtime efficiency.

__ ../../../graph/index.html

.. _dfs: ../../../graph/doc/depth_first_search.html

.. |dfs| replace:: ``depth_first_search``


Headers And Namespaces
======================

Most components of the Parameter library are declared in a
header named for the component.  For example, ::

  #include <boost/parameter/keyword.hpp>

will ensure ``boost::parameter::keyword`` is known to the
compiler.  There is also a combined header,
``boost/parameter.hpp``, that includes most of the library's
components.  For the the rest of this tutorial, unless we say
otherwise, you can use the rule above to figure out which header
to ``#include`` to access any given component of the library.

.. @example.append('''
   using boost::parameter::keyword;
   ''')

.. @test('compile')

Also, the examples below will also be written as if the
namespace alias ::

  namespace parameter = boost::parameter;

.. @ignore()

has been declared: we'll write ``parameter::xxx`` instead of
``boost::parameter::xxx``.

The Abstract Interface to |dfs|
===============================

The Graph library's |dfs| algorithm is a generic function accepting
from one to four arguments by reference.  If all arguments were
required, its signature might be as follows::

   template <
       class Graph, class DFSVisitor, class Index, class ColorMap
   >
   void depth_first_search(
     , Graph const& graph 
     , DFSVisitor visitor
     , typename graph_traits<g>::vertex_descriptor root_vertex
     , IndexMap index_map
     , ColorMap& color);

.. @ignore()

However, most of the parameters have a useful default value, as
shown in the table below.

.. _`parameter table`: 
.. _`default expressions`: 

.. table:: ``depth_first_search`` Parameters

  +----------------+----------+---------------------------------+----------------------------------+
  | Parameter Name | Dataflow | Type                            | Default Value (if any)           |
  +================+==========+=================================+==================================+
  |``graph``       | in       |Model of |IncidenceGraph|_ and   |none - this argument is required. |
  |                |          ||VertexListGraph|_               |                                  |
  |                |          |                                 |                                  |
  +----------------+----------+---------------------------------+----------------------------------+
  |``visitor``     | in       |Model of |DFSVisitor|_           |``boost::dfs_visitor<>()``        |
  +----------------+----------+---------------------------------+----------------------------------+
  |``root_vertex`` | in       |``graph``'s vertex descriptor    |``*vertices(graph).first``        |
  |                |          |type.                            |                                  |
  +----------------+----------+---------------------------------+----------------------------------+
  |``index_map``   | in       |Model of |ReadablePropertyMap|_  |``get(boost::vertex_index,graph)``|
  |                |          |with key type := ``graph``'s     |                                  |
  |                |          |vertex descriptor and value type |                                  |
  |                |          |an integer type.                 |                                  |
  +----------------+----------+---------------------------------+----------------------------------+
  |``color_map``   | in/out   |Model of |ReadWritePropertyMap|_ |an ``iterator_property_map``      |
  |                |          |with key type := ``graph``'s     |created from a ``std::vector`` of |
  |                |          |vertex descriptor type.          |``default_color_type`` of size    |
  |                |          |                                 |``num_vertices(graph)`` and using |
  |                |          |                                 |``index_map`` for the index map.  |
  +----------------+----------+---------------------------------+----------------------------------+

.. |IncidenceGraph| replace:: :concept:`Incidence Graph`
.. |VertexListGraph| replace:: :concept:`Vertex List Graph`
.. |DFSVisitor| replace:: :concept:`DFS Visitor`
.. |ReadablePropertyMap| replace:: :concept:`Readable Property Map`
.. |ReadWritePropertyMap| replace:: :concept:`Read/Write Property Map`

.. _`IncidenceGraph`: ../../../graph/doc/IncidenceGraph.html
.. _`VertexListGraph`: ../../../graph/doc/VertexListGraph.html
.. _`DFSVisitor`: ../../../graph/doc/DFSVisitor.html
.. _`ReadWritePropertyMap`: ../../../property_map/doc/ReadWritePropertyMap.html
.. _`ReadablePropertyMap`: ../../../property_map/doc/ReadablePropertyMap.html

Don't be intimidated by the information in the second and third
columns above.  For the purposes of this exercise, you don't need
to understand them in detail.

Defining the Keywords
=====================

The point of this exercise is to make it possible to call
``depth_first_search`` with named arguments, leaving out any
arguments for which the default is appropriate:

.. parsed-literal::

  graphs::depth_first_search(g, **color_map_=my_color_map**);

.. @ignore()

To make that syntax legal, there needs to be an object called
“\ ``color_map_``\ ” whose assignment operator can accept a
``my_color_map`` argument.  In this step we'll create one such
**keyword object** for each parameter.  Each keyword object will be
identified by a unique **keyword tag type**.  

.. Revisit this

  We're going to define our interface in namespace ``graphs``.  Since
  users need access to the keyword objects, but not the tag types,
  we'll define the keyword objects so they're accessible through
  ``graphs``, and we'll hide the tag types away in a nested
  namespace, ``graphs::tag``.  The library provides a convenient
  macro for that purpose.

We're going to define our interface in namespace ``graphs``.  The
library provides a convenient macro for defining keyword objects::

  #include <boost/parameter/name.hpp>

  namespace graphs
  {
    BOOST_PARAMETER_NAME(graph)    // Note: no semicolon
    BOOST_PARAMETER_NAME(visitor)
    BOOST_PARAMETER_NAME(root_vertex)
    BOOST_PARAMETER_NAME(index_map)
    BOOST_PARAMETER_NAME(color_map)
  }

.. @test('compile')

The declaration of the ``graph`` keyword you see here is
equivalent to::

  namespace graphs 
  {
    namespace tag { struct graph; } // keyword tag type

    namespace // unnamed
    {
      // A reference to the keyword object
      boost::parameter::keyword<tag::graph>& _graph
      = boost::parameter::keyword<tag::graph>::get();
    }
  }

.. @example.prepend('#include <boost/parameter/keyword.hpp>')
.. @test('compile')

It defines a *keyword tag type* named ``tag::graph`` and a *keyword
object* reference named ``_graph``.

This “fancy dance” involving an unnamed namespace and references
is all done to avoid violating the One Definition Rule (ODR)
[#odr]_ when the named parameter interface is used by function
templates that are instantiated in multiple translation
units (MSVC6.x users see `this note`__).

__ `Compiler Can't See References In Unnamed Namespace`_

Writing the Function
====================

Now that we have our keywords defined, the function template
definition follows a simple pattern using the
``BOOST_PARAMETER_FUNCTION`` macro::

  #include <boost/parameter/preprocessor.hpp>

  namespace graphs
  {
    BOOST_PARAMETER_FUNCTION(
        (void),                // 1. parenthesized return type
        depth_first_search,    // 2. name of the function template

        tag,                   // 3. namespace of tag types

        (required (graph, *) ) // 4. one required parameter, and

        (optional              //    four optional parameters, with defaults
          (visitor,           *, boost::dfs_visitor<>()) 
          (root_vertex,       *, *vertices(graph).first) 
          (index_map,         *, get(boost::vertex_index,graph)) 
          (in_out(color_map), *, 
            default_color_map(num_vertices(graph), index_map) ) 
        )
    )
    {
        // ... body of function goes here...
        // use graph, visitor, index_map, and color_map
    }
  }

.. @example.prepend('''
   #include <boost/parameter/name.hpp>

   BOOST_PARAMETER_NAME(graph)
   BOOST_PARAMETER_NAME(visitor)
   BOOST_PARAMETER_NAME(root_vertex)
   BOOST_PARAMETER_NAME(index_map)
   BOOST_PARAMETER_NAME(color_map)

   namespace boost {

   template <class T = int>
   struct dfs_visitor
   {};

   int vertex_index = 0;

   }''')

.. @test('compile')

The arguments to ``BOOST_PARAMETER_FUNCTION`` are:

1. The return type of the resulting function template.  Parentheses
   around the return type prevent any commas it might contain from
   confusing the preprocessor, and are always required.

2. The name of the resulting function template.

3. The name of a namespace where we can find tag types whose names
   match the function's parameter names.

4. The function signature.  

Function Signatures
===================

Function signatures are described as one or two adjacent
parenthesized terms (a Boost.Preprocessor_ sequence_) describing
the function's parameters in the order in which they'd be expected
if passed positionally.  Any required parameters must come first,
but the ``(required … )`` clause can be omitted when all the
parameters are optional.

.. _Boost.Preprocessor: ../../../preprocessor/index.html

Required Parameters
-------------------

.. compound::

  Required parameters are given first—nested in a ``(required … )``
  clause—as a series of two-element tuples describing each parameter
  name and any requirements on the argument type.  In this case there
  is only a single required parameter, so there's just a single
  tuple:

  .. parsed-literal::

     (required **(graph, \*)** )

  Since ``depth_first_search`` doesn't require any particular type
  for its ``graph`` parameter, we use an asterix to indicate that
  any type is allowed.  Required parameters must always precede any
  optional parameters in a signature, but if there are *no*
  required parameters, the ``(required … )`` clause can be omitted
  entirely.

.. @example.prepend('''
   #include <boost/parameter.hpp>

   BOOST_PARAMETER_NAME(graph)

   BOOST_PARAMETER_FUNCTION((void), f, tag,
   ''')

.. @example.append(') {}')
.. @test('compile')

Optional Parameters
-------------------

.. compound::

  Optional parameters—nested in an ``(optional … )`` clause—are given
  as a series of adjacent *three*\ -element tuples describing the
  parameter name, any requirements on the argument type, *and* and an
  expression representing the parameter's default value:

  .. parsed-literal::

    (optional **\
        (visitor,           \*, boost::dfs_visitor<>()) 
        (root_vertex,       \*, \*vertices(graph).first) 
        (index_map,         \*, get(boost::vertex_index,graph)) 
        (in_out(color_map), \*, 
          default_color_map(num_vertices(graph), index_map) )**
    )

.. @example.prepend('''
   #include <boost/parameter.hpp>

   namespace boost
   {
     int vertex_index = 0;

     template <class T = int>
     struct dfs_visitor
     {};
   }

   BOOST_PARAMETER_NAME(graph)
   BOOST_PARAMETER_NAME(visitor)
   BOOST_PARAMETER_NAME(root_vertex)
   BOOST_PARAMETER_NAME(index_map)
   BOOST_PARAMETER_NAME(color_map)

   BOOST_PARAMETER_FUNCTION((void), f, tag,
     (required (graph, *))
   ''')

.. @example.append(') {}')
.. @test('compile')

Handling “Out” Parameters
-------------------------

.. compound::

  Within the function body, a parameter name such as ``visitor`` is
  a *C++ reference*, bound either to an actual argument passed by
  the caller or to the result of evaluating a default expression.
  In most cases, parameter types are of the form ``T const&`` for
  some ``T``.  Parameters whose values are expected to be modified,
  however, must be passed by reference to *non*\ -``const``.  To
  indicate that ``color_map`` is both read and written, we wrap
  its name in ``in_out(…)``:

  .. parsed-literal::

    (optional
        (visitor,            \*, boost::dfs_visitor<>()) 
        (root_vertex,        \*, \*vertices(graph).first) 
        (index_map,          \*, get(boost::vertex_index,graph)) 
        (**in_out(color_map)**, \*, 
          default_color_map(num_vertices(graph), index_map) )
    )

.. @example.prepend('''
   #include <boost/parameter.hpp>

   namespace boost
   {
     int vertex_index = 0;

     template <class T = int>
     struct dfs_visitor
     {};
   }

   BOOST_PARAMETER_NAME(graph)

   BOOST_PARAMETER_NAME(visitor)
   BOOST_PARAMETER_NAME(root_vertex)
   BOOST_PARAMETER_NAME(index_map)
   BOOST_PARAMETER_NAME(color_map)

   BOOST_PARAMETER_FUNCTION((void), f, tag,
     (required (graph, *))
   ''')

.. @example.append(') {}')
.. @test('compile')

If ``color_map`` were strictly going to be modified but not examined,
we could have written ``out(color_map)``.  There is no functional
difference between ``out`` and ``in_out``; the library provides
both so you can make your interfaces more self-documenting.

Positional Arguments
--------------------

When arguments are passed positionally (without the use of
keywords), they will be mapped onto parameters in the order the
parameters are given in the signature, so for example in this
call ::

  graphs::depth_first_search(x, y);

.. @ignore()

``x`` will always be interpreted as a graph and ``y`` will always
be interpreted as a visitor.

.. _sequence: http://boost-consulting.com/mplbook/preprocessor.html#sequences

Default Expression Evaluation
-----------------------------

.. compound::

  Note that in our example, the value of the graph parameter is
  used in the default expressions for ``root_vertex``,
  ``index_map`` and ``color_map``.  

  .. parsed-literal::

        (required (**graph**, \*) )
        (optional
          (visitor,           \*, boost::dfs_visitor<>()) 
          (root_vertex,       \*, \*vertices(**graph**).first) 
          (index_map,         \*, get(boost::vertex_index,\ **graph**)) 
          (in_out(color_map), \*, 
            default_color_map(num_vertices(**graph**), index_map) ) 
        )

  .. @ignore()

  A default expression is evaluated in the context of all preceding
  parameters, so you can use any of their values by name.

.. compound::

  A default expression is never evaluated—or even instantiated—if
  an actual argument is passed for that parameter.  We can actually
  demonstrate that with our code so far by replacing the body of
  ``depth_first_search`` with something that prints the arguments:

  .. parsed-literal::

    #include <boost/graph/depth_first_search.hpp> // for dfs_visitor

    BOOST_PARAMETER_FUNCTION(
        (void), depth_first_search, tag
        *…signature goes here…*
    )
    {
       std::cout << "graph=" << graph << std::endl;
       std::cout << "visitor=" << visitor << std::endl;
       std::cout << "root_vertex=" << root_vertex << std::endl;
       std::cout << "index_map=" << index_map << std::endl;
       std::cout << "color_map=" << color_map << std::endl;
    }

    int main()
    {
        depth_first_search(1, 2, 3, 4, 5);

        depth_first_search(
            "1", '2', _color_map = '5',
            _index_map = "4", _root_vertex = "3");
    }

  Despite the fact that default expressions such as
  ``vertices(graph).first`` are ill-formed for the given ``graph``
  arguments, both calls will compile, and each one will print
  exactly the same thing.

.. @example.prepend('''
   #include <boost/parameter.hpp>
   #include <iostream>

   BOOST_PARAMETER_NAME(graph)
   BOOST_PARAMETER_NAME(visitor)
   BOOST_PARAMETER_NAME(root_vertex)
   BOOST_PARAMETER_NAME(index_map)
   BOOST_PARAMETER_NAME(color_map)''')

.. @example.replace_emphasis('''
   , (required 
       (graph, *)
       (visitor, *)
       (root_vertex, *)
       (index_map, *)
       (color_map, *)
     )
   ''')
.. @test('compile')

Signature Matching and Overloading
----------------------------------

In fact, the function signature is so general that any call to
``depth_first_search`` with fewer than five arguments will match
our function, provided we pass *something* for the required
``graph`` parameter.  That might not seem to be a problem at first;
after all, if the arguments don't match the requirements imposed by
the implementation of ``depth_first_search``, a compilation error
will occur later, when its body is instantiated.

There are at least three problems with very general function
signatures.  

1. By the time our ``depth_first_search`` is instantiated, it has
   been selected as the best matching overload.  Some other
   ``depth_first_search`` overload might've worked had it been
   chosen instead.  By the time we see a compilation error, there's
   no chance to change that decision.

2. Even if there are no overloads, error messages generated at
   instantiation time usually expose users to confusing
   implementation details.  For example, users might see references
   to names generated by ``BOOST_PARAMETER_FUNCTION`` such as
   ``graphs::detail::depth_first_search_with_named_params`` (or
   worse—think of the kinds of errors you get from your STL
   implementation when you make a mistake). [#ConceptCpp]_

3. The problems with exposing such permissive function template
   signatures have been the subject of much discussion, especially
   in the presence of `unqualified calls`__.  If all we want is to
   avoid unintentional argument-dependent lookup (ADL), we can
   isolate ``depth_first_search`` in a namespace containing no
   types [#using]_, but suppose we *want* it to found via ADL?

__ http://anubis.dkuug.dk/jtc1/sc22/wg21/docs/lwg-defects.html#225

It's usually a good idea to prevent functions from being considered
for overload resolution when the passed argument types aren't
appropriate.  The library already does this when the required
``graph`` parameter is not supplied, but we're not likely to see a
depth first search that doesn't take a graph to operate on.
Suppose, instead, that we found a different depth first search
algorithm that could work on graphs that don't model
|IncidenceGraph|_?  If we just added a simple overload,
it would be ambiguous::

  // new overload
  BOOST_PARAMETER_FUNCTION(
      (void), depth_first_search, (tag), (required (graph,*))( … ))
  {
      // new algorithm implementation
  }

  …

  // ambiguous!
  depth_first_search(boost::adjacency_list<>(), 2, "hello");

.. @ignore()

Adding Type Requirements
........................

We really don't want the compiler to consider the original version
of ``depth_first_search`` because the ``root_vertex`` argument,
``"hello"``, doesn't meet the requirement__ that it match the
``graph`` parameter's vertex descriptor type.  Instead, this call
should just invoke our new overload.  To take the original
``depth_first_search`` overload out of contention, we need to tell
the library about this requirement by replacing the ``*`` element
of the signature with the required type, in parentheses:

__ `parameter table`_

.. parsed-literal::

  (root_vertex,       
       **(typename boost::graph_traits<graph_type>::vertex_descriptor)**,
       \*vertices(graph).first) 

.. @ignore()

Now the original ``depth_first_search`` will only be called when
the ``root_vertex`` argument can be converted to the graph's vertex
descriptor type, and our example that *was* ambiguous will smoothly
call the new overload.

.. Note:: The *type* of the ``graph`` argument is available in the
   signature—and in the function body—as ``graph_type``.  In
   general, to access the type of any parameter *foo*, write *foo*\
   ``_type``.


Predicate Requirements
......................

The requirements on other arguments are a bit more interesting than
those on ``root_vertex``; they can't be described in terms of simple
type matching.  Instead, they must be described in terms of `MPL
Metafunctions`__.  There's no space to give a complete description
of metafunctions or of graph library details here, but we'll show
you the complete signature with maximal checking, just to give you
a feel for how it's done.  Each predicate metafunction is enclosed
in parentheses *and preceded by an asterix*, as follows:

.. parsed-literal::

    BOOST_PARAMETER_FUNCTION(
        (void), depth_first_search, graphs

      , (required 
          (graph 
           , **\ \*(boost::mpl::and_<
                   boost::is_convertible<
                       boost::graph_traits<_>::traversal_category
                     , boost::incidence_graph_tag
                   >
                 , boost::is_convertible<
                       boost::graph_traits<_>::traversal_category
                     , boost::vertex_list_graph_tag
                   >
               >)** ))

        (optional
          (visitor, \*, boost::dfs_visitor<>()) // not checkable

          (root_vertex
            , (typename boost::graph_traits<graphs::graph::_>::vertex_descriptor)
            , \*vertices(graph).first)
 
          (index_map
            , **\ \*(boost::mpl::and_<
                  boost::is_integral<
                      boost::property_traits<_>::value_type
                  >
                , boost::is_same<
                      typename boost::graph_traits<graphs::graph::_>::vertex_descriptor
                    , boost::property_traits<_>::key_type
                  >
              >)**
            , get(boost::vertex_index,graph))
 
          (in_out(color_map)
            , **\ \*(boost::is_same<
                  typename boost::graph_traits<graphs::graph::_>::vertex_descriptor
                , boost::property_traits<_>::key_type
              >)**
           , default_color_map(num_vertices(graph), index_map) ) 
        )
    )

.. @example.prepend('''
   #include <boost/parameter.hpp>

   BOOST_PARAMETER_NAME((_graph, graphs) graph) 
   BOOST_PARAMETER_NAME((_visitor, graphs) visitor) 
   BOOST_PARAMETER_NAME((_root_vertex, graphs) root_vertex) 
   BOOST_PARAMETER_NAME((_index_map, graphs) index_map) 
   BOOST_PARAMETER_NAME((_color_map, graphs) color_map)

   using boost::mpl::_;

   namespace boost
   {
     struct incidence_graph_tag {};
     struct vertex_list_graph_tag {};

     int vertex_index = 0;

     template <class T>
     struct graph_traits
     {
         typedef int traversal_category;
         typedef int vertex_descriptor;
     };

     template <class T>
     struct property_traits
     {
         typedef int value_type;
         typedef int key_type;
     };

     template <class T = int>
     struct dfs_visitor 
     {};
   }''')

.. @example.append('''
   {}''')

.. @test('compile')

__ ../../../mpl/doc/refmanual/metafunction.html

We acknowledge that this signature is pretty hairy looking.
Fortunately, it usually isn't necessary to so completely encode the
type requirements on arguments to generic functions.  However, it
is usally worth the effort to do so: your code will be more
self-documenting and will often provide a better user experience.
You'll also have an easier transition to an upcoming C++ standard
with `language support for concepts`__.

__ `ConceptC++`_

Deduced Parameters
------------------

To illustrate deduced parameter support we'll have to leave behind
our example from the Graph library.  Instead, consider the example
of the |def|_ function from Boost.Python_.  Its signature is
roughly as follows::

  template <
    class Function, Class KeywordExpression, class CallPolicies
  >
  void def(
      // Required parameters
      char const* name, Function func

      // Optional, deduced parameters
    , char const* docstring = ""
    , KeywordExpression keywords = no_keywords()
    , CallPolicies policies = default_call_policies()
  );

.. @ignore()

Try not to be too distracted by the use of the term “keywords” in
this example: although it means something analogous in Boost.Python
to what it means in the Parameter library, for the purposes of this
exercise you can think of it as being completely different.

When calling ``def``, only two arguments are required.  The
association between any additional arguments and their parameters
can be determined by the types of the arguments actually passed, so
the caller is neither required to remember argument positions or
explicitly specify parameter names for those arguments.  To
generate this interface using ``BOOST_PARAMETER_FUNCTION``, we need
only enclose the deduced parameters in a ``(deduced …)`` clause, as
follows: 

.. parsed-literal::

  namespace mpl = boost::mpl;

  BOOST_PARAMETER_FUNCTION(
      (void), def, tag,

      (required (name,(char const\*)) (func,\*) )   // nondeduced

      **(deduced** 
        (optional 
          (docstring, (char const\*), "")

          (keywords
             , \*(is_keyword_expression<mpl::_>) // see [#is_keyword_expression]_
             , no_keywords())

          (policies
             , \*(mpl::not_<
                   mpl::or_<
                       boost::is_convertible<mpl::_, char const\*>
                     , is_keyword_expression<mpl::_> // see [#is_keyword_expression]_
                   >
               >)
             , default_call_policies()
           )
         )
       **)**
   )
   {
      *…*
   }

.. @example.replace_emphasis('')

.. @example.prepend('''
   #include <boost/parameter.hpp>

   BOOST_PARAMETER_NAME(name)
   BOOST_PARAMETER_NAME(func)
   BOOST_PARAMETER_NAME(docstring)
   BOOST_PARAMETER_NAME(keywords)
   BOOST_PARAMETER_NAME(policies)

   struct default_call_policies
   {};

   struct no_keywords
   {};

   struct keywords
   {};

   template <class T>
   struct is_keyword_expression
     : boost::mpl::false_
   {};

   template <>
   struct is_keyword_expression<keywords>
     : boost::mpl::true_
   {};

   default_call_policies some_policies;

   void f()
   {}

   ''')

.. Admonition:: Syntax Note

  A ``(deduced …)`` clause always contains a ``(required …)``
  and/or an ``(optional …)`` subclause, and must follow any
  ``(required …)`` or ``(optional …)`` clauses indicating
  nondeduced parameters at the outer level.

With the declaration above, the following two calls are equivalent:

.. parsed-literal::

  def("f", &f, **some_policies**, **"Documentation for f"**);
  def("f", &f, **"Documentation for f"**, **some_policies**);

.. @example.prepend('''
   int main()
   {''')

If the user wants to pass a ``policies`` argument that was also,
for some reason, convertible to ``char const*``, she can always
specify the parameter name explicitly, as follows:

.. parsed-literal::

  def(
      "f", &f
     , **_policies = some_policies**, "Documentation for f");

.. @example.append('}')
.. @test('compile', howmany='all')

.. _Boost.Python: ../../../python/doc/index.html
.. |def| replace:: ``def``
.. _def: ../../../python/doc/v2/def.html

----------------------------------
Parameter-Enabled Member Functions
----------------------------------


The ``BOOST_PARAMETER_MEMBER_FUNCTION`` and
``BOOST_PARAMETER_CONST_MEMBER_FUNCTION`` macros accept exactly the
same arguments as ``BOOST_PARAMETER_FUNCTION``, but are designed to
be used within the body of a class::

  BOOST_PARAMETER_NAME(arg1)
  BOOST_PARAMETER_NAME(arg2)

  struct callable2
  {
      BOOST_PARAMETER_CONST_MEMBER_FUNCTION(
          (void), call, tag, (required (arg1,(int))(arg2,(int))))
      {
          std::cout << arg1 << ", " << arg2 << std::endl;
      }
  };

.. @example.prepend('''
   #include <boost/parameter.hpp>
   #include <iostream>
   using namespace boost::parameter;
   ''')

.. @test('compile')

These macros don't directly allow a function's interface to be
separated from its implementation, but you can always forward
arguments on to a separate implementation function::

  struct callable2
  {
      BOOST_PARAMETER_CONST_MEMBER_FUNCTION(
          (void), call, tag, (required (arg1,(int))(arg2,(int))))
      {
          call_impl(arg1,arg2);
      }
   private:
      void call_impl(int, int); // implemented elsewhere.
  };

.. @example.prepend('''
   #include <boost/parameter.hpp>

   BOOST_PARAMETER_NAME(arg1)
   BOOST_PARAMETER_NAME(arg2)
   using namespace boost::parameter;
   ''')

.. @test('compile')

Static Member Functions
=======================

To expose a static member function, simply insert the keyword
“``static``” before the function name:

.. parsed-literal::

  BOOST_PARAMETER_NAME(arg1)

  struct somebody
  {
      BOOST_PARAMETER_MEMBER_FUNCTION(
          (void), **static** f, tag, (optional (arg1,(int),0)))
      {
          std::cout << arg1 << std::endl;
      }
  };

.. @example.prepend('''
   #include <boost/parameter.hpp>
   #include <iostream>
   using namespace boost::parameter;
   ''')

.. @test('compile')


------------------------------
Parameter-Enabled Constructors
------------------------------

The lack of a “delegating constructor”
feature in C++
(http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2006/n1986.pdf)
limits somewhat the quality of interface this library can provide
for defining parameter-enabled constructors.  The usual workaround
for a lack of constructor delegation applies: one must factor the
common logic into a base class.  

Let's build a parameter-enabled constructor that simply prints its
arguments.  The first step is to write a base class whose
constructor accepts a single argument known as an |ArgumentPack|_:
a bundle of references to the actual arguments, tagged with their
keywords.  The values of the actual arguments are extracted from
the |ArgumentPack| by *indexing* it with keyword objects::

  BOOST_PARAMETER_NAME(name)
  BOOST_PARAMETER_NAME(index)

  struct myclass_impl
  {
      template <class ArgumentPack>
      myclass_impl(ArgumentPack const& args)
      {
          std::cout << "name = " << args[_name] 
                    << "; index = " << args[_index | 42] 
                    << std::endl;
      }
  };

.. @example.prepend('''
   #include <boost/parameter.hpp>
   #include <iostream>''')

Note that the bitwise or (“\ ``|``\ ”) operator has a special
meaning when applied to keyword objects that are passed to an
|ArgumentPack|\ 's indexing operator: it is used to indicate a
default value.  In this case if there is no ``index`` parameter in
the |ArgumentPack|, ``42`` will be used instead.

Now we are ready to write the parameter-enabled constructor
interface::

  struct myclass : myclass_impl
  {
      BOOST_PARAMETER_CONSTRUCTOR(
          myclass, (myclass_impl), tag
        , (required (name,*)) (optional (index,*))) // no semicolon
  };

Since we have supplied a default value for ``index`` but not for
``name``, only ``name`` is required.  We can exercise our new
interface as follows::

  myclass x("bob", 3);                     // positional
  myclass y(_index = 12, _name = "sally"); // named
  myclass z("june");                       // positional/defaulted

.. @example.wrap('int main() {', '}')
.. @test('run', howmany='all')

For more on |ArgumentPack| manipulation, see the `Advanced Topics`_
section.

---------------------------------
Parameter-Enabled Class Templates
---------------------------------

In this section we'll use Boost.Parameter to build Boost.Python_\
's `class_`_ template, whose “signature” is:

.. parsed-literal::

  template class<
      ValueType, BaseList = bases<>
    , HeldType = ValueType, Copyable = void
  >
  class class\_;

.. @ignore()

Only the first argument, ``ValueType``, is required.

.. _class_: http://www.boost.org/libs/python/doc/v2/class.html#class_-spec

Named Template Parameters
=========================

First, we'll build an interface that allows users to pass arguments
positionally or by name:

.. parsed-literal::

  struct B { virtual ~B() = 0; };
  struct D : B { ~D(); };

  class_<
       **class_type<B>**, **copyable<boost::noncopyable>** 
  > …;

  class_<
      **D**, **held_type<std::auto_ptr<D> >**, **base_list<bases<B> >**
  > …;

.. @ignore()

Template Keywords
-----------------

The first step is to define keywords for each template parameter::

  namespace boost { namespace python {

  BOOST_PARAMETER_TEMPLATE_KEYWORD(class_type)
  BOOST_PARAMETER_TEMPLATE_KEYWORD(base_list)
  BOOST_PARAMETER_TEMPLATE_KEYWORD(held_type)
  BOOST_PARAMETER_TEMPLATE_KEYWORD(copyable)

  }}

.. @example.prepend('#include <boost/parameter.hpp>')
.. @test('compile')

The declaration of the ``class_type`` keyword you see here is
equivalent to::

  namespace boost { namespace python {

  namespace tag { struct class_type; } // keyword tag type
  template <class T>
  struct class_type
    : parameter::template_keyword<tag::class_type,T>
  {};

  }}

.. @example.prepend('#include <boost/parameter.hpp>')
.. @test('compile')

It defines a keyword tag type named ``tag::class_type`` and a
*parameter passing template* named ``class_type``.

Class Template Skeleton
-----------------------

The next step is to define the skeleton of our class template,
which has three optional parameters.  Because the user may pass
arguments in any order, we don't know the actual identities of
these parameters, so it would be premature to use descriptive names
or write out the actual default values for any of them.  Instead,
we'll give them generic names and use the special type
``boost::parameter::void_`` as a default:

.. parsed-literal::

  namespace boost { namespace python {

  template <
      class A0
    , class A1 = parameter::void\_
    , class A2 = parameter::void\_
    , class A3 = parameter::void\_
  >
  struct class\_
  {
      *…*
  };

  }}

.. @example.prepend('#include <boost/parameter.hpp>')
.. @example.replace_emphasis('')
.. @test('compile')

Class Template Signatures
-------------------------

Next, we need to build a type, known as a |ParameterSpec|_,
describing the “signature” of ``boost::python::class_``.  A
|ParameterSpec|_ enumerates the required and optional parameters in
their positional order, along with any type requirements (note that
it does *not* specify defaults -- those will be dealt with
separately)::

  namespace boost { namespace python {

  using boost::mpl::_;

  typedef parameter::parameters<
      required<tag::class_type, boost::is_class<_> >
    , parameter::optional<tag::base_list, mpl::is_sequence<_> >
    , parameter::optional<tag::held_type>
    , parameter::optional<tag::copyable>
  > class_signature;

  }}

.. @example.prepend('''
   #include <boost/parameter.hpp>
   #include <boost/mpl/is_sequence.hpp>
   #include <boost/noncopyable.hpp>
   #include <boost/type_traits/is_class.hpp>
   #include <memory>

   using namespace boost::parameter;

   namespace boost { namespace python {

   BOOST_PARAMETER_TEMPLATE_KEYWORD(class_type)
   BOOST_PARAMETER_TEMPLATE_KEYWORD(base_list)
   BOOST_PARAMETER_TEMPLATE_KEYWORD(held_type)
   BOOST_PARAMETER_TEMPLATE_KEYWORD(copyable)

   template <class B = int>
   struct bases
   {};

   }}''')

.. |ParameterSpec| replace:: :concept:`ParameterSpec`

.. _ParameterSpec: reference.html#parameterspec

.. _binding_intro:

Argument Packs and Parameter Extraction
---------------------------------------

Next, within the body of ``class_`` , we use the |ParameterSpec|\ 's
nested ``::bind< … >`` template to bundle the actual arguments into an
|ArgumentPack|_ type, and then use the library's ``value_type< … >``
metafunction to extract “logical parameters”.  ``value_type< … >`` is
a lot like ``binding< … >``, but no reference is added to the actual
argument type.  Note that defaults are specified by passing it an
optional third argument::

  namespace boost { namespace python {

  template <
      class A0
    , class A1 = parameter::void_
    , class A2 = parameter::void_
    , class A3 = parameter::void_
  >
  struct class_
  {
      // Create ArgumentPack
      typedef typename 
        class_signature::bind<A0,A1,A2,A3>::type 
      args;

      // Extract first logical parameter.
      typedef typename parameter::value_type<
        args, tag::class_type>::type class_type;
      
      typedef typename parameter::value_type<
        args, tag::base_list, bases<> >::type base_list;
      
      typedef typename parameter::value_type<
        args, tag::held_type, class_type>::type held_type;
      
      typedef typename parameter::value_type<
        args, tag::copyable, void>::type copyable;
  };

  }}

.. |ArgumentPack| replace:: :concept:`ArgumentPack`
.. _ArgumentPack: reference.html#argumentpack

Exercising the Code So Far
==========================

.. compound::

  Revisiting our original examples, ::

    typedef boost::python::class_<
        class_type<B>, copyable<boost::noncopyable> 
    > c1;

    typedef boost::python::class_<
        D, held_type<std::auto_ptr<D> >, base_list<bases<B> > 
    > c2;

  .. @example.prepend('''
     using boost::python::class_type;
     using boost::python::copyable;
     using boost::python::held_type;
     using boost::python::base_list;
     using boost::python::bases;

     struct B {};
     struct D {};''')

  we can now examine the intended parameters::

    BOOST_MPL_ASSERT((boost::is_same<c1::class_type, B>));
    BOOST_MPL_ASSERT((boost::is_same<c1::base_list, bases<> >));
    BOOST_MPL_ASSERT((boost::is_same<c1::held_type, B>));
    BOOST_MPL_ASSERT((
         boost::is_same<c1::copyable, boost::noncopyable>
    ));

    BOOST_MPL_ASSERT((boost::is_same<c2::class_type, D>));
    BOOST_MPL_ASSERT((boost::is_same<c2::base_list, bases<B> >));
    BOOST_MPL_ASSERT((
        boost::is_same<c2::held_type, std::auto_ptr<D> >
    ));
    BOOST_MPL_ASSERT((boost::is_same<c2::copyable, void>));

.. @test('compile', howmany='all')

Deduced Template Parameters
===========================

To apply a deduced parameter interface here, we need only make the
type requirements a bit tighter so the ``held_type`` and
``copyable`` parameters can be crisply distinguished from the
others.  Boost.Python_ does this by requiring that ``base_list`` be
a specialization of its ``bases< … >`` template (as opposed to
being any old MPL sequence) and by requiring that ``copyable``, if
explicitly supplied, be ``boost::noncopyable``.  One easy way of
identifying specializations of ``bases< … >`` is to derive them all
from the same class, as an implementation detail:

.. parsed-literal::

  namespace boost { namespace python {

  namespace detail { struct bases_base {}; }

  template <class A0 = void, class A1 = void, class A2 = void *…* >
  struct bases **: detail::bases_base**
  {};

  }}  

.. @example.replace_emphasis('')
.. @example.prepend('''
   #include <boost/parameter.hpp>
   #include <boost/mpl/is_sequence.hpp>
   #include <boost/noncopyable.hpp>
   #include <memory>

   using namespace boost::parameter;
   using boost::mpl::_;

   namespace boost { namespace python {

   BOOST_PARAMETER_TEMPLATE_KEYWORD(class_type)
   BOOST_PARAMETER_TEMPLATE_KEYWORD(base_list)
   BOOST_PARAMETER_TEMPLATE_KEYWORD(held_type)
   BOOST_PARAMETER_TEMPLATE_KEYWORD(copyable)

   }}''')

Now we can rewrite our signature to make all three optional
parameters deducible::

  typedef parameter::parameters<
      required<tag::class_type, is_class<_> >

    , parameter::optional<
          deduced<tag::base_list>
        , is_base_and_derived<detail::bases_base,_>
      >

    , parameter::optional<
          deduced<tag::held_type>
        , mpl::not_<
              mpl::or_<
                  is_base_and_derived<detail::bases_base,_>
                , is_same<noncopyable,_>
              >
          >
      >

    , parameter::optional<deduced<tag::copyable>, is_same<noncopyable,_> >

  > class_signature;

.. @example.prepend('''
   #include <boost/type_traits/is_class.hpp>
   namespace boost { namespace python {''')

.. @example.append('''
   template <
       class A0
     , class A1 = parameter::void_
     , class A2 = parameter::void_
     , class A3 = parameter::void_
   >
   struct class_
   {
       // Create ArgumentPack
       typedef typename 
         class_signature::bind<A0,A1,A2,A3>::type 
       args;
 
       // Extract first logical parameter.
       typedef typename parameter::value_type<
         args, tag::class_type>::type class_type;
      
       typedef typename parameter::value_type<
         args, tag::base_list, bases<> >::type base_list;
      
       typedef typename parameter::value_type<
         args, tag::held_type, class_type>::type held_type;
      
       typedef typename parameter::value_type<
         args, tag::copyable, void>::type copyable;
   };

   }}''')

It may seem like we've added a great deal of complexity, but the
benefits to our users are greater.  Our original examples can now
be written without explicit parameter names:

.. parsed-literal::

  typedef boost::python::class_<**B**, **boost::noncopyable**> c1;

  typedef boost::python::class_<**D**, **std::auto_ptr<D>**, **bases<B>** > c2;

.. @example.prepend('''
   struct B {};
   struct D {};

   using boost::python::bases;''')

.. @example.append('''
   BOOST_MPL_ASSERT((boost::is_same<c1::class_type, B>));
   BOOST_MPL_ASSERT((boost::is_same<c1::base_list, bases<> >));
   BOOST_MPL_ASSERT((boost::is_same<c1::held_type, B>));
   BOOST_MPL_ASSERT((
        boost::is_same<c1::copyable, boost::noncopyable>
   ));

   BOOST_MPL_ASSERT((boost::is_same<c2::class_type, D>));
   BOOST_MPL_ASSERT((boost::is_same<c2::base_list, bases<B> >));
   BOOST_MPL_ASSERT((
       boost::is_same<c2::held_type, std::auto_ptr<D> >
   ));
   BOOST_MPL_ASSERT((boost::is_same<c2::copyable, void>));''')

.. @test('compile', howmany='all')

===============
Advanced Topics
===============

At this point, you should have a good grasp of the basics.  In this
section we'll cover some more esoteric uses of the library.

-------------------------
Fine-Grained Name Control
-------------------------

If you don't like the leading-underscore naming convention used
to refer to keyword objects, or you need the name ``tag`` for
something other than the keyword type namespace, there's another
way to use ``BOOST_PARAMETER_NAME``:

.. parsed-literal::

   BOOST_PARAMETER_NAME(\ **(**\ *object-name*\ **,** *tag-namespace*\ **)** *parameter-name*\ )

.. @ignore()

Here is a usage example:

.. parsed-literal::

  BOOST_PARAMETER_NAME((**pass_foo**, **keywords**) **foo**)

  BOOST_PARAMETER_FUNCTION(
    (int), f, 
    **keywords**, (required (**foo**, \*)))
  {
      return **foo** + 1;
  }

  int x = f(**pass_foo** = 41);

.. @example.prepend('#include <boost/parameter.hpp>')
.. @example.append('''
   int main()
   {}''')
.. @test('run')

Before you use this more verbose form, however, please read the
section on `best practices for keyword object naming`__.

__ `Keyword Naming`_

-----------------------
More |ArgumentPack|\ s
-----------------------

We've already seen |ArgumentPack|\ s when we looked at
`parameter-enabled constructors`_ and `class templates`__.  As you
might have guessed, |ArgumentPack|\ s actually lie at the heart of
everything this library does; in this section we'll examine ways to
build and manipulate them more effectively.

__ binding_intro_

Building |ArgumentPack|\ s
==========================

The simplest |ArgumentPack| is the result of assigning into a
keyword object::

   BOOST_PARAMETER_NAME(index)

   template <class ArgumentPack>
   int print_index(ArgumentPack const& args)
   {
       std::cout << "index = " << args[_index] << std::endl;
       return 0;
   }

   int x = print_index(_index = 3);  // prints "index = 3"

.. @example.prepend('''
   #include <boost/parameter.hpp>
   #include <iostream>''')

Also, |ArgumentPack|\ s can be composed using the comma operator.
The extra parentheses below are used to prevent the compiler from
seeing two separate arguments to ``print_name_and_index``::

   BOOST_PARAMETER_NAME(name)

   template <class ArgumentPack>
   int print_name_and_index(ArgumentPack const& args)
   {
       std::cout << "name = " << args[_name] << "; ";
       return print_index(args);
   }

   int y = print_name_and_index((_index = 3, _name = "jones"));

To build an |ArgumentPack| with positional arguments, we can use a
|ParameterSpec|_.  As introduced described in the section on `Class
Template Signatures`_, a |ParameterSpec| describes the positional
order of parameters and any associated type requirements.  Just as
we can build an |ArgumentPack| *type* with its nested ``::bind< …
>`` template, we can build an |ArgumentPack| *object* by invoking
its function call operator:

.. parsed-literal::

  parameter::parameters<
      required<tag::\ name, is_convertible<_,char const*> >
    , optional<tag::\ index, is_convertible<_,int> >
  > spec;

  char const sam[] = "sam";
  int twelve = 12;

  int z0 = print_name_and_index( **spec(**\ sam, twelve\ **)** );

  int z1 = print_name_and_index( 
     **spec(**\ _index=12, _name="sam"\ **)** 
  );

.. @example.prepend('''
   namespace parameter = boost::parameter;
   using parameter::required;
   using parameter::optional;
   using boost::is_convertible;
   using boost::mpl::_;''')

.. @example.append('''
   int main()
   {}''')

.. @test('run', howmany='all')

Note that because of the `forwarding problem`_, ``parameter::parameters::operator()``
can't accept non-const rvalues.

.. _`forwarding problem`: http://std.dkuug.dk/jtc1/sc22/wg21/docs/papers/2002/n1385.htm

Extracting Parameter Types
==========================

If we want to know the types of the arguments passed to
``print_name_and_index``, we have a couple of options.  The
simplest and least error-prone approach is to forward them to a
function template and allow *it* to do type deduction::

   BOOST_PARAMETER_NAME(name)
   BOOST_PARAMETER_NAME(index)

   template <class Name, class Index>
   int deduce_arg_types_impl(Name& name, Index& index)
   {
       Name& n2 = name;  // we know the types
       Index& i2 = index;
       return index;
   }

   template <class ArgumentPack>
   int deduce_arg_types(ArgumentPack const& args)
   {
       return deduce_arg_types_impl(args[_name], args[_index|42]);
   }

.. @example.prepend('''
   #include <boost/parameter.hpp>
   #include <cassert>''')

.. @example.append('''
   int a1 = deduce_arg_types((_name = "foo"));
   int a2 = deduce_arg_types((_name = "foo", _index = 3));

   int main()
   {
       assert(a1 == 42);
       assert(a2 == 3);
   }''')

.. @test('run')

Occasionally one needs to deduce argument types without an extra
layer of function call.  For example, suppose we wanted to return
twice the value of the ``index`` parameter?  In that
case we can use the ``value_type< … >`` metafunction introduced
`earlier`__::

   BOOST_PARAMETER_NAME(index)

   template <class ArgumentPack>
   typename parameter::value_type<ArgumentPack, tag::index, int>::type
   twice_index(ArgumentPack const& args)
   {
       return 2 * args[_index|42];
   }

   int six = twice_index(_index = 3);

.. @example.prepend('''
   #include <boost/parameter.hpp>
   #include <boost/type_traits/remove_reference.hpp>
   #include <cassert>

   namespace parameter = boost::parameter;
   ''')

.. @example.append('''
   int main()
   {
       assert(six == 6);
   }''')

.. @test('run', howmany='all')

Note that if we had used ``binding< … >`` rather than ``value_type< …
>``, we would end up returning a reference to the temporary created in
the ``2 * …`` expression.

__ binding_intro_

Lazy Default Computation
========================

When a default value is expensive to compute, it would be
preferable to avoid it until we're sure it's absolutely necessary.
``BOOST_PARAMETER_FUNCTION`` takes care of that problem for us, but
when using |ArgumentPack|\ s explicitly, we need a tool other than
``operator|``::

   BOOST_PARAMETER_NAME(s1)
   BOOST_PARAMETER_NAME(s2)
   BOOST_PARAMETER_NAME(s3)

   template <class ArgumentPack>
   std::string f(ArgumentPack const& args)
   {
       std::string const& s1 = args[_s1];
       std::string const& s2 = args[_s2];
       typename parameter::binding<
           ArgumentPack,tag::s3,std::string
       >::type s3 = args[_s3|(s1+s2)]; // always constructs s1+s2
       return s3;
   }

   std::string x = f((_s1="hello,", _s2=" world", _s3="hi world"));

.. @example.prepend('''
   #include <boost/parameter.hpp>
   #include <string>
   
   namespace parameter = boost::parameter;''')

.. @example.append('''
   int main()
   {}''')

.. @test('run')

In the example above, the string ``"hello, world"`` is constructed
despite the fact that the user passed us a value for ``s3``.  To
remedy that, we can compute the default value *lazily* (that is,
only on demand), by using ``boost::bind()`` to create a function
object.

.. danielw: I'm leaving the text below in the source, because we might
.. want to change back to it after 1.34, and if I remove it now we
.. might forget about it.

.. by combining the logical-or (“``||``”) operator
.. with a function object built by the Boost Lambda_ library: [#bind]_

.. parsed-literal::

   typename parameter::binding<
       ArgumentPack, tag::s3, std::string
   >::type s3 = args[_s3
       **|| boost::bind(std::plus<std::string>(), boost::ref(s1), boost::ref(s2))** ];

.. @example.prepend('''
   #include <boost/bind.hpp>
   #include <boost/ref.hpp>
   #include <boost/parameter.hpp>
   #include <string>
   #include <functional>

   namespace parameter = boost::parameter;

   BOOST_PARAMETER_NAME(s1)
   BOOST_PARAMETER_NAME(s2)
   BOOST_PARAMETER_NAME(s3)

   template <class ArgumentPack>
   std::string f(ArgumentPack const& args)
   {
       std::string const& s1 = args[_s1];
       std::string const& s2 = args[_s2];''')

.. @example.append('''
       return s3;
   }

   std::string x = f((_s1="hello,", _s2=" world", _s3="hi world"));

   int main()
   {}''')

.. @test('run')

.. .. _Lambda: ../../../lambda/index.html

.. sidebar:: Mnemonics

   To remember the difference between ``|`` and ``||``, recall that
   ``||`` normally uses short-circuit evaluation: its second
   argument is only evaluated if its first argument is ``false``.
   Similarly, in ``color_map[param||f]``, ``f`` is only invoked if
   no ``color_map`` argument was supplied.

The expression ``bind(std::plus<std::string>(), ref(s1), ref(s2))`` yields
a *function object* that, when invoked, adds the two strings together.
That function will only be invoked if no ``s3`` argument is supplied by 
the caller.

.. The expression ``lambda::var(s1)+lambda::var(s2)`` yields a
.. *function object* that, when invoked, adds the two strings
.. together.  That function will only be invoked if no ``s3`` argument
.. is supplied by the caller.

================ 
 Best Practices
================

By now you should have a fairly good idea of how to use the
Parameter library.  This section points out a few more-marginal
issues that will help you use the library more effectively.

--------------
Keyword Naming
--------------

``BOOST_PARAMETER_NAME`` prepends a leading underscore to the names
of all our keyword objects in order to avoid the following
usually-silent bug:

.. parsed-literal::

  namespace people
  {
    namespace tag { struct name; struct age;  }

    namespace // unnamed
    {
      boost::parameter::keyword<tag::name>& **name**
      = boost::parameter::keyword<tag::name>::instance;
      boost::parameter::keyword<tag::age>& **age**
      = boost::parameter::keyword<tag::age>::instance;
    }

    BOOST_PARAMETER_FUNCTION(
        (void), g, tag, (optional (name, \*, "bob")(age, \*, 42)))
    {
        std::cout << name << ":" << age;
    }

    void f(int age)
    {
    :vellipsis:`\ 
       .
       .
       .
     ` 
       g(**age** = 3); // whoops!
    }
  }

.. @ignore()

Although in the case above, the user was trying to pass the value
``3`` as the ``age`` parameter to ``g``, what happened instead
was that ``f``\ 's ``age`` argument got reassigned the value 3,
and was then passed as a positional argument to ``g``.  Since
``g``'s first positional parameter is ``name``, the default value
for ``age`` is used, and g prints ``3:42``.  Our leading
underscore naming convention that makes this problem less likely
to occur.

In this particular case, the problem could have been detected if
f's ``age`` parameter had been made ``const``, which is always a
good idea whenever possible.  Finally, we recommend that you use
an enclosing namespace for all your code, but particularly for
names with leading underscores.  If we were to leave out the
``people`` namespace above, names in the global namespace
beginning with leading underscores—which are reserved to your C++
compiler—might become irretrievably ambiguous with those in our
unnamed namespace.

----------
Namespaces
----------

In our examples we've always declared keyword objects in (an
unnamed namespace within) the same namespace as the
Boost.Parameter-enabled functions using those keywords:

.. parsed-literal::

  namespace lib
  {
    **BOOST_PARAMETER_NAME(name)
    BOOST_PARAMETER_NAME(index)**

    BOOST_PARAMETER_FUNCTION(
      (int), f, tag, 
      (optional (name,*,"bob")(index,(int),1))
    )
    {
        std::cout << name << ":" << index << std::endl;
        return index;
    }
  }

.. @example.prepend('''
   #include <boost/parameter.hpp>
   #include <iostream>''')
.. @namespace_setup = str(example)
.. @ignore()

Users of these functions have a few choices:

1. Full qualification:

  .. parsed-literal::

    int x = **lib::**\ f(**lib::**\ _name = "jill", **lib::**\ _index = 1);

  This approach is more verbose than many users would like.

.. @example.prepend(namespace_setup)
.. @example.append('int main() {}')
.. @test('run')

2. Make keyword objects available through
   *using-declarations*:

  .. parsed-literal::

    **using lib::_name;
    using lib::_index;**

    int x = lib::f(_name = "jill", _index = 1);

  This version is much better at the actual call site, but the
  *using-declarations* themselves can be verbose and hard-to
  manage.

.. @example.prepend(namespace_setup)
.. @example.append('int main() {}')
.. @test('run')

3. Bring in the entire namespace with a *using-directive*:

  .. parsed-literal::

    **using namespace lib;**
    int x = **f**\ (_name = "jill", _index = 3);

  This option is convenient, but it indiscriminately makes the
  *entire* contents of ``lib`` available without qualification.

.. @example.prepend(namespace_setup)
.. @example.append('int main() {}')
.. @test('run')

If we add an additional namespace around keyword declarations,
though, we can give users more control:

.. parsed-literal::

  namespace lib
  {
    **namespace keywords
    {**
       BOOST_PARAMETER_NAME(name)
       BOOST_PARAMETER_NAME(index)
    **}**

    BOOST_PARAMETER_FUNCTION(
      (int), f, **keywords::**\ tag, 
      (optional (name,*,"bob")(index,(int),1))
    )
    {
        std::cout << name << ":" << index << std::endl;
        return index;
    }
  }

.. @example.prepend('''
   #include <boost/parameter.hpp>
   #include <iostream>''')

Now users need only a single *using-directive* to bring in just the
names of all keywords associated with ``lib``:

.. parsed-literal::
  
  **using namespace lib::keywords;**
  int y = lib::f(_name = "bob", _index = 2);

.. @example.append('int main() {}')
.. @test('run', howmany='all')

-------------
Documentation
-------------

The interface idioms enabled by Boost.Parameter are completely new
(to C++), and as such are not served by pre-existing documentation
conventions.  

.. Note:: This space is empty because we haven't settled on any
   best practices yet.  We'd be very pleased to link to your
   documentation if you've got a style that you think is worth
   sharing.

============================
 Portability Considerations
============================

Use the `regression test results`_ for the latest Boost release of
the Parameter library to see how it fares on your favorite
compiler.  Additionally, you may need to be aware of the following
issues and workarounds for particular compilers.

.. _`regression test results`: http://www.boost.org/regression/release/user/parameter.html

-----------------
No SFINAE Support
-----------------

Some older compilers don't support SFINAE.  If your compiler meets
that criterion, then Boost headers will ``#define`` the preprocessor
symbol ``BOOST_NO_SFINAE``, and parameter-enabled functions won't be
removed from the overload set based on their signatures.

---------------------------
No Support for |result_of|_
---------------------------

.. |result_of| replace:: ``result_of``

.. _result_of: ../../../utility/utility.htm#result_of

`Lazy default computation`_ relies on the |result_of| class
template to compute the types of default arguments given the type
of the function object that constructs them.  On compilers that
don't support |result_of|, ``BOOST_NO_RESULT_OF`` will be
``#define``\ d, and the compiler will expect the function object to
contain a nested type name, ``result_type``, that indicates its
return type when invoked without arguments.  To use an ordinary
function as a default generator on those compilers, you'll need to
wrap it in a class that provides ``result_type`` as a ``typedef``
and invokes the function via its ``operator()``.

.. 
  Can't Declare |ParameterSpec| via ``typedef``
  =============================================

  In principle you can declare a |ParameterSpec| as a ``typedef``
  for a specialization of ``parameters<…>``, but Microsoft Visual C++
  6.x has been seen to choke on that usage.  The workaround is to use
  inheritance and declare your |ParameterSpec| as a class:

  .. parsed-literal::

       **struct dfs_parameters
         :** parameter::parameters<
             tag::graph, tag::visitor, tag::root_vertex
           , tag::index_map, tag::color_map
       > **{};**


  Default Arguments Unsupported on Nested Templates
  =================================================

  As of this writing, Borland compilers don't support the use of
  default template arguments on member class templates.  As a result,
  you have to supply ``BOOST_PARAMETER_MAX_ARITY`` arguments to every
  use of ``parameters<…>::match``.  Since the actual defaults used
  are unspecified, the workaround is to use
  |BOOST_PARAMETER_MATCH|_ to declare default arguments for SFINAE.

  .. |BOOST_PARAMETER_MATCH| replace:: ``BOOST_PARAMETER_MATCH``

--------------------------------------------------
Compiler Can't See References In Unnamed Namespace
--------------------------------------------------

If you use Microsoft Visual C++ 6.x, you may find that the compiler
has trouble finding your keyword objects.  This problem has been
observed, but only on this one compiler, and it disappeared as the
test code evolved, so we suggest you use it only as a last resort
rather than as a preventative measure.  The solution is to add
*using-declarations* to force the names to be available in the
enclosing namespace without qualification::

    namespace graphs
    {
      using graphs::graph;
      using graphs::visitor;
      using graphs::root_vertex;
      using graphs::index_map;
      using graphs::color_map;
    }

================
 Python Binding
================

.. _python: python.html

Follow `this link`__ for documentation on how to expose
Boost.Parameter-enabled functions to Python with `Boost.Python`_.

__ python.html

===========
 Reference
===========

.. _reference: reference.html

Follow `this link`__ to the Boost.Parameter reference
documentation.  

__ reference.html

==========
 Glossary
==========

.. _arguments:

:Argument (or “actual argument”): the value actually passed to a
  function or class template

.. _parameter:

:Parameter (or “formal parameter”): the name used to refer to an
  argument within a function or class template.  For example, the
  value of ``f``'s *parameter* ``x`` is given by the *argument*
  ``3``::

    int f(int x) { return x + 1 }
    int y = f(3);

==================
 Acknowledgements
==================

The authors would like to thank all the Boosters who participated
in the review of this library and its documentation, most
especially our review manager, Doug Gregor.

--------------------------

.. [#old_interface] As of Boost 1.33.0 the Graph library was still
   using an `older named parameter mechanism`__, but there are
   plans to change it to use Boost.Parameter (this library) in an
   upcoming release, while keeping the old interface available for
   backward-compatibility.  

__ ../../../graph/doc/bgl_named_params.html

.. [#odr] The **One Definition Rule** says that any given entity in
   a C++ program must have the same definition in all translation
   units (object files) that make up a program.

.. [#vertex_descriptor] If you're not familiar with the Boost Graph
   Library, don't worry about the meaning of any
   Graph-library-specific details you encounter.  In this case you
   could replace all mentions of vertex descriptor types with
   ``int`` in the text, and your understanding of the Parameter
   library wouldn't suffer.

.. [#ConceptCpp] This is a major motivation behind `ConceptC++`_.

.. _`ConceptC++`: http://www.generic-programming.org/software/ConceptGCC/

.. .. [#bind] The Lambda library is known not to work on `some
..   less-conformant compilers`__.  When using one of those you could
..   use `Boost.Bind`_ to generate the function object::

..      boost::bind(std::plus<std::string>(),s1,s2)

.. [#is_keyword_expression] Here we're assuming there's a predicate
   metafunction ``is_keyword_expression`` that can be used to
   identify models of Boost.Python's KeywordExpression concept.

.. .. __ http://www.boost.org/regression/release/user/lambda.html
.. _Boost.Bind: ../../../bind/index.html


.. [#using] You can always give the illusion that the function
   lives in an outer namespace by applying a *using-declaration*::

      namespace foo_overloads
      {
        // foo declarations here
        void foo() { ... }
        ...
      }
      using foo_overloads::foo;

    This technique for avoiding unintentional argument-dependent
    lookup is due to Herb Sutter.


.. [#sfinae] This capability depends on your compiler's support for SFINAE. 
   **SFINAE**: **S**\ ubstitution **F**\ ailure **I**\ s
   **N**\ ot **A**\ n **E** rror.  If type substitution during the
   instantiation of a function template results in an invalid type,
   no compilation error is emitted; instead the overload is removed
   from the overload set. By producing an invalid type in the
   function signature depending on the result of some condition,
   we can decide whether or not an overload is considered during overload
   resolution.  The technique is formalized in
   the |enable_if|_ utility.  Most recent compilers support SFINAE;
   on compilers that don't support it, the Boost config library
   will ``#define`` the symbol ``BOOST_NO_SFINAE``.
   See
   http://www.semantics.org/once_weakly/w02_SFINAE.pdf for more
   information on SFINAE.

.. |enable_if| replace:: ``enable_if``
.. _enable_if: ../../../utility/enable_if.html



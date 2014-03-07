+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 The Boost Parameter Library Python Binding Documentation 
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

:Authors:       David Abrahams, Daniel Wallin
:Contact:       dave@boost-consulting.com, daniel@boostpro.com
:organization:  `BoostPro Computing`_
:date:          $Date: 2010-05-27 10:58:58 -0700 (Thu, 27 May 2010) $

:copyright:     Copyright David Abrahams, Daniel Wallin
                2005-2009. Distributed under the Boost Software License,
                Version 1.0. (See accompanying file LICENSE_1_0.txt
                or copy at http://www.boost.org/LICENSE_1_0.txt)

:abstract:      Makes it possible to bind Boost.Parameter-enabled
                functions, operators and constructors to Python.

|(logo)|__

.. |(logo)| image:: ../../../../boost.png
   :alt: Boost

__ ../../../../index.htm

.. _`BoostPro Computing`: http://www.boostpro.com


.. role:: class
    :class: class

.. role:: concept
    :class: concept

.. role:: function
    :class: function

.. |ParameterSpec| replace:: :concept:`ParameterSpec`

.. contents::
    :depth: 1

Introduction
------------

``boost/parameter/python.hpp`` introduces a group of |def_visitors|_ that can
be used to easily expose Boost.Parameter-enabled member functions to Python with 
Boost.Python. It also provides a function template ``def()`` that can be used
to expose Boost.Parameter-enabled free functions.

.. |def_visitor| replace:: ``def_visitor``
.. |def_visitors| replace:: ``def_visitors``

.. _def_visitor: def_visitors_
.. _def_visitors: ../../../python/doc/v2/def_visitor.html

When binding a Boost.Parameter enabled function, the keyword tags
must be specified.  Additionally, because Boost.Parameter enabled
functions are templates, the desired function signature must be
specified.

..  The keyword tags are specified as an `MPL Sequence`_, using the
    pointer qualifications described in |ParameterSpec|_ below.  The
    signature is also specifid as an `MPL sequence`_ of parameter
    types. Additionally, ``boost::parameter::python::function`` and
    ``boost::parameter::python::def`` requires a class with forwarding
    overloads. We will take a closer look at how this is done in the
    tutorial section below.

The keyword tags and associated argument types are specified as an `MPL
Sequence`_, using the function type syntax described in |ParameterSpec|_
below. Additionally, ``boost::parameter::python::function`` and
``boost::parameter::python::def`` requires a class with forwarding overloads.
We will take a closer look at how this is done in the tutorial section below.

.. The last two sentences are terribly vague.  Which namespace is
.. ``function`` in?  Isn't the return type always needed?  What
.. else are we going to do other than pass these sequences to
.. function?

.. _`MPL Sequence`: ../../../mpl/doc/refmanual/sequences.html
.. _parameterspec: `concept ParameterSpec`_

Tutorial
--------

In this section we will outline the steps needed to bind a simple
Boost.Parameter-enabled member function to Python. Knowledge of the
Boost.Parameter macros_ are required to understand this section.

.. _macros: index.html

The class and member function we are interested in binding looks
like this:

.. parsed-literal::

  #include <boost/parameter/keyword.hpp>
  #include <boost/parameter/preprocessor.hpp>
  #include <boost/parameter/python.hpp>
  #include <boost/python.hpp>

  // First the keywords
  BOOST_PARAMETER_KEYWORD(tag, title)
  BOOST_PARAMETER_KEYWORD(tag, width)
  BOOST_PARAMETER_KEYWORD(tag, height)

  class window
  {
  public:
      BOOST_PARAMETER_MEMBER_FUNCTION(
        (void), open, tag,
        (required (title, (std::string)))
        (optional (width, (unsigned), 400)
                  (height, (unsigned), 400))
      )
      {
          *… function implementation …*
      }
  };

.. @example.prepend('#include <cassert>')
.. @example.replace_emphasis('''
   assert(title == "foo");
   assert(height == 20);
   assert(width == 400);
   ''')

It defines a set of overloaded member functions called ``open`` with one
required parameter and two optional ones. To bind this member function to
Python we use the binding utility ``boost::parameter::python::function``.
``boost::parameter::python::function`` is a |def_visitor|_ that we'll instantiate
and pass to ``boost::python::class_::def()``.

To use ``boost::parameter::python::function`` we first need to define
a class with forwarding overloads. This is needed because ``window::open()``
is a function template, so we can't refer to it in any other way. 

::

  struct open_fwd
  {
      template <class A0, class A1, class A2>
      void operator()(
          boost::type<void>, window& self
        , A0 const& a0, A1 const& a1, A2 const& a2
      )
      {
          self.open(a0, a1, a2);
      }
  };

The first parameter, ``boost::type<void>``, tells the forwarding overload
what the return type should be. In this case we know that it's always void
but in some cases, when we are exporting several specializations of a
Boost.Parameter-enabled template, we need to use that parameter to
deduce the return type.

``window::open()`` takes a total of 3 parameters, so the forwarding function
needs to take three parameters as well.

.. Note::

    We only need one overload in the forwarding class, despite the
    fact that there are two optional parameters. There are special
    circumstances when several overload are needed; see 
    `special keywords`_.

Next we'll define the module and export the class:

::

  BOOST_PYTHON_MODULE(my_module)
  {
      using namespace boost::python;
      namespace py = boost::parameter::python;
      namespace mpl = boost::mpl;

      class_<window>("window")
          .def(
              "open", py::function<
                  open_fwd
                , mpl::vector<
                      void
                    , tag::title(std::string)
                    , tag::width*(unsigned)
                    , tag::height*(unsigned)
                  >
              >()
          );
  }

.. @jam_prefix.append('import python ;')
.. @jam_prefix.append('stage . : my_module /boost/python//boost_python ;')
.. @my_module = build(
        output = 'my_module'
      , target_rule = 'python-extension'
      , input = '/boost/python//boost_python'
      , howmany = 'all'
    )

.. @del jam_prefix[:]

``py::function`` is passed two parameters. The first one is the class with
forwarding overloads that we defined earlier. The second one is an `MPL
Sequence`_ with the keyword tag types and argument types for the function
specified as function types. The pointer syntax used in ``tag::width*`` and
``tag::height*`` means that the parameter is optional. The first element of
the `MPL Sequence`_ is the return type of the function, in this case ``void``,
which is passed as the first argument to ``operator()`` in the forwarding
class.

..  The
    pointer syntax means that the parameter is optional, so in this case
    ``width`` and ``height`` are optional parameters. The third parameter
    is an `MPL Sequence`_ with the desired function signature. The return type comes first, and
    then the parameter types:

    .. parsed-literal::

        mpl::vector<void,        std::string, unsigned, unsigned>
                    *return type*  *title*        *width*     *height*

    .. @ignore()

That's it! This class can now be used in Python with the expected syntax::

    >>> w = my_module.window()
    >>> w.open(title = "foo", height = 20)

.. @example.prepend('import my_module')
.. @run_python(module_path = my_module)

.. Sorry to say this at such a late date, but this syntax really
.. strikes me as cumbersome.  Couldn't we do something like:

    class_<window>("window")
          .def(
              "open", 
              (void (*)( 
                  tag::title(std::string), 
                  tag::width*(unsigned), 
                  tag::height*(unsigned)) 
              )0
          );

   or at least:

      class_<window>("window")
          .def(
              "open", 
              mpl::vector<
                  void, 
                  tag::title(std::string), 
                  tag::width*(unsigned), 
                  tag::height*(unsigned)
              >()
          );

   assuming, that is, that we will have to repeat the tags (yes,
   users of broken compilers will have to give us function pointer
   types instead).

------------------------------------------------------------------------------

concept |ParameterSpec|
-----------------------

A |ParameterSpec| is a function type ``K(T)`` that describes both the keyword tag,
``K``, and the argument type, ``T``, for a parameter.

``K`` is either:

* A *required* keyword of the form ``Tag``
* **or**, an *optional* keyword of the form ``Tag*``
* **or**, a *special* keyword of the form ``Tag**``

where ``Tag`` is a keyword tag type, as used in a specialization
of |keyword|__.

.. |keyword| replace:: ``boost::parameter::keyword``
__ ../../../parameter/doc/html/reference.html#keyword

The **arity range** for an `MPL Sequence`_ of |ParameterSpec|'s is
defined as the closed range:

.. parsed-literal::

  [ mpl::size<S> - number of *special* keyword tags in ``S``, mpl::size<S> ]

For example, the **arity range** of ``mpl::vector2<x(int),y(int)>`` is ``[2,2]``,
the **arity range** of ``mpl::vector2<x(int),y*(int)>`` is ``[2,2]`` and the
**arity range** of ``mpl::vector2<x(int),y**(int)>`` is ``[1,2]``.



*special* keywords
---------------------------------

Sometimes it is desirable to have a default value for a parameter that differ
in type from the parameter. This technique is useful for doing simple tag-dispatching
based on the presence of a parameter. For example:

.. An example_ of this is given in the Boost.Parameter
   docs. The example uses a different technique, but could also have been written like this:

.. parsed-literal::

  namespace core
  {
    template <class ArgumentPack>
    void dfs_dispatch(ArgumentPack const& args, mpl::false\_)
    {
        *…compute and use default color map…*
    }

    template <class ArgumentPack, class ColorMap>
    void dfs_dispatch(ArgumentPack const& args, ColorMap colormap)
    {
        *…use colormap…*
    }
  }

  template <class ArgumentPack>
  void depth_first_search(ArgumentPack const& args)
  {
      core::dfs_dispatch(args, args[color | mpl::false_()]);
  }

.. @example.prepend('''
   #include <boost/parameter/keyword.hpp>
   #include <boost/parameter/parameters.hpp>
   #include <boost/mpl/bool.hpp>
   #include <cassert>

   BOOST_PARAMETER_KEYWORD(tag, color);

   typedef boost::parameter::parameters<tag::color> params;

   namespace mpl = boost::mpl;
   ''')

.. @example.replace_emphasis('''
   assert(args[color | 1] == 1);
   ''')

.. @example.replace_emphasis('''
   assert(args[color | 1] == 0);
   ''')

.. @example.append('''
   int main()
   {
       depth_first_search(params()());
       depth_first_search(params()(color = 0));
   }''')

.. @build()

.. .. _example: index.html#dispatching-based-on-the-presence-of-a-default

In the above example the type of the default for ``color`` is ``mpl::false_``, a
type that is distinct from any color map that the user might supply.

When binding the case outlined above, the default type for ``color`` will not
be convertible to the parameter type. Therefore we need to tag the ``color``
keyword as a *special* keyword. This is done by specifying the tag as
``tag::color**`` when binding the function (see `concept ParameterSpec`_ for
more details on the tagging). By doing this we tell the binding functions that
it needs to generate two overloads, one with the ``color`` parameter present
and one without. Had there been two *special* keywords, four overloads would
need to be generated. The number of generated overloads is equal to 2\
:sup:`N`, where ``N`` is the number of *special* keywords.

------------------------------------------------------------------------------

class template ``init``
-----------------------

Defines a named parameter enabled constructor.

.. parsed-literal::

    template <class ParameterSpecs>
    struct init : python::def_visitor<init<ParameterSpecs> >
    {
        template <class Class> 
        void def(Class& class\_);

        template <class CallPolicies>
        *def\_visitor* operator[](CallPolicies const& policies) const;
    };

.. @ignore()

``init`` requirements 
~~~~~~~~~~~~~~~~~~~~~

* ``ParameterSpecs`` is an `MPL sequence`_ where each element is a
  model of |ParameterSpec|. 
* For every ``N`` in ``[U,V]``, where ``[U,V]`` is the **arity
  range** of ``ParameterSpecs``, ``Class`` must support these
  expressions: 

  ======================= ============= =========================================
  Expression              Return type   Requirements
  ======================= ============= =========================================
  ``Class(a0, …, aN)``    \-            ``a0``\ …\ ``aN`` are tagged arguments.
  ======================= ============= =========================================



``template <class CallPolicies> operator[](CallPolicies const&)``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Returns a ``def_visitor`` equivalent to ``*this``, except that it
uses CallPolicies when creating the binding.


Example
~~~~~~~

.. parsed-literal::

    #include <boost/parameter/keyword.hpp>
    #include <boost/parameter/preprocessor.hpp>
    #include <boost/parameter/python.hpp>
    #include <boost/python.hpp>
    #include <boost/mpl/vector.hpp>

    BOOST_PARAMETER_KEYWORD(tag, x)
    BOOST_PARAMETER_KEYWORD(tag, y)

    struct base 
    { 
        template <class ArgumentPack>
        base(ArgumentPack const& args)
        {
            *… use args …*
        }
    };

    class X : base
    {
    public:
        BOOST_PARAMETER_CONSTRUCTOR(X, (base), tag,
            (required (x, \*))
            (optional (y, \*))
        )
    };

    BOOST_PYTHON_MODULE(*module name*)
    {
        using namespace boost::python;
        namespace py = boost::parameter::python;
        namespace mpl = boost::mpl;

        class_<X>("X", no_init)
            .def(
                py::init<
                    mpl::vector<tag::x(int), tag::y\*(int)>
                >()
            );
    }

.. @example.replace_emphasis('''
   assert(args[x] == 0);
   assert(args[y | 1] == 1);
   ''')

.. @example.replace_emphasis('my_module')

.. @jam_prefix.append('import python ;')
.. @jam_prefix.append('stage . : my_module /boost/python//boost_python ;')
.. @my_module = build(
        output = 'my_module'
      , target_rule = 'python-extension'
      , input = '/boost/python//boost_python'
    )

------------------------------------------------------------------------------

class template ``call``
-----------------------

Defines a ``__call__`` operator, mapped to ``operator()`` in C++.

.. parsed-literal::

    template <class ParameterSpecs>
    struct call : python::def_visitor<call<ParameterSpecs> >
    {
        template <class Class> 
        void def(Class& class\_);

        template <class CallPolicies>
        *def\_visitor* operator[](CallPolicies const& policies) const;
    };

.. @ignore()

``call`` requirements 
~~~~~~~~~~~~~~~~~~~~~

* ``ParameterSpecs`` is an `MPL sequence`_ where each element
  except the first models |ParameterSpec|. The first element
  is the result type of ``c(…)``.
* ``Class`` must support these expressions, where ``c`` is an 
  instance of ``Class``:

  =================== ==================== =======================================
  Expression          Return type          Requirements
  =================== ==================== =======================================
  ``c(a0, …, aN)``    Convertible to ``R`` ``a0``\ …\ ``aN`` are tagged arguments.
  =================== ==================== =======================================

  For every ``N`` in ``[U,V]``, where ``[U,V]`` is the **arity range** of ``ParameterSpecs``.


``template <class CallPolicies> operator[](CallPolicies const&)``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Returns a ``def_visitor`` equivalent to ``*this``, except that it
uses CallPolicies when creating the binding.


Example
~~~~~~~

.. parsed-literal::

    #include <boost/parameter/keyword.hpp>
    #include <boost/parameter/preprocessor.hpp>
    #include <boost/parameter/python.hpp>
    #include <boost/python.hpp>
    #include <boost/mpl/vector.hpp>

    BOOST_PARAMETER_KEYWORD(tag, x)
    BOOST_PARAMETER_KEYWORD(tag, y)

    namespace parameter = boost::parameter;

    typedef parameter::parameters<
        parameter::required<tag::x>
      , parameter::optional<tag::y>
    > call_parameters;

    class X
    {
    public:
        template <class ArgumentPack>
        int call_impl(ArgumentPack const& args)
        {
            *… use args …*
        }

        template <class A0>
        int operator()(A0 const& a0)
        {
            return call_impl(call_parameters()(a0));
        }

        template <class A0, class A1>
        int operator()(A0 const& a0, A1 const& a1)
        {
            return call_impl(call_parameters()(a0,a1));
        }
    };

    BOOST_PYTHON_MODULE(*module name*)
    {
        using namespace boost::python;
        namespace py = parameter::python;
        namespace mpl = boost::mpl;

        class_<X>("X")
            .def(
                py::call<
                    mpl::vector<int, tag::x(int), tag::y\*(int)>
                >()
            );
    }    

.. @example.replace_emphasis('''
   assert(args[x] == 0);
   assert(args[y | 1] == 1);
   return 0;
   ''')

.. @example.replace_emphasis('my_module')

.. @my_module = build(
        output = 'my_module'
      , target_rule = 'python-extension'
      , input = '/boost/python//boost_python'
    )

------------------------------------------------------------------------------

class template ``function``
---------------------------

Defines a named parameter enabled member function.

.. parsed-literal::

    template <class Fwd, class ParameterSpecs>
    struct function : python::def_visitor<function<Fwd, ParameterSpecs> >
    {
        template <class Class, class Options> 
        void def(Class& class\_, char const* name, Options const& options);
    };

.. @ignore()

``function`` requirements 
~~~~~~~~~~~~~~~~~~~~~~~~~

* ``ParameterSpecs`` is an `MPL sequence`_ where each element
  except the first models |ParameterSpec|. The first element
  is the result type of ``c.f(…)``, where ``f`` is the member
  function.
* An instance of ``Fwd`` must support this expression:

  ============================================ ==================== =================================================
  Expression                                   Return type          Requirements
  ============================================ ==================== =================================================
  ``fwd(boost::type<R>(), self, a0, …, aN)``   Convertible to ``R`` ``self`` is a reference to the object on which
                                                                    the function should be invoked. ``a0``\ …\ ``aN``
                                                                    are tagged arguments.
  ============================================ ==================== =================================================

  For every ``N`` in ``[U,V]``, where ``[U,V]`` is the **arity range** of ``ParameterSpecs``.


Example
~~~~~~~

This example exports a member function ``f(int x, int y = …)`` to Python. The
sequence of |ParameterSpec|'s ``mpl::vector2<tag::x(int), tag::y*(int)>`` has
an **arity range** of [2,2], so we only need one forwarding overload.

.. parsed-literal::

    #include <boost/parameter/keyword.hpp>
    #include <boost/parameter/preprocessor.hpp>
    #include <boost/parameter/python.hpp>
    #include <boost/python.hpp>
    #include <boost/mpl/vector.hpp>

    BOOST_PARAMETER_KEYWORD(tag, x)
    BOOST_PARAMETER_KEYWORD(tag, y)

    class X
    {
    public:
        BOOST_PARAMETER_MEMBER_FUNCTION((void), f, tag,
            (required (x, \*))
            (optional (y, \*, 1))
        )
        {
            *…*
        }
    };

    struct f_fwd
    {
        template <class A0, class A1>
        void operator()(boost::type<void>, X& self, A0 const& a0, A1 const& a1)
        {
            self.f(a0, a1);
        }
    };

    BOOST_PYTHON_MODULE(*module name*)
    {
        using namespace boost::python;
        namespace py = boost::parameter::python;
        namespace mpl = boost::mpl;

        class_<X>("X")
            .def("f",
                py::function<
                    f_fwd
                  , mpl::vector<void, tag::x(int), tag::y\*(int)>
                >()
            );
    }

.. @example.replace_emphasis('''
   assert(x == 0);
   assert(y == 1);
   ''')

.. @example.replace_emphasis('my_module')

.. @my_module = build(
        output = 'my_module'
      , target_rule = 'python-extension'
      , input = '/boost/python//boost_python'
    )

------------------------------------------------------------------------------

function template ``def``
-------------------------

Defines a named parameter enabled free function in the current Python scope.

.. parsed-literal::

    template <class Fwd, class ParameterSpecs>
    void def(char const* name);

.. @ignore()

``def`` requirements 
~~~~~~~~~~~~~~~~~~~~

* ``ParameterSpecs`` is an `MPL sequence`_ where each element
  except the first models |ParameterSpec|. The first element
  is the result type of ``f(…)``, where ``f`` is the function.
* An instance of ``Fwd`` must support this expression:

  ====================================== ==================== =======================================
  Expression                             Return type          Requirements
  ====================================== ==================== =======================================
  ``fwd(boost::type<R>(), a0, …, aN)``   Convertible to ``R`` ``a0``\ …\ ``aN`` are tagged arguments.
  ====================================== ==================== =======================================

  For every ``N`` in ``[U,V]``, where ``[U,V]`` is the **arity range** of ``ParameterSpecs``.


Example
~~~~~~~

This example exports a function ``f(int x, int y = …)`` to Python. The
sequence of |ParameterSpec|'s ``mpl::vector2<tag::x(int), tag::y*(int)>`` has
an **arity range** of [2,2], so we only need one forwarding overload.

.. parsed-literal::

    BOOST_PARAMETER_FUNCTION((void), f, tag,
        (required (x, \*))
        (optional (y, \*, 1))
    )
    {
        *…*
    }

    struct f_fwd
    {
        template <class A0, class A1>
        void operator()(boost::type<void>, A0 const& a0, A1 const& a1)
        {
            f(a0, a1);
        }
    };

    BOOST_PYTHON_MODULE(…)
    {
        def<
            f_fwd
          , mpl::vector<
                void, tag::\ x(int), tag::\ y\*(int)
            >
        >("f");
    }

.. @ignore()

.. again, the undefined ``fwd`` identifier.

Portability
-----------

The Boost.Parameter Python binding library requires *partial template
specialization*.


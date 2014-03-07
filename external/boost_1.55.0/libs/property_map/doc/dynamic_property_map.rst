================================
|(logo)|__ Dynamic Property Maps
================================

.. Copyright 2004-5 The Trustees of Indiana University.
 
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

.. |(logo)| image:: ../../../boost.png
   :align: middle
   :alt: Boost

__ ../../../index.htm

Summary
-------
The dynamic property map interfaces provides access to a collection of
property maps through a dynamically-typed interface. An algorithm can
use it to manipulate property maps without knowing their key or
value types at compile-time. Type-safe codes can use dynamic property
maps to interface more easily and completely with scripting languages
and other text-based representations of key-value data.

.. contents::

Introduction
------------
The Boost Property Map library specifies statically type-safe
interfaces through which key-value pairs can be manipulated by 
generic algorithms. Typically, an algorithm that uses property maps is
parameterized on the types of the property maps it uses, and it
manipulates them using the interfaces specified by the
Boost Property Map Library.
 
The following generic function illustrates property map basics.


::

  template <typename AgeMap, typename GPAMap>
  void 
  manipulate_freds_info(AgeMap ages, GPAMap gpas) {

    typedef typename boost::property_traits<AgeMap>::key_type name_type;
    typedef typename boost::property_traits<AgeMap>::value_type age_type;
    typedef typename boost::property_traits<GPAMap>::value_type gpa_type;

    name_type fred = "Fred";

    age_type old_age = get(ages, fred);
    gpa_type old_gpa = get(gpas, fred);

    std::cout << "Fred's old age: " << old_age << "\n"
	      << "Fred's old gpa: " << old_gpa << "\n";

    age_type new_age = 18;
    gpa_type new_gpa = 3.9;
    put(ages, fred, new_age);
    put(gpas, fred, new_gpa);
  }

The function is parameterized on two property map types, ``AgeMap`` and
``GPAMap``, and takes a value parameter for each of those types.  The
function uses the ``property_traits`` interface to ascertain, at
compile-time, the value and key types of the property maps.  The code
then retrieves Fred's old information, using the ``get`` function, and
updates it using the ``put`` function. The ``get`` function is required by the
Readable Property Map concept and both ``get`` and ``put`` are required by the
Read/Write Property Map concept.

The above function not only requires the two type parameters to model
property map concepts, but also makes some extra assumptions.
``AgeMap`` and ``GPAMap`` must have the same key type, and that type must be
constructable from a string.  Furthermore, ``AgeMap``'s value type must be
constructable from an ``int``.  Although these requirements are not
explicitly stated, they are statically checked during compilation and
failure to meet them yields compile-time errors.

Although the static typing of property map interfaces usually provides
desirable compile-time safety, some algorithms require a more dynamic
interface to property maps. For example, the Boost Graph Library (BGL)
provides functions that can initialize a graph by interpreting the
contents of a textual graph description (i.e. a GraphML file).  Such
general-purpose graph description languages can specify an arbitrary
number of edge and vertex properties, using strings to represent the
key-value pairs.  A graph reader function should capture these
arbitrary properties, but since function templates can only be
parameterized on a fixed number of property maps, the traditional
techniques for handling property maps do not suffice to implement them.

Dynamic property maps specifically address the need for an interface
to property maps whose checking is delayed to runtime.  Several
components combine to provide support for dynamic property maps. The
``dynamic_properties`` class collects a
group of heterogenous objects that model concepts from
the Boost Property Map library. Each property map is assigned a
string-based key when it is added to the collection, and it can be
addressed using that key.  Internally, ``dynamic_properties`` adapts
each contained property map with the dynamic property map interface,
which provides ``get`` and ``put`` functions that
can be called using values of any type that meets a few requirements.
Internally, the dynamic property map converts key and value pairs to
meet the requirements of the underlying property map or signals a
runtime exception if it cannot.


"Fred's Info" Revisited
~~~~~~~~~~~~~~~~~~~~~~~
Here's what the example above looks like using the
``dynamic_properties`` interface:

::

  void manipulate_freds_info(boost::dynamic_properties& properties)
  {
    using boost::get;
    std::string fred = "Fred";

    int old_age = get<int>("age", properties, fred);
    std::string old_gpa = get("gpa", properties, fred);

    std::cout << "Fred's old age: " << old_age << "\n"
	      << "Fred's old gpa: " << old_gpa << "\n";

    std::string new_age = "18";
    double new_gpa = 3.9;
    put("age",properties,fred,new_age);
    put("gpa",properties,fred,new_gpa);
  }

The new function is not a template parameterized on the property map
types but instead a concrete function that takes a ``dynamic_properties``
object.  Furthermore, the code no longer makes reference to key or
value types: keys and values are represented with strings.
Nonetheless the function still uses non-string types where they are
useful.  For instance, Fred's old age is represented using an ``int``.
It's value is retreived by calling ``get`` with a
type parameter, which determines its return type.  Finally, the
``get`` and ``put`` functions are each supplied a string-based key that
differs depending on the property of concern.  

Here's an example of how the above function might be called.

::

  int main()
  {
    using boost::get;

    // build property maps using associative_property_map
    std::map<std::string, int> name2age;
    std::map<std::string, double> name2gpa;
    boost::associative_property_map< std::map<std::string, int> >
      age_map(name2age);
    boost::associative_property_map< std::map<std::string, double> >
      gpa_map(name2gpa);

    std::string fred("Fred");
    // add key-value information
    name2age.insert(make_pair(fred,17));
    name2gpa.insert(make_pair(fred,2.7));

    // build and populate dynamic interface
    boost::dynamic_properties properties;
    properties.property("age",age_map);
    properties.property("gpa",gpa_map);

    manipulate_freds_info(properties);

    std::cout << "Fred's age: " << get(age_map,fred) << "\n"
	      << "Fred's gpa: " << get(gpa_map,fred) << "\n";	    
  }

The code first creates two property maps using ``std::map`` and the
``associative_property_map`` adaptor.  After initializing the
property maps with key-value data, it constructs a
``dynamic_properties`` object and adds to it both property maps,
keyed on the strings "age" and "gpa".  Finally ``manipulate_freds_info``
is passed the ``dynamic_properties`` object and the results of its changes are
displayed.  

As shown above, the ``dynamic_properties`` object provides, where needed, a
dynamically-typed interface to property maps yet preserves the static
typing of property map uses elsewhere in an application.

Reference
---------
::

  class dynamic_properties

The ``dynamic_properties`` class provides a dynamically-typed interface to
a set of property maps. To use it, one must populate
an object of this class with property maps using the ``property`` member
function.

Member Functions
~~~~~~~~~~~~~~~~

::

  dynamic_properties()
  dynamic_properties(
    const boost::function<
      boost::shared_ptr<dynamic_property_map> (
	const std::string&, const boost::any&, const boost::any&)
      >& fn)

A ``dynamic_properties`` object can be constructed with a function object
that, when called, creates a new property map.  The library provides the 
``ignore_other_properties`` function object, which lets the ``dynamic_properties`` object ignore any properties that it hasn't been prepared to record.
If an attempt is made
to ``put`` a key-value pair to a nonexistent ``dynamic_properties`` key,
then this function is called with the ``dynamic_properties`` key and the
intended property key and value .  If ``dynamic_properties`` is
default-constructed, such a ``put`` attempt throws 
``property_not_found``. 


::

  template<typename PropertyMap>
  dynamic_properties& 
  property(const std::string& name, PropertyMap property_map)

This member function adds a property map to the set of maps contained,
using ``name`` as its key.

Requirements: ``PropertyMap`` must model Readable Property Map or
Read/Write Property Map.

::

  void insert(const std::string& name, boost::shared_ptr<dynamic_property_map> pm)

This member function directly adds a ``dynamic_property_map``
to the collection, using ``name`` as its key.

::

  iterator begin()
  const_iterator begin() const

This member function returns an iterator over the set of property maps
held by the ``dynamic_properties`` object.

::

  iterator end()
  const_iterator end() const

This member function returns a terminal iterator over the set of
dynamic property maps held by the ``dynamic_properties`` object.  It is used to
terminate traversals over the set of dynamic property maps

::

  iterator lower_bound(const std::string& name) 

This member function returns an iterator that points to the first
property map whose ``dynamic_properties`` key is ``name``.  
Bear in mind that multiple property maps may have the same
``dynamic_properties`` key, so long as their property map key types differ.

Invariant: The range [ lower_bound(name), end() ) contains every
property map that has name for its ``dynamic_properties`` key.

Free functions
~~~~~~~~~~~~~~

::

  boost::shared_ptr<boost::dynamic_property_map> 
  ignore_other_properties(const std::string&,
                          const boost::any&,
                          const boost::any&)

When passed to the ``dynamic_properties`` constructor, this function
allows the ``dynamic_properties`` object to disregard attempts to put
values to unknown keys without signaling an error.

::

  template<typename Key, typename Value>
  bool put(const std::string& name, dynamic_properties& dp, const Key& key, 
           const Value& value)

This function adds a key-value pair to the property map with the
matching name and key type. If no matching property map is found,
behavior depends on the availability of a property map generator.  If
a property map generator was supplied when the ``dynamic_properties``
object was constructed, then that function is used to create a new
property map.  If the generator fails to generate a property map
(returns a null ``shared_ptr``), then the ``put`` function returns
``false``.  If, on the other hand, the ``dynamic_properties`` object
has no property map generator (meaning it was default-constructed),
then ``property_not_found`` is thrown. If a candidate property map is
found but it does not support ``put``, ``dynamic_const_put_error`` is
thrown.

::

  template<typename Value, typename Key>
  Value get(const std::string& name, const dynamic_properties& dp, 
            const Key& key)

This function gets the value from the property-map whose namee is
given and whose key type matches. If ``Value`` is ``std::string``, then the
property map's value type must either be ``std::string`` or model
OutputStreamable.  In the latter case, the ``get`` function converts the
value to a string.  If no matching property map is found,
``dynamic_get_failure`` is thrown.


=============================================================================

::

  class dynamic_property_map

This class describes the interface used by ``dynamic_properties`` to
interact with a user's property maps polymorphically. 

::

  boost::any get(const any& key)

Given a representation of a key, return the value associated with that key. 

Requirement:
1) The object passed as the key must be convertible to a value of the
map's key type. Details of that conversion are unspecified.
2) For this expression to be valid, the key must be
associated with some value, otherwise the result is undefined.

::

  std::string get_string(const any& key) 

Given a representation of a key, return the string representation
of the value associated with that key.

Requirements:
1) The object passed as the key must be convertible to the
property map's key type. Details of that conversion are unspecified.
2) For this expression to be valid, the key must be
associated with some value, otherwise the result is undefined.
3) The value type of the property map must model Output Streamable.

::

  void put(const any& key, const any& value) 

Given a representation of a key and a representation of a value, the
key and value are associated in the property map.

Requirements:
1) The object passed as the key must be convertible to the
property map's key type. Details of that conversion are unspecified.
2) The object passed as the value must be convertible to the
property map's value type. Details of that conversion are unspecified.
3) The property map need not support this member function, in which
case an error will be signaled.  This is the runtime analogue of the
Readable Property Map concept.

::

  const std::type_info& key() const 

Returns a ``type_info`` object that represents the property map's key type.

::

  const std::type_info& value() const 

Returns a ``type_info`` object that represents the property map's value type.


Exceptions
~~~~~~~~~~

::

  struct dynamic_property_exception : public std::exception {
    virtual ~dynamic_property_exception() throw() {}
  };

  struct property_not_found : public std::exception {
    std::string property;
    property_not_found(const std::string& property);
    virtual ~property_not_found() throw();

    const char* what() const throw();
  };

  struct dynamic_get_failure : public std::exception {
    std::string property;
    dynamic_get_failure(const std::string& property);
    virtual ~dynamic_get_failure() throw();

    const char* what() const throw();
  };

  struct dynamic_const_put_error  : public std::exception {
    virtual ~dynamic_const_put_error() throw();

    const char* what() const throw();
  };


Under certain circumstances, calls to ``dynamic_properties`` member
functions will throw one of the above exceptions.  The three concrete
exceptions can all be caught using the general
``dynamic_property_exception`` moniker when greater precision is not
needed.  In addition, all of the above exceptions derive from the
standard ``std::exception`` for even more generalized error handling.
The specific circumstances that result in these exceptions are
described above.

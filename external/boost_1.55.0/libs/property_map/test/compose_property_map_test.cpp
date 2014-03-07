// Copyright (C) 2013 Eurodecision
// Authors: Guillaume Pinot
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/property_map/compose_property_map.hpp>

#include <boost/property_map/function_property_map.hpp>
#include <boost/test/minimal.hpp>

void concept_checks()
{
    using namespace boost;
    {
        typedef null_archetype<> Key;
        //typedef assignable_archetype<copy_constructible_archetype<> > Value;
        typedef copy_constructible_archetype<assignable_archetype<> > Value;
        typedef readable_property_map_archetype<Key, Key> GPMap;
        typedef readable_property_map_archetype<Key, Value> FPMap;
        typedef compose_property_map<FPMap, GPMap> CPM;
        function_requires<ReadablePropertyMapConcept<CPM, Key> >();
    }
    {
        typedef null_archetype<> Key;
        typedef copy_constructible_archetype<assignable_archetype<> > Value;
        typedef readable_property_map_archetype<Key, Key> GPMap;
        typedef writable_property_map_archetype<Key, Value> FPMap;
        typedef compose_property_map<FPMap, GPMap> CPM;
        function_requires<WritablePropertyMapConcept<CPM, Key> >();
    }
    {
        typedef null_archetype<> Key;
        typedef copy_constructible_archetype<assignable_archetype<> > Value;
        typedef readable_property_map_archetype<Key, Key> GPMap;
        typedef read_write_property_map_archetype<Key, Value> FPMap;
        typedef compose_property_map<FPMap, GPMap> CPM;
        function_requires<ReadWritePropertyMapConcept<CPM, Key> >();
    }
    {
        typedef null_archetype<> Key;
        typedef copy_constructible_archetype<assignable_archetype<> > Value;
        typedef readable_property_map_archetype<Key, Key> GPMap;
        typedef lvalue_property_map_archetype<Key, Value> FPMap;
        typedef compose_property_map<FPMap, GPMap> CPM;
        function_requires<LvaluePropertyMapConcept<CPM, Key> >();
    }
    {
        typedef null_archetype<> Key;
        typedef copy_constructible_archetype<assignable_archetype<> > Value;
        typedef readable_property_map_archetype<Key, Key> GPMap;
        typedef mutable_lvalue_property_map_archetype<Key, Value> FPMap;
        typedef compose_property_map<FPMap, GPMap> CPM;
        function_requires<Mutable_LvaluePropertyMapConcept<CPM, Key> >();
    }
}

void pointer_pmap_check()
{
    const int idx[] = {2, 0, 4, 1, 3};
    double v[] = {1., 3., 0., 4., 2.};
    boost::compose_property_map<double*, const int*> cpm(v, idx);

    for (int i = 0; i < 5; ++i) {
        BOOST_CHECK(get(cpm, i) == static_cast<double>(i));
        ++cpm[i];
        BOOST_CHECK(cpm[i] == static_cast<double>(i + 1));
        put(cpm, i, 42.);
        BOOST_CHECK(cpm[i] == 42.);
    }
}

struct modulo_add_one {
    typedef int result_type;
    modulo_add_one(int m): modulo(m) {}
    int operator()(int i) const {return (i + 1) % modulo;}
    int modulo;
};

void readable_pmap_checks()
{
    using namespace boost;
    typedef function_property_map<modulo_add_one, int> modulo_add_one_pmap;

    compose_property_map<modulo_add_one_pmap, modulo_add_one_pmap>
            cpm(modulo_add_one(5), modulo_add_one(5));

    for (int i = 0; i < 10; ++i)
        BOOST_CHECK(get(cpm, i) == (i + 2) % 5);
}

int
test_main(int, char**)
{
    concept_checks();
    pointer_pmap_check();
    readable_pmap_checks();

    return 0;
}

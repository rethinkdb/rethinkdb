// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------

// Intentionally no include guards (to be included more than once)

#if !defined(CHTYPE) || !defined(T) || !defined(PTREE) || !defined(NOCASE) || !defined(WIDECHAR)
#   error No character type specified
#endif

void test_debug(PTREE *)
{
#if 0
    // Check count
    BOOST_CHECK(PTREE::debug_get_instances_count() == 0);

    {

        // Create ptrees
        PTREE pt1, pt2;
        BOOST_CHECK(PTREE::debug_get_instances_count() == 2);

        // Create PTREE
        PTREE *pt3 = new PTREE;
        BOOST_CHECK(PTREE::debug_get_instances_count() == 3);

        // Insert
        pt1.push_back(std::make_pair(T("key"), *pt3));
        BOOST_CHECK(PTREE::debug_get_instances_count() == 4);

        // Insert
        pt2.push_back(std::make_pair(T("key"), *pt3));
        BOOST_CHECK(PTREE::debug_get_instances_count() == 5);

        // Clear
        pt1.clear();
        BOOST_CHECK(PTREE::debug_get_instances_count() == 4);

        // Clear
        pt2.clear();
        BOOST_CHECK(PTREE::debug_get_instances_count() == 3);

        // Delete
        delete pt3;
        BOOST_CHECK(PTREE::debug_get_instances_count() == 2);

    }

    // Check count
    BOOST_CHECK(PTREE::debug_get_instances_count() == 0);
#endif
}

void test_constructor_destructor_assignment(PTREE *)
{

    {

        // Test constructor from string
        PTREE pt1(T("data"));
        BOOST_CHECK(pt1.data() == T("data"));
        //BOOST_CHECK(PTREE::debug_get_instances_count() == 1);

        // Do insertions
        PTREE &tmp1 = pt1.put(T("key1"), T("data1"));
        PTREE &tmp2 = pt1.put(T("key2"), T("data2"));
        tmp1.put(T("key3"), T("data3"));
        tmp2.put(T("key4"), T("data4"));
        //BOOST_CHECK(PTREE::debug_get_instances_count() == 5);

        // Make a copy using copy constructor
        PTREE *pt2 = new PTREE(pt1);
        BOOST_CHECK(*pt2 == pt1);
        //BOOST_CHECK(PTREE::debug_get_instances_count() == 10);

        // Make a copy using = operator
        PTREE *pt3 = new PTREE;
        *pt3 = *pt2;
        BOOST_CHECK(*pt3 == *pt2);
        //BOOST_CHECK(PTREE::debug_get_instances_count() == 15);

        // Test self assignment
        pt1 = pt1;
        BOOST_CHECK(pt1 == *pt2);
        BOOST_CHECK(pt1 == *pt3);
        //BOOST_CHECK(PTREE::debug_get_instances_count() == 15);

        // Destroy
        delete pt2;
        //BOOST_CHECK(PTREE::debug_get_instances_count() == 10);

        // Destroy
        delete pt3;
        //BOOST_CHECK(PTREE::debug_get_instances_count() == 5);

    }

    // Check count
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 0);

}

void test_insertion(PTREE *)
{

    // Do insertions
    PTREE pt;
    PTREE tmp1(T("data1"));
    PTREE tmp2(T("data2"));
    PTREE tmp3(T("data3"));
    PTREE tmp4(T("data4"));
    PTREE::iterator it1 = pt.insert(pt.end(), std::make_pair(T("key1"), tmp1));
    PTREE::iterator it2 = pt.insert(it1, std::make_pair(T("key2"), tmp2));
    PTREE::iterator it3 = it1->second.push_back(std::make_pair(T("key3"), tmp3));
    PTREE::iterator it4 = it1->second.push_front(std::make_pair(T("key4"), tmp4));
    it2->second.insert(it2->second.end(), it1->second.begin(), it1->second.end());

    // Check instance count
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 11);

    // Check contents
    BOOST_CHECK(pt.get(T("key1"), T("")) == T("data1"));
    BOOST_CHECK(pt.get(T("key2"), T("")) == T("data2"));
    BOOST_CHECK(pt.get(T("key1.key3"), T("")) == T("data3"));
    BOOST_CHECK(pt.get(T("key1.key4"), T("")) == T("data4"));
    BOOST_CHECK(pt.get(T("key2.key3"), T("")) == T("data3"));
    BOOST_CHECK(pt.get(T("key2.key4"), T("")) == T("data4"));

    // Check sequence
    PTREE::iterator it = it2;
    ++it; BOOST_CHECK(it == it1);
    ++it; BOOST_CHECK(it == pt.end());
    it = it4; 
    ++it; BOOST_CHECK(it == it3);
    ++it; BOOST_CHECK(it == it1->second.end());

}

void test_erasing(PTREE *)
{
    
    // Do insertions
    PTREE pt;
    PTREE tmp1(T("data1"));
    PTREE tmp2(T("data2"));
    PTREE tmp3(T("data3"));
    PTREE tmp4(T("data4"));
    PTREE::iterator it1 = pt.insert(pt.end(), std::make_pair(T("key1"), tmp1));
    PTREE::iterator it2 = pt.insert(it1, std::make_pair(T("key2"), tmp2));
    it1->second.push_back(std::make_pair(T("key"), tmp3));
    it1->second.push_front(std::make_pair(T("key"), tmp4));
    it2->second.insert(it2->second.end(), it1->second.begin(), it1->second.end());

    // Check instance count
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 11);

    // Test range erase
    PTREE::iterator it = it1->second.erase(it1->second.begin(), it1->second.end());
    BOOST_CHECK(it == it1->second.end());
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 9);

    // Test single erase
    PTREE::size_type n = pt.erase(T("key1"));
    BOOST_CHECK(n == 1);
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 8);

    // Test multiple erase
    n = it2->second.erase(T("key"));
    BOOST_CHECK(n == 2);
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 6);

    // Test one more erase
    n = pt.erase(T("key2"));
    BOOST_CHECK(n == 1);
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 5);

}

void test_clear(PTREE *)
{

    // Do insertions
    PTREE pt(T("data"));
    pt.push_back(std::make_pair(T("key"), PTREE(T("data"))));

    // Check instance count
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 2);

    // Test clear
    pt.clear();
    BOOST_CHECK(pt.empty());
    BOOST_CHECK(pt.data().empty());
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 1);

}

void test_pushpop(PTREE *)
{

    // Do insertions
    PTREE pt;
    PTREE tmp1(T("data1"));
    PTREE tmp2(T("data2"));
    PTREE tmp3(T("data3"));
    PTREE tmp4(T("data4"));
    pt.push_back(std::make_pair(T("key3"), tmp3));
    pt.push_front(std::make_pair(T("key2"), tmp2));
    pt.push_back(std::make_pair(T("key4"), tmp4));
    pt.push_front(std::make_pair(T("key1"), tmp1));

    // Check instance count
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 9);

    // Check sequence
    PTREE::iterator it = pt.begin();
    BOOST_CHECK(it->first == T("key1")); ++it;
    BOOST_CHECK(it->first == T("key2")); ++it;
    BOOST_CHECK(it->first == T("key3")); ++it;
    BOOST_CHECK(it->first == T("key4")); ++it;
    BOOST_CHECK(it == pt.end());

    // Test pops
    pt.pop_back();
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 8);
    BOOST_CHECK(pt.front().second.data() == T("data1"));
    BOOST_CHECK(pt.back().second.data() == T("data3"));
    pt.pop_front();
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 7);
    BOOST_CHECK(pt.front().second.data() == T("data2"));
    BOOST_CHECK(pt.back().second.data() == T("data3"));
    pt.pop_back();
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 6);
    BOOST_CHECK(pt.front().second.data() == T("data2"));
    BOOST_CHECK(pt.back().second.data() == T("data2"));
    pt.pop_front();
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 5);
    BOOST_CHECK(pt.empty());

}

void test_container_iteration(PTREE *)
{
    
    // Do insertions
    PTREE pt;
    pt.put(T("key3"), T(""));
    pt.put(T("key1"), T(""));
    pt.put(T("key4"), T(""));
    pt.put(T("key2"), T(""));

    // iterator
    {
        PTREE::iterator it = pt.begin();
        BOOST_CHECK(it->first == T("key3")); ++it;
        BOOST_CHECK(it->first == T("key1")); ++it;
        BOOST_CHECK(it->first == T("key4")); ++it;
        BOOST_CHECK(it->first == T("key2")); ++it;
        BOOST_CHECK(it == pt.end());
    }

    // const_iterator
    {
        PTREE::const_iterator it = pt.begin();
        BOOST_CHECK(it->first == T("key3")); ++it;
        BOOST_CHECK(it->first == T("key1")); ++it;
        BOOST_CHECK(it->first == T("key4")); ++it;
        BOOST_CHECK(it->first == T("key2")); ++it;
        BOOST_CHECK(it == pt.end());
    }

    // reverse_iterator
    {
        PTREE::reverse_iterator it = pt.rbegin();
        BOOST_CHECK(it->first == T("key2")); ++it;
        BOOST_CHECK(it->first == T("key4")); ++it;
        BOOST_CHECK(it->first == T("key1")); ++it;
        BOOST_CHECK(it->first == T("key3")); ++it;
        BOOST_CHECK(it == pt.rend());
    }

    // const_reverse_iterator
    {
        PTREE::const_reverse_iterator it = pt.rbegin();
        BOOST_CHECK(it->first == T("key2")); ++it;
        BOOST_CHECK(it->first == T("key4")); ++it;
        BOOST_CHECK(it->first == T("key1")); ++it;
        BOOST_CHECK(it->first == T("key3")); ++it;
        BOOST_CHECK(it == PTREE::const_reverse_iterator(pt.rend()));
    }

}

void test_swap(PTREE *)
{
    
    PTREE pt1, pt2;

    // Do insertions
    pt1.put(T("key1"), T(""));
    pt1.put(T("key2"), T(""));
    pt1.put(T("key1.key3"), T(""));
    pt1.put(T("key1.key4"), T(""));

    // Check counts
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 6);
    BOOST_CHECK(pt1.size() == 2);
    BOOST_CHECK(pt1.get_child(T("key1")).size() == 2);
    BOOST_CHECK(pt2.size() == 0);

    // Swap using member function
    pt1.swap(pt2);
    
    // Check counts
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 6);
    BOOST_CHECK(pt2.size() == 2);
    BOOST_CHECK(pt2.get_child(T("key1")).size() == 2);
    BOOST_CHECK(pt1.size() == 0);

    // Swap using free function
    swap(pt1, pt2);

    // Check counts
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 6);
    BOOST_CHECK(pt1.size() == 2);
    BOOST_CHECK(pt1.get_child(T("key1")).size() == 2);
    BOOST_CHECK(pt2.size() == 0);

    // Swap using std algorithm
    std::swap(pt1, pt2);

    // Check counts
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 6);
    BOOST_CHECK(pt2.size() == 2);
    BOOST_CHECK(pt2.get_child(T("key1")).size() == 2);
    BOOST_CHECK(pt1.size() == 0);

}

void test_sort_reverse(PTREE *)
{
    
    PTREE pt;

    // Do insertions
    pt.put(T("key2"), T("data2"));
    pt.put(T("key1"), T("data1"));
    pt.put(T("key4"), T("data4"));
    pt.put(T("key3"), T("data3"));
    pt.put(T("key3.key1"), T(""));
    pt.put(T("key4.key2"), T(""));

    // Reverse
    pt.reverse();

    // Check sequence
    {
        PTREE::iterator it = pt.begin();
        BOOST_CHECK(it->first == T("key3")); ++it;
        BOOST_CHECK(it->first == T("key4")); ++it;
        BOOST_CHECK(it->first == T("key1")); ++it;
        BOOST_CHECK(it->first == T("key2")); ++it;
        BOOST_CHECK(it == pt.end());
    }
    // Check sequence using find to check if index is ok
    {
        PTREE::iterator it = pt.begin();
        BOOST_CHECK(it == pt.to_iterator(pt.find(T("key3")))); ++it;
        BOOST_CHECK(it == pt.to_iterator(pt.find(T("key4")))); ++it;
        BOOST_CHECK(it == pt.to_iterator(pt.find(T("key1")))); ++it;
        BOOST_CHECK(it == pt.to_iterator(pt.find(T("key2")))); ++it;
        BOOST_CHECK(it == pt.end());
    }

    // Sort
    pt.sort(SortPred<PTREE>());

    // Check sequence
    {
        PTREE::iterator it = pt.begin();
        BOOST_CHECK(it->first == T("key1")); ++it;
        BOOST_CHECK(it->first == T("key2")); ++it;
        BOOST_CHECK(it->first == T("key3")); ++it;
        BOOST_CHECK(it->first == T("key4")); ++it;
        BOOST_CHECK(it == pt.end());
    }
    // Check sequence (using find to check if index is ok)
    {
        PTREE::iterator it = pt.begin();
        BOOST_CHECK(it == pt.to_iterator(pt.find(T("key1")))); ++it;
        BOOST_CHECK(it == pt.to_iterator(pt.find(T("key2")))); ++it;
        BOOST_CHECK(it == pt.to_iterator(pt.find(T("key3")))); ++it;
        BOOST_CHECK(it == pt.to_iterator(pt.find(T("key4")))); ++it;
        BOOST_CHECK(it == pt.end());
    }
    
    // Sort reverse
    pt.sort(SortPredRev<PTREE>());

    // Check sequence
    {
        PTREE::iterator it = pt.begin();
        BOOST_CHECK(it->first == T("key4")); ++it;
        BOOST_CHECK(it->first == T("key3")); ++it;
        BOOST_CHECK(it->first == T("key2")); ++it;
        BOOST_CHECK(it->first == T("key1")); ++it;
        BOOST_CHECK(it == pt.end());
    }
    // Check sequence (using find to check if index is ok)
    {
        PTREE::iterator it = pt.begin();
        BOOST_CHECK(it == pt.to_iterator(pt.find(T("key4")))); ++it;
        BOOST_CHECK(it == pt.to_iterator(pt.find(T("key3")))); ++it;
        BOOST_CHECK(it == pt.to_iterator(pt.find(T("key2")))); ++it;
        BOOST_CHECK(it == pt.to_iterator(pt.find(T("key1")))); ++it;
        BOOST_CHECK(it == pt.end());
    }

}

void test_case(PTREE *)
{
    
    // Do insertions
    PTREE pt;
    pt.put(T("key1"), T("data1"));
    pt.put(T("KEY2"), T("data2"));
    pt.put(T("kEy1.keY3"), T("data3"));
    pt.put(T("KEY1.key4"), T("data4"));

    // Check findings depending on traits type
#if (NOCASE == 0)
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 7);
    BOOST_CHECK(pt.get(T("key1"), T("")) == T("data1"));
    BOOST_CHECK(pt.get(T("key2"), T("")) == T(""));
    BOOST_CHECK(pt.get(T("key1.key3"), T("")) == T(""));
    BOOST_CHECK(pt.get(T("KEY1.key4"), T("")) == T("data4"));
#else
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 5);
    BOOST_CHECK(pt.get(T("key1"), T("1")) == pt.get(T("KEY1"), T("2")));
    BOOST_CHECK(pt.get(T("key2"), T("1")) == pt.get(T("KEY2"), T("2")));
    BOOST_CHECK(pt.get(T("key1.key3"), T("1")) == pt.get(T("KEY1.KEY3"), T("2")));
    BOOST_CHECK(pt.get(T("key1.key4"), T("1")) == pt.get(T("KEY1.KEY4"), T("2")));
#endif

    // Do more insertions
    pt.push_back(PTREE::value_type(T("key1"), PTREE()));
    pt.push_back(PTREE::value_type(T("key1"), PTREE()));

    // Test counts
#if (NOCASE == 0)
    BOOST_CHECK(pt.count(T("key1")) == 3);
    BOOST_CHECK(pt.count(T("KEY1")) == 1);
    BOOST_CHECK(pt.count(T("key2")) == 0);
    BOOST_CHECK(pt.count(T("KEY2")) == 1);
    BOOST_CHECK(pt.count(T("key3")) == 0);
    BOOST_CHECK(pt.count(T("KEY3")) == 0);
#else
    BOOST_CHECK(pt.count(T("key1")) == 3);
    BOOST_CHECK(pt.count(T("KEY1")) == 3);
    BOOST_CHECK(pt.count(T("key2")) == 1);
    BOOST_CHECK(pt.count(T("KEY2")) == 1);
    BOOST_CHECK(pt.count(T("key3")) == 0);
    BOOST_CHECK(pt.count(T("KEY3")) == 0);
#endif

}

void test_comparison(PTREE *)
{

    // Prepare original
    PTREE pt_orig(T("data"));
    pt_orig.put(T("key1"), T("data1"));
    pt_orig.put(T("key1.key3"), T("data2"));
    pt_orig.put(T("key1.key4"), T("data3"));
    pt_orig.put(T("key2"), T("data4"));

    // Test originals
    {
        PTREE pt1(pt_orig);
        PTREE pt2(pt_orig);
        BOOST_CHECK(pt1 == pt2);
        BOOST_CHECK(pt2 == pt1);
        BOOST_CHECK(!(pt1 != pt2));
        BOOST_CHECK(!(pt2 != pt1));
    }

    // Test originals with modified case
#if (NOCASE != 0)
    {
        PTREE pt1(pt_orig);
        PTREE pt2(pt_orig);
        pt1.pop_back();
        pt1.put(T("KEY2"), T("data4"));
        BOOST_CHECK(pt1 == pt2);
        BOOST_CHECK(pt2 == pt1);
        BOOST_CHECK(!(pt1 != pt2));
        BOOST_CHECK(!(pt2 != pt1));
    }
#endif

    // Test modified copies (both modified the same way)
    {
        PTREE pt1(pt_orig);
        PTREE pt2(pt_orig);
        pt1.put(T("key1.key5"), T("."));
        pt2.put(T("key1.key5"), T("."));
        BOOST_CHECK(pt1 == pt2);
        BOOST_CHECK(pt2 == pt1);
        BOOST_CHECK(!(pt1 != pt2));
        BOOST_CHECK(!(pt2 != pt1));
    }

    // Test modified copies (modified root data)
    {
        PTREE pt1(pt_orig);
        PTREE pt2(pt_orig);
        pt1.data() = T("a");
        pt2.data() = T("b");
        BOOST_CHECK(!(pt1 == pt2));
        BOOST_CHECK(!(pt2 == pt1));
        BOOST_CHECK(pt1 != pt2);
        BOOST_CHECK(pt2 != pt1);
    }

    // Test modified copies (added subkeys with different data)
    {
        PTREE pt1(pt_orig);
        PTREE pt2(pt_orig);
        pt1.put(T("key1.key5"), T("a"));
        pt2.put(T("key1.key5"), T("b"));
        BOOST_CHECK(!(pt1 == pt2));
        BOOST_CHECK(!(pt2 == pt1));
        BOOST_CHECK(pt1 != pt2);
        BOOST_CHECK(pt2 != pt1);
    }

    // Test modified copies (added subkeys with different keys)
    {
        PTREE pt1(pt_orig);
        PTREE pt2(pt_orig);
        pt1.put(T("key1.key5"), T(""));
        pt2.put(T("key1.key6"), T(""));
        BOOST_CHECK(!(pt1 == pt2));
        BOOST_CHECK(!(pt2 == pt1));
        BOOST_CHECK(pt1 != pt2);
        BOOST_CHECK(pt2 != pt1);
    }

    // Test modified copies (added subkey to only one copy)
    {
        PTREE pt1(pt_orig);
        PTREE pt2(pt_orig);
        pt1.put(T("key1.key5"), T(""));
        BOOST_CHECK(!(pt1 == pt2));
        BOOST_CHECK(!(pt2 == pt1));
        BOOST_CHECK(pt1 != pt2);
        BOOST_CHECK(pt2 != pt1);
    }

}

void test_front_back(PTREE *)
{
    
    // Do insertions
    PTREE pt;
    pt.put(T("key1"), T(""));
    pt.put(T("key2"), T(""));

    // Check front and back
    BOOST_CHECK(pt.front().first == T("key1"));
    BOOST_CHECK(pt.back().first == T("key2"));

}

void test_get_put(PTREE *)
{
    
    typedef std::basic_string<CHTYPE> str_t;

    // Temporary storage
    str_t tmp_string;
    boost::optional<int> opt_int;
    boost::optional<long> opt_long;
    boost::optional<double> opt_double;
    boost::optional<float> opt_float;
    boost::optional<str_t> opt_string;
    boost::optional<CHTYPE> opt_char;
    boost::optional<bool> opt_bool;
    
    // Do insertions via put
    PTREE pt;
    PTREE &pt1 = pt.put(T("k1"), 1);
    PTREE &pt2 = pt.put(T("k2.k"), 2.5);
    PTREE &pt3 = pt.put(T("k3.k.k"), T("ala ma kota"));
    PTREE &pt4 = pt.put(T("k4.k.k.k"), CHTYPE('c'));
    PTREE &pt5 = pt.put(T("k5.k.k.k.f"), false);
    PTREE &pt6 = pt.put(T("k5.k.k.k.t"), true);

    // Check instances count
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 17);

    // Check if const char * version returns std::string
    BOOST_CHECK(typeid(pt.get_value(T(""))) == typeid(str_t));
    
    // Do extractions via get (throwing version)
    BOOST_CHECK(pt.get<int>(T("k1")) == 1);
    BOOST_CHECK(pt.get<long>(T("k1")) == 1);
    BOOST_CHECK(pt.get<double>(T("k2.k")) == 2.5);
    BOOST_CHECK(pt.get<float>(T("k2.k")) == 2.5f);
    BOOST_CHECK(pt.get<str_t>(T("k3.k.k")) == str_t(T("ala ma kota")));
    BOOST_CHECK(pt.get<CHTYPE>(T("k4.k.k.k")) == CHTYPE('c'));
    BOOST_CHECK(pt.get<bool>(T("k5.k.k.k.f")) == false);
    BOOST_CHECK(pt.get<bool>(T("k5.k.k.k.t")) == true);

    // Do extractions via get (default value version)
    BOOST_CHECK(pt.get(T("k1"), 0) == 1);
    BOOST_CHECK(pt.get(T("k1"), 0L) == 1);
    BOOST_CHECK(pt.get(T("k2.k"), 0.0) == 2.5);
    BOOST_CHECK(pt.get(T("k2.k"), 0.0f) == 2.5f);
    BOOST_CHECK(pt.get(T("k3.k.k"), str_t()) == str_t(T("ala ma kota")));
    BOOST_CHECK(pt.get(T("k3.k.k"), T("")) == T("ala ma kota"));
    BOOST_CHECK(pt.get(T("k4.k.k.k"), CHTYPE('\0')) == CHTYPE('c'));
    BOOST_CHECK(pt.get(T("k5.k.k.k.f"), true) == false);
    BOOST_CHECK(pt.get(T("k5.k.k.k.t"), false) == true);

    // Do extractions via get (optional version)
    opt_int = pt.get_optional<int>(T("k1"));
    BOOST_CHECK(opt_int && *opt_int == 1);
    opt_long = pt.get_optional<long>(T("k1"));
    BOOST_CHECK(opt_long && *opt_long == 1);
    opt_double = pt.get_optional<double>(T("k2.k"));
    BOOST_CHECK(opt_double && *opt_double == 2.5);
    opt_float = pt.get_optional<float>(T("k2.k"));
    BOOST_CHECK(opt_float && *opt_float == 2.5f);
    opt_string = pt.get_optional<str_t>(T("k3.k.k"));
    BOOST_CHECK(opt_string && *opt_string == str_t(T("ala ma kota")));
    opt_char = pt.get_optional<CHTYPE>(T("k4.k.k.k"));
    BOOST_CHECK(opt_char && *opt_char == CHTYPE('c'));
    opt_bool = pt.get_optional<bool>(T("k5.k.k.k.f"));
    BOOST_CHECK(opt_bool && *opt_bool == false);
    opt_bool = pt.get_optional<bool>(T("k5.k.k.k.t"));
    BOOST_CHECK(opt_bool && *opt_bool == true);

    // Do insertions via put_value
    pt1.put_value(short(1));
    pt2.put_value(2.5f);
    pt3.put_value(str_t(T("ala ma kota")));
    pt4.put_value(CHTYPE('c'));
    pt5.put_value(false);
    pt6.put_value(true);

    // Do extractions via get_value (throwing version)
    BOOST_CHECK(pt1.get_value<int>() == 1);
    BOOST_CHECK(pt1.get_value<long>() == 1);
    BOOST_CHECK(pt2.get_value<double>() == 2.5);
    BOOST_CHECK(pt2.get_value<float>() == 2.5f);
    BOOST_CHECK(pt3.get_value<str_t>() == str_t(T("ala ma kota")));
    BOOST_CHECK(pt4.get_value<CHTYPE>() == CHTYPE('c'));
    BOOST_CHECK(pt5.get_value<bool>() == false);
    BOOST_CHECK(pt6.get_value<bool>() == true);

    // Do extractions via get_value (default value version)
    BOOST_CHECK(pt1.get_value(0) == 1);
    BOOST_CHECK(pt1.get_value(0L) == 1);
    BOOST_CHECK(pt2.get_value(0.0) == 2.5);
    BOOST_CHECK(pt2.get_value(0.0f) == 2.5f);
    BOOST_CHECK(pt3.get_value(str_t()) == str_t(T("ala ma kota")));
    BOOST_CHECK(pt3.get_value(T("")) == T("ala ma kota"));
    BOOST_CHECK(pt4.get_value(CHTYPE('\0')) == CHTYPE('c'));
    BOOST_CHECK(pt5.get_value(true) == false);
    BOOST_CHECK(pt6.get_value(false) == true);

    // Do extractions via get_value (optional version)
    opt_int = pt1.get_value_optional<int>();
    BOOST_CHECK(opt_int && *opt_int == 1);
    opt_long = pt1.get_value_optional<long>();
    BOOST_CHECK(opt_long && *opt_long == 1);
    opt_double = pt2.get_value_optional<double>();
    BOOST_CHECK(opt_double && *opt_double == 2.5);
    opt_float = pt2.get_value_optional<float>();
    BOOST_CHECK(opt_float && *opt_float == 2.5f);
    opt_string = pt3.get_value_optional<str_t>();
    BOOST_CHECK(opt_string && *opt_string == str_t(T("ala ma kota")));
    opt_char = pt4.get_value_optional<CHTYPE>();
    BOOST_CHECK(opt_char && *opt_char == CHTYPE('c'));
    opt_bool = pt5.get_value_optional<bool>();
    BOOST_CHECK(opt_bool && *opt_bool == false);
    opt_bool = pt6.get_value_optional<bool>();
    BOOST_CHECK(opt_bool && *opt_bool == true);

    // Do incorrect extractions (throwing version)
    try
    {
        pt.get<int>(T("k2.k.bogus.path"));
        BOOST_ERROR("No required exception thrown");
    }
    catch (boost::property_tree::ptree_bad_path &) { }
    catch (...)
    {
        BOOST_ERROR("Wrong exception type thrown");
    }
    try
    {
        pt.get<int>(T("k2.k"));
        BOOST_ERROR("No required exception thrown");
    }
    catch (boost::property_tree::ptree_bad_data &) { }
    catch (...)
    {
        BOOST_ERROR("Wrong exception type thrown");
    }

    // Do incorrect extractions (default value version)
    BOOST_CHECK(pt.get(T("k2.k"), -7) == -7);
    BOOST_CHECK(pt.get(T("k3.k.k"), -7) == -7);
    BOOST_CHECK(pt.get(T("k4.k.k.k"), -7) == -7);

    // Do incorrect extractions (optional version)
    BOOST_CHECK(!pt.get_optional<int>(T("k2.k")));
    BOOST_CHECK(!pt.get_optional<int>(T("k3.k.k")));
    BOOST_CHECK(!pt.get_optional<int>(T("k4.k.k.k")));

    // Test multiple puts with the same key
    {
        PTREE pt;
        pt.put(T("key"), 1);
        BOOST_CHECK(pt.get<int>(T("key")) == 1);
        BOOST_CHECK(pt.size() == 1);
        pt.put(T("key"), 2);
        BOOST_CHECK(pt.get<int>(T("key")) == 2);
        BOOST_CHECK(pt.size() == 1);
        pt.put(T("key.key.key"), 1);
        PTREE &child = pt.get_child(T("key.key"));
        BOOST_CHECK(pt.get<int>(T("key.key.key")) == 1);
        BOOST_CHECK(child.size() == 1);
        BOOST_CHECK(child.count(T("key")) == 1);
        pt.put(T("key.key.key"), 2);
        BOOST_CHECK(pt.get<int>(T("key.key.key")) == 2);
        BOOST_CHECK(child.size() == 1);
        BOOST_CHECK(child.count(T("key")) == 1);
    }

    // Test multiple puts with the same key
    {
        PTREE pt;
        pt.put(T("key"), 1);
        BOOST_CHECK(pt.get<int>(T("key")) == 1);
        BOOST_CHECK(pt.size() == 1);
        pt.put(T("key"), 2);
        BOOST_CHECK(pt.get<int>(T("key")) == 2);
        BOOST_CHECK(pt.size() == 1);
        pt.put(T("key.key.key"), 1);
        PTREE &child = pt.get_child(T("key.key"));
        BOOST_CHECK(child.size() == 1);
        BOOST_CHECK(child.count(T("key")) == 1);
        pt.add(T("key.key.key"), 2);
        BOOST_CHECK(child.size() == 2);
        BOOST_CHECK(child.count(T("key")) == 2);
    }

    // Test that put does not destroy children
    {
        PTREE pt;
        pt.put(T("key1"), 1);
        pt.put(T("key1.key2"), 2);
        BOOST_CHECK(pt.get<int>(T("key1"), 0) == 1);
        BOOST_CHECK(pt.get<int>(T("key1.key2"), 0) == 2);
        pt.put(T("key1"), 2);
        BOOST_CHECK(pt.get<int>(T("key1"), 0) == 2);
        BOOST_CHECK(pt.get<int>(T("key1.key2"), 0) == 2);
    }

    // Test that get of single character that is whitespace works
    {
        PTREE pt;
        pt.put_value(T(' '));
        CHTYPE ch = pt.get_value<CHTYPE>();
        BOOST_CHECK(ch == T(' '));
    }

    // Test that get of non-char value with trailing and leading whitespace works
    {
        PTREE pt;
        pt.put_value(T(" \t\n99 \t\n"));
        BOOST_CHECK(pt.get_value<int>(0) == 99);
    }

}
                                    
void test_get_child_put_child(PTREE *)
{

    typedef std::basic_string<CHTYPE> str_t;

    PTREE pt(T("ala ma kota"));

    // Do insertions via put_child
    PTREE pt1, pt2, pt3;
    pt1.put_child(T("k1"), PTREE());
    pt1.put_child(T("k2.k"), PTREE());
    pt2.put_child(T("k1"), pt);
    pt2.put_child(T("k2.k"), pt);

    // Const references to test const versions of methods
    const PTREE &cpt1 = pt1, &cpt2 = pt2;

    // Do correct extractions via get_child (throwing version)
    BOOST_CHECK(pt1.get_child(T("k1")).empty());
    BOOST_CHECK(pt1.get_child(T("k2.k")).empty());
    BOOST_CHECK(pt2.get_child(T("k1")) == pt);
    BOOST_CHECK(pt2.get_child(T("k2.k")) == pt);
    BOOST_CHECK(cpt1.get_child(T("k1")).empty());
    BOOST_CHECK(cpt1.get_child(T("k2.k")).empty());
    BOOST_CHECK(cpt2.get_child(T("k1")) == pt);
    BOOST_CHECK(cpt2.get_child(T("k2.k")) == pt);

    // Do correct extractions via get_child (default value version)
    BOOST_CHECK(pt1.get_child(T("k1"), PTREE(T("def"))) != PTREE(T("def")));
    BOOST_CHECK(pt1.get_child(T("k2.k"), PTREE(T("def"))) != PTREE(T("def")));
    BOOST_CHECK(pt2.get_child(T("k1"), PTREE(T("def"))) == pt);
    BOOST_CHECK(pt2.get_child(T("k2.k"), PTREE(T("def"))) == pt);
    BOOST_CHECK(cpt1.get_child(T("k1"), PTREE(T("def"))) != PTREE(T("def")));
    BOOST_CHECK(cpt1.get_child(T("k2.k"), PTREE(T("def"))) != PTREE(T("def")));
    BOOST_CHECK(cpt2.get_child(T("k1"), PTREE(T("def"))) == pt);
    BOOST_CHECK(cpt2.get_child(T("k2.k"), PTREE(T("def"))) == pt);

    // Do correct extractions via get_child (optional version)
    boost::optional<PTREE &> opt;
    boost::optional<const PTREE &> copt;
    opt = pt1.get_child_optional(T("k1"));
    BOOST_CHECK(opt);
    opt = pt1.get_child_optional(T("k2.k"));
    BOOST_CHECK(opt);
    opt = pt2.get_child_optional(T("k1"));
    BOOST_CHECK(opt && *opt == pt);
    opt = pt2.get_child_optional(T("k2.k"));
    BOOST_CHECK(opt && *opt == pt);
    copt = cpt1.get_child_optional(T("k1"));
    BOOST_CHECK(copt);
    copt = cpt1.get_child_optional(T("k2.k"));
    BOOST_CHECK(copt);
    copt = cpt2.get_child_optional(T("k1"));
    BOOST_CHECK(copt && *copt == pt);
    copt = cpt2.get_child_optional(T("k2.k"));
    BOOST_CHECK(copt && *copt == pt);

    // Do incorrect extractions via get_child (throwing version)
    try
    {
        pt.get_child(T("k2.k.bogus.path"));
        BOOST_ERROR("No required exception thrown");
    }
    catch (boost::property_tree::ptree_bad_path &) { }
    catch (...)
    {
        BOOST_ERROR("Wrong exception type thrown");
    }

    // Do incorrect extractions via get_child (default value version)
    BOOST_CHECK(&pt.get_child(T("k2.k.bogus.path"), pt3) == &pt3);

    // Do incorrect extractions via get_child (optional version)
    BOOST_CHECK(!pt.get_child_optional(T("k2.k.bogus.path")));

    // Test multiple puts with the same key
    {
        PTREE pt, tmp1(T("data1")), tmp2(T("data2"));
        pt.put_child(T("key"), tmp1);
        BOOST_CHECK(pt.get_child(T("key")) == tmp1);
        BOOST_CHECK(pt.size() == 1);
        pt.put_child(T("key"), tmp2);
        BOOST_CHECK(pt.get_child(T("key")) == tmp2);
        BOOST_CHECK(pt.size() == 1);
        pt.put_child(T("key.key.key"), tmp1);
        PTREE &child = pt.get_child(T("key.key"));
        BOOST_CHECK(child.size() == 1);
        pt.put_child(T("key.key.key"), tmp2);
        BOOST_CHECK(child.size() == 1);
    }

    // Test multiple adds with the same key
    {
        PTREE pt, tmp1(T("data1")), tmp2(T("data2"));
        pt.add_child(T("key"), tmp1);
        BOOST_CHECK(pt.size() == 1);
        pt.add_child(T("key"), tmp2);
        BOOST_CHECK(pt.size() == 2);
        BOOST_CHECK(pt.count(T("key")) == 2);
        pt.add_child(T("key.key.key"), tmp1);
        PTREE &child = pt.get_child(T("key.key"));
        BOOST_CHECK(child.size() == 1);
        BOOST_CHECK(child.count(T("key")) == 1);
        pt.add_child(T("key.key.key"), tmp2);
        BOOST_CHECK(child.size() == 2);
        BOOST_CHECK(child.count(T("key")) == 2);
    }

    // Test assigning child to tree
    {
        PTREE pt;
        pt.put(T("foo.bar"), T("baz"));
        pt = pt.get_child(T("foo"));
        BOOST_CHECK(pt.size() == 1);
        BOOST_CHECK(pt.get< std::basic_string<CHTYPE> >(T("bar")) == T("baz"));
    }

}

void test_equal_range(PTREE *)
{
    PTREE pt;
    pt.add_child(T("k1"), PTREE());
    pt.add_child(T("k2"), PTREE());
    pt.add_child(T("k1"), PTREE());
    pt.add_child(T("k3"), PTREE());
    pt.add_child(T("k1"), PTREE());
    pt.add_child(T("k2"), PTREE());

    BOOST_CHECK(boost::distance(pt.equal_range(T("k1"))) == 3);
    BOOST_CHECK(boost::distance(pt.equal_range(T("k2"))) == 2);
    BOOST_CHECK(boost::distance(pt.equal_range(T("k3"))) == 1);
}

void test_path_separator(PTREE *)
{

    typedef PTREE::path_type path;

    // Check instances count
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 0);

    // Do insertions
    PTREE pt;
    pt.put(T("key1"), T("1"));
    pt.put(T("key2.key"), T("2"));
    pt.put(T("key3.key.key"), T("3"));
    pt.put(path(T("key4"), CHTYPE('/')), T("4"));
    pt.put(path(T("key5/key"), CHTYPE('/')), T("5"));
    pt.put(path(T("key6/key/key"), CHTYPE('/')), T("6"));

    // Check instances count
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 13);

    // Do correct extractions
    BOOST_CHECK(pt.get(T("key1"), 0) == 1);
    BOOST_CHECK(pt.get(T("key2.key"), 0) == 2);
    BOOST_CHECK(pt.get(T("key3.key.key"), 0) == 3);
    BOOST_CHECK(pt.get(path(T("key4"), CHTYPE('/')), 0) == 4);
    BOOST_CHECK(pt.get(path(T("key5/key"), CHTYPE('/')), 0) == 5);
    BOOST_CHECK(pt.get(path(T("key6/key/key"), CHTYPE('/')), 0) == 6);

    // Do incorrect extractions
    BOOST_CHECK(pt.get(T("key2/key"), 0) == 0);
    BOOST_CHECK(pt.get(T("key3/key/key"), 0) == 0);
    BOOST_CHECK(pt.get(path(T("key5.key"), CHTYPE('/')), 0) == 0);
    BOOST_CHECK(pt.get(path(T("key6.key.key"), CHTYPE('/')), 0) == 0);

}

void test_path(PTREE *)
{

    typedef PTREE::path_type path;

    // Insert
    PTREE pt;
    pt.put(T("key1.key2.key3"), 1);

    // Test operator /=
    {
        path p;
        p /= T("key1"); p /= T("key2"); p /= T("key3");
        BOOST_CHECK(pt.get<int>(p, 0) == 1);
    }

    // Test operator /=
    {
        path p(T("key1"));
        p /= T("key2.key3");
        BOOST_CHECK(pt.get<int>(p, 0) == 1);
    }

    // Test operator /=
    {
        path p;
        path p1(T("key1.key2"));
        path p2(T("key3"));
        p /= p1;
        p /= p2;
        BOOST_CHECK(pt.get<int>(p, 0) == 1);
    }

    // Test operator /
    {
        path p = path(T("key1")) / T("key2.key3");
        BOOST_CHECK(pt.get<int>(p, 0) == 1);
    }

    // Test operator /
    {
        path p = T("key1.key2") / path(T("key3"));
        BOOST_CHECK(pt.get<int>(p, 0) == 1);
    }

}

void test_precision(PTREE *)
{

    typedef double real;

    // Quite precise PI value
    real pi = real(3.1415926535897932384626433832795028841971);

    // Put and get
    PTREE pt;
    pt.put_value(pi);
    real pi2 = pt.get_value<real>();

    // Test if precision is "good enough", i.e. if stream precision increase worked
    using namespace std;
    real error = abs(pi - pi2) *
        pow(real(numeric_limits<real>::radix), 
            real(numeric_limits<real>::digits));
    BOOST_CHECK(error < 100);

}

void test_locale(PTREE *)
{
    typedef boost::property_tree::translator_between<
        std::basic_string<CHTYPE>, double>::type translator;
    try
    {

        // Write strings in english and french locales
        PTREE pt;
#ifdef BOOST_WINDOWS
        std::locale loc_english("english");
        std::locale loc_french("french");
#else
        std::locale loc_english("en_GB");
        std::locale loc_french("fr_FR");
#endif
        pt.put(T("english"), 1.234, translator(loc_english));
        pt.put(T("french"), 1.234, translator(loc_french));

        // Test contents
        BOOST_CHECK(pt.get<PTREE::data_type>(T("english")) == T("1.234"));
        BOOST_CHECK(pt.get<PTREE::data_type>(T("french")) == T("1,234"));

    }
    catch (boost::property_tree::ptree_error &)
    {
        throw;
    }
    catch (std::runtime_error &e)
    {
        std::cerr << "Required locale not supported by the platform. "
                     "Skipping locale tests (caught std::runtime_error with message " << 
                     e.what() << ").\n";
    }

}

void test_custom_data_type(PTREE *)
{

    typedef std::basic_string<CHTYPE> Str;
    typedef PTREE::key_compare Comp;
    typedef PTREE::path_type Path;

    // Property_tree with boost::any as data type
    typedef boost::property_tree::basic_ptree<Str, boost::any, Comp> my_ptree;
    my_ptree pt;

    // Put/get int value
    pt.put(T("int value"), 3);
    int int_value = pt.get<int>(T("int value"));
    BOOST_CHECK(int_value == 3);

    // Put/get string value
    pt.put<std::basic_string<CHTYPE> >(T("string value"), T("foo bar"));
    std::basic_string<CHTYPE> string_value = pt.get<std::basic_string<CHTYPE> >(T("string value"));
    BOOST_CHECK(string_value == T("foo bar"));

    // Put/get list<int> value
    int list_data[] = { 1, 2, 3, 4, 5 };
    pt.put<std::list<int> >(T("list value"), std::list<int>(list_data, list_data + sizeof(list_data) / sizeof(*list_data)));
    std::list<int> list_value = pt.get<std::list<int> >(T("list value"));
    BOOST_CHECK(list_value.size() == 5);
    BOOST_CHECK(list_value.front() == 1);
    BOOST_CHECK(list_value.back() == 5);

}

void test_empty_size_max_size(PTREE *)
{

    PTREE pt;
    BOOST_CHECK(pt.max_size());
    BOOST_CHECK(pt.empty());
    BOOST_CHECK(pt.size() == 0);

    pt.put(T("test1"), 1);
    BOOST_CHECK(pt.max_size());
    BOOST_CHECK(!pt.empty());
    BOOST_CHECK(pt.size() == 1);

    pt.put(T("test2"), 2);
    BOOST_CHECK(pt.max_size());
    BOOST_CHECK(!pt.empty());
    BOOST_CHECK(pt.size() == 2);

}

void test_ptree_bad_path(PTREE *)
{

    PTREE pt;

    try
    {
        pt.get<int>(T("non.existent.path"));
    }
    catch (boost::property_tree::ptree_bad_path &e)
    {
        PTREE::path_type path = e.path<PTREE::path_type>();
        std::string what = e.what();
        BOOST_CHECK(what.find("non.existent.path") != std::string::npos);
        return;
    }

    BOOST_ERROR("No required exception thrown");

}

void test_ptree_bad_data(PTREE *)
{

    PTREE pt;
    pt.put_value("non convertible to int");

    try
    {
        pt.get_value<int>();
    }
    catch (boost::property_tree::ptree_bad_data &e)
    {
        PTREE::data_type data = e.data<PTREE::data_type>();
        std::string what = e.what();
        // FIXME: Bring back what translation or make it more clear that it
        // doesn't work.
        //BOOST_CHECK(what.find("non convertible to int") != std::string::npos);
        return;
    }

    BOOST_ERROR("No required exception thrown");
}

void test_serialization(PTREE *)
{

    // Prepare test tree
    PTREE pt;
    pt.put_value(1);
    pt.put(T("key1"), 3);
    pt.put(T("key1.key11)"), 3.3);
    pt.put(T("key1.key12"), T("foo"));
    pt.put(T("key2"), true);
    pt.put(T("key2.key21.key211.key2111.key21111"), T("super deep!"));
    pt.put_child(T("empty"), PTREE());
    pt.put(T("loooooong"), PTREE::data_type(10000, CHTYPE('a')));

    // Endforce const for input
    const PTREE &pt1 = pt;
    
    // Test text archives
    {
        std::stringstream stream;
        boost::archive::text_oarchive oa(stream);
        oa & pt1;
        boost::archive::text_iarchive ia(stream);
        PTREE pt2;
        ia & pt2;
        BOOST_CHECK(pt1 == pt2);
    }
    
    // Test binary archives
    {
        std::stringstream stream;
        boost::archive::binary_oarchive oa(stream);
        oa & pt1;
        boost::archive::binary_iarchive ia(stream);
        PTREE pt2;
        ia & pt2;
        BOOST_CHECK(pt1 == pt2);
    }

    // Test XML archives
    {
        std::stringstream stream;
        boost::archive::xml_oarchive oa(stream);
        oa & boost::serialization::make_nvp("pt", pt1);
        boost::archive::xml_iarchive ia(stream);
        PTREE pt2;
        ia & boost::serialization::make_nvp("pt", pt2);
        BOOST_CHECK(pt1 == pt2);
    }

}

void test_bool(PTREE *)
{

    // Prepare test tree
    PTREE pt;
    pt.put(T("bool.false.1"), false);
    pt.put(T("bool.false.2"), T("0"));
    pt.put(T("bool.true.1"), true);
    pt.put(T("bool.true.2"), 1);
    pt.put(T("bool.invalid.1"), T(""));
    pt.put(T("bool.invalid.2"), T("tt"));
    pt.put(T("bool.invalid.3"), T("ff"));
    pt.put(T("bool.invalid.4"), T("2"));
    pt.put(T("bool.invalid.5"), T("-1"));

    // Test false
    for (PTREE::iterator it = pt.get_child(T("bool.false")).begin(); it != pt.get_child(T("bool.false")).end(); ++it)
        BOOST_CHECK(it->second.get_value<bool>() == false);

    // Test true
    for (PTREE::iterator it = pt.get_child(T("bool.true")).begin(); it != pt.get_child(T("bool.true")).end(); ++it)
        BOOST_CHECK(it->second.get_value<bool>() == true);

    // Test invalid
    for (PTREE::iterator it = pt.get_child(T("bool.invalid")).begin(); it != pt.get_child(T("bool.invalid")).end(); ++it)
    {
        BOOST_CHECK(it->second.get_value<bool>(false) == false);
        BOOST_CHECK(it->second.get_value<bool>(true) == true);
    }

}

void test_char(PTREE *)
{

    // Prepare test tree
    PTREE pt;
#if WIDECHAR == 0
    pt.put(T("char"), char('A'));
#endif
    pt.put(T("signed char"), static_cast<signed char>('A'));
    pt.put(T("unsigned char"), static_cast<unsigned char>('A'));
    pt.put(T("signed char min"), (std::numeric_limits<signed char>::min)());
    pt.put(T("signed char max"), (std::numeric_limits<signed char>::max)());
    pt.put(T("unsigned char min"), (std::numeric_limits<unsigned char>::min)());
    pt.put(T("unsigned char max"), (std::numeric_limits<unsigned char>::max)());

    // Verify normal conversions
#if WIDECHAR == 0
    BOOST_CHECK(pt.get<char>(T("char")) == 'A');
#endif
    BOOST_CHECK(pt.get<signed char>(T("signed char")) ==
        static_cast<signed char>('A'));
    BOOST_CHECK(pt.get<unsigned char>(T("unsigned char")) ==
        static_cast<unsigned char>('A'));

    // Verify that numbers are saved for signed and unsigned char
    BOOST_CHECK(pt.get<int>(T("signed char")) == int('A'));
    BOOST_CHECK(pt.get<int>(T("unsigned char")) == int('A'));

    // Verify ranges
    BOOST_CHECK(pt.get<signed char>(T("signed char min")) ==
        (std::numeric_limits<signed char>::min)());
    BOOST_CHECK(pt.get<signed char>(T("signed char max")) ==
        (std::numeric_limits<signed char>::max)());
    BOOST_CHECK(pt.get<unsigned char>(T("unsigned char min")) ==
        (std::numeric_limits<unsigned char>::min)());
    BOOST_CHECK(pt.get<unsigned char>(T("unsigned char max")) ==
        (std::numeric_limits<unsigned char>::max)());

}

void test_sort(PTREE *)
{
  PTREE pt;
  pt.put(T("one"), T("v1"));
  pt.put(T("two"), T("v2"));
  pt.put(T("three"), T("v3"));
  pt.put(T("four"), T("v4"));

  pt.sort();

  PTREE::iterator it = pt.begin();
  BOOST_CHECK(std::distance(it, pt.end()) == 4);
  BOOST_CHECK(it->first == T("four"));
  BOOST_CHECK(it->second.data() == T("v4"));
  ++it;
  BOOST_CHECK(it->first == T("one"));
  BOOST_CHECK(it->second.data() == T("v1"));
  ++it;
  BOOST_CHECK(it->first == T("three"));
  BOOST_CHECK(it->second.data() == T("v3"));
  ++it;
  BOOST_CHECK(it->first == T("two"));
  BOOST_CHECK(it->second.data() == T("v2"));
}

void test_leaks(PTREE *)
{
    //BOOST_CHECK(PTREE::debug_get_instances_count() == 0);
}

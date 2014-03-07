

// Boost.Range ATL Extension
//
// Copyright Shunsuke Sogame 2005-2006.
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)


// #include <pstade/vodka/drink.hpp>

#include <boost/test/test_tools.hpp>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define BOOST_LIB_NAME boost_test_exec_monitor
#include <boost/config/auto_link.hpp>

#define BOOST_RANGE_DETAIL_MICROSOFT_TEST
#include <boost/range/atl.hpp> // can be placed first


#include <map>
#include <string>
#include <boost/concept_check.hpp>
#include <boost/mpl/if.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/distance.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>


#include <boost/foreach.hpp>


#include <atlbase.h> // for ATL3 CSimpleArray/CSimpleValArray
#if !(_ATL_VER < 0x0700)
    #include <atlcoll.h>
    #include <cstringt.h>
    #include <atlsimpstr.h>
    #include <atlstr.h>
#endif


namespace brdm = boost::range_detail_microsoft;


#if !(_ATL_VER < 0x0700)


template< class ArrayT, class SampleRange >
bool test_init_auto_ptr_array(ArrayT& arr, SampleRange& sample)
{
    typedef typename boost::range_iterator<SampleRange>::type iter_t;

    for (iter_t it = boost::begin(sample), last = boost::end(sample); it != last; ++it) {
        arr.Add(*it); // moves ownership
    }

    return boost::distance(arr) == boost::distance(sample);
}


template< class ListT, class SampleRange >
bool test_init_auto_ptr_list(ListT& lst, SampleRange& sample)
{
    typedef typename boost::range_iterator<SampleRange>::type iter_t;
    typedef typename boost::range_value<SampleRange>::type val_t;

    for (iter_t it = boost::begin(sample), last = boost::end(sample); it != last; ++it) {
        lst.AddTail(*it); // moves ownership
    }

    return boost::distance(lst) == boost::distance(sample);
}


// Workaround:
// CRBTree provides no easy access function, but yes, it is the range!
//
template< class AtlMapT, class KeyT, class MappedT >
bool test_atl_map_has(AtlMapT& map, const KeyT& k, const MappedT m)
{
    typedef typename boost::range_iterator<AtlMapT>::type iter_t;

    for (iter_t it = boost::begin(map), last = boost::end(map); it != last; ++it) {
        if (it->m_key == k && it->m_value == m)
            return true;
    }

    return false;
}


template< class AtlMapT, class MapT >
bool test_atl_map(AtlMapT& map, const MapT& sample)
{
    typedef typename boost::range_iterator<AtlMapT>::type iter_t;
    typedef typename boost::range_const_iterator<MapT>::type siter_t;

    bool result = true;

    result = result && (boost::distance(map) == boost::distance(sample));
    if (!result)
        return false;

    {
        for (iter_t it = boost::begin(map), last = boost::end(map); it != last; ++it) {
            result = result && brdm::test_find_key_and_mapped(sample, std::make_pair(it->m_key, it->m_value));
        }
    }

    {
        for (siter_t it = boost::begin(sample), last = boost::end(sample); it != last; ++it) {
            result = result && (test_atl_map_has)(map, it->first, it->second);
        }
    }

    return result;
}


template< class MapT, class SampleMap >
bool test_init_atl_multimap(MapT& map, const SampleMap& sample)
{
    typedef typename boost::range_const_iterator<SampleMap>::type iter_t;

    for (iter_t it = boost::const_begin(sample), last = boost::const_end(sample); it != last; ++it) {
        map.Insert(it->first, it->second);
    }

    return boost::distance(map) == boost::distance(sample);
}


// arrays
//

template< class Range >
void test_CAtlArray(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ATL::CAtlArray<val_t> rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, val_t *>::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, val_t const*>::value ));

    rng_t rng;
    BOOST_CHECK( brdm::test_init_array(rng, sample) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class ValT, class Range >
void test_CAutoPtrArray(Range& sample)
{
    typedef ValT val_t;

    typedef ATL::CAutoPtrArray<val_t> rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, boost::indirect_iterator< ATL::CAutoPtr<val_t> *> >::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, boost::indirect_iterator< ATL::CAutoPtr<val_t> const*> >::value ));

    rng_t rng;
    BOOST_CHECK( ::test_init_auto_ptr_array(rng, sample) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class I, class Range >
void test_CInterfaceArray(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ATL::CInterfaceArray<I> rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, ATL::CComQIPtr<I> * >::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, ATL::CComQIPtr<I> const* >::value ));

    rng_t rng;
    BOOST_CHECK( brdm::test_init_array(rng, sample) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


// lists
//

template< class Range >
void test_CAtlList(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ATL::CAtlList<val_t> rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter< rng_t, brdm::list_iterator<rng_t,       val_t> >::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  < rng_t, brdm::list_iterator<rng_t const, val_t const> >::value ));

    rng_t rng;
    BOOST_CHECK( brdm::test_init_list(rng, sample) );
    BOOST_CHECK( brdm::test_bidirectional(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class ValT, class Range >
void test_CAutoPtrList(Range& sample)
{
    typedef ValT val_t;

    typedef ATL::CAutoPtrList<val_t> rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter< rng_t, boost::indirect_iterator< brdm::list_iterator<rng_t,       ATL::CAutoPtr<val_t> > > >::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  < rng_t, boost::indirect_iterator< brdm::list_iterator<rng_t const, ATL::CAutoPtr<val_t> const> > >::value ));

    rng_t rng;
    BOOST_CHECK( ::test_init_auto_ptr_list(rng, sample) );
    BOOST_CHECK( brdm::test_bidirectional(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class ValT, class Range >
void test_CHeapPtrList(const Range& sample)
{
    typedef ValT val_t;

    typedef ATL::CHeapPtrList<val_t> rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter< rng_t, boost::indirect_iterator< brdm::list_iterator<rng_t,       ATL::CHeapPtr<val_t> > > >::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  < rng_t, boost::indirect_iterator< brdm::list_iterator<rng_t const, ATL::CHeapPtr<val_t> const> > >::value ));

    rng_t rng;
    BOOST_CHECK( ::test_init_auto_ptr_list(rng, sample) );
    BOOST_CHECK( brdm::test_bidirectional(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class I, class Range >
void test_CInterfaceList(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ATL::CInterfaceList<I> rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter< rng_t, brdm::list_iterator<rng_t,       ATL::CComQIPtr<I> > >::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  < rng_t, brdm::list_iterator<rng_t const, ATL::CComQIPtr<I> const> >::value ));

    rng_t rng;
    BOOST_CHECK( brdm::test_init_list(rng, sample) );
    BOOST_CHECK( brdm::test_bidirectional(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


// strings
//

template< class Range >
void test_CSimpleStringT(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef typename boost::mpl::if_< boost::is_same<val_t, char>,
        ATL::CAtlStringA,
        ATL::CAtlStringW
    >::type derived_t;

    typedef ATL::CSimpleStringT<val_t> rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, typename rng_t::PXSTR>::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, typename rng_t::PCXSTR>::value ));

    derived_t drng;
    rng_t& rng = drng;
    BOOST_CHECK( brdm::test_init_string(rng, sample) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    // BOOST_CHECK( brdm::test_emptiness(rng) ); no default constructible
}


template< int n, class Range >
void test_CFixedStringT(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef typename boost::mpl::if_< boost::is_same<val_t, char>,
        ATL::CAtlStringA,
        ATL::CAtlStringW
    >::type base_t;

    typedef ATL::CFixedStringT<base_t, n> rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, typename rng_t::PXSTR>::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, typename rng_t::PCXSTR>::value ));

    rng_t rng;
    BOOST_CHECK( brdm::test_init_string(rng, sample) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class Range >
void test_CStringT(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef typename boost::mpl::if_< boost::is_same<val_t, char>,
        ATL::CAtlStringA, // == CStringT<char, X>
        ATL::CAtlStringW  // == CStringT<wchar_t, X>
    >::type rng_t;

    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, typename rng_t::PXSTR>::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, typename rng_t::PCXSTR>::value ));

    rng_t rng;
    BOOST_CHECK( brdm::test_init_string(rng, sample) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class Range >
void test_CStaticString(const Range& sample)
{
#if !defined(BOOST_RANGE_ATL_NO_TEST_UNDOCUMENTED_RANGE)
    {
        typedef ATL::CStaticString<char, 20> rng_t;
        BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, char const *>::value ));
        BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, char const *>::value ));

        rng_t rng("hello static string");
        BOOST_CHECK( *(boost::begin(rng)+4) == 'o' );
        BOOST_CHECK( *(boost::end(rng)-3) == 'i' );
    }

    {
        typedef ATL::CStaticString<wchar_t, 40> rng_t;
        BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, wchar_t const *>::value ));
        BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, wchar_t const *>::value ));

        rng_t rng(L"hello static string");
        BOOST_CHECK( *(boost::begin(rng)+4) == L'o' );
        BOOST_CHECK( *(boost::end(rng)-3) == L'i' );
    }
#endif

    (void)sample; // unused
}


#endif // !(_ATL_VER < 0x0700) 


template< class Range >
void test_CComBSTR(const Range& sample)
{
    typedef ATL::CComBSTR rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, OLECHAR *>::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, OLECHAR const*>::value ));

    rng_t rng(OLESTR("hello CComBSTR range!"));
    BOOST_CHECK( brdm::test_equals(rng, std::string("hello CComBSTR range!")) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );

    (void)sample; // unused
}


// simples
//

template< class Range >
void test_CSimpleArray(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ATL::CSimpleArray<val_t> rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, val_t *>::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, val_t const*>::value ));

    rng_t rng;
    BOOST_CHECK( brdm::test_init_array(rng, sample) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class Range >
void test_CSimpleMap(const Range& sample)
{
#if !defined(BOOST_RANGE_ATL_NO_TEST_UNDOCUMENTED_RANGE)

    typedef ATL::CSimpleMap<int, double> rng_t;

    rng_t rng;
    rng.Add(3, 3.0);
    rng.Add(4, 2.0);

    BOOST_CHECK( boost::begin(rng)->get<0>() == 3.0 );
    BOOST_CHECK( (boost::end(rng)-1)->get<1>() == 2.0 );

#endif

    (void)sample; // unused
}


template< class Range >
void test_CSimpleValArray(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ATL::CSimpleArray<val_t> rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, val_t *>::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, val_t const*>::value ));

    rng_t rng;
    BOOST_CHECK( brdm::test_init_array(rng, sample) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


// maps
//

template< class MapT >
void test_CAtlMap(const MapT& sample)
{
    typedef typename MapT::key_type k_t;
    typedef typename MapT::mapped_type m_t;

    typedef ATL::CAtlMap<k_t, m_t> rng_t;

    rng_t rng;
    boost::function_requires< boost::ForwardRangeConcept<rng_t> >();
    BOOST_CHECK( brdm::test_init_map(rng, sample) );
    BOOST_CHECK( ::test_atl_map(rng, sample) );
}


template< class MapT >
void test_CRBTree(const MapT& sample)
{
    typedef typename MapT::key_type k_t;
    typedef typename MapT::mapped_type m_t;

    typedef ATL::CRBMap<k_t, m_t> derived_t;
    typedef ATL::CRBTree<k_t, m_t> rng_t;

    derived_t drng;
    rng_t& rng = drng;

    boost::function_requires< boost::BidirectionalRangeConcept<rng_t> >();
    BOOST_CHECK( brdm::test_init_map(drng, sample) );
    BOOST_CHECK( ::test_atl_map(rng, sample) );
}


template< class MapT >
void test_CRBMap(const MapT& sample)
{
    typedef typename MapT::key_type k_t;
    typedef typename MapT::mapped_type m_t;

    typedef ATL::CRBMap<k_t, m_t> rng_t;

    rng_t rng;
    boost::function_requires< boost::BidirectionalRangeConcept<rng_t> >();
    BOOST_CHECK( brdm::test_init_map(rng, sample) );
    BOOST_CHECK( ::test_atl_map(rng, sample) );
}


template< class MapT >
void test_CRBMultiMap(const MapT& sample)
{
    typedef typename MapT::key_type k_t;
    typedef typename MapT::mapped_type m_t;

    typedef ATL::CRBMultiMap<k_t, m_t> rng_t;

    rng_t rng;
    boost::function_requires< boost::BidirectionalRangeConcept<rng_t> >();
    BOOST_CHECK( ::test_init_atl_multimap(rng, sample) );
    BOOST_CHECK( ::test_atl_map(rng, sample) );
}


// main test
//

void test_atl()
{

    // ordinary ranges
    //
    {
        std::string sample("rebecca judy and mary whiteberry chat monchy");
#if !(_ATL_VER < 0x0700)
        ::test_CAtlArray(sample);
        ::test_CAtlList(sample);
        ::test_CSimpleStringT(sample);
        ::test_CFixedStringT<44>(sample);
        ::test_CStringT(sample);
        ::test_CStaticString(sample);
#endif
        ::test_CComBSTR(sample);
        ::test_CSimpleArray(sample);
        ::test_CSimpleMap(sample);
        ::test_CSimpleValArray(sample);
    }


    {
        std::wstring sample(L"rebecca judy and mary whiteberry chat monchy");
#if !(_ATL_VER < 0x0700)
        ::test_CAtlArray(sample);
        ::test_CAtlList(sample);
        ::test_CSimpleStringT(sample);
        ::test_CFixedStringT<44>(sample);
        ::test_CStringT(sample);
        ::test_CStaticString(sample);
#endif
        ::test_CComBSTR(sample);
        ::test_CSimpleArray(sample);
        ::test_CSimpleMap(sample);
        ::test_CSimpleValArray(sample);
    }

    // pointer ranges
    //
#if !(_ATL_VER < 0x0700)
    {
        typedef ATL::CAutoPtr<int> ptr_t;
        ptr_t
            ptr0(new int(3)), ptr1(new int(4)), ptr2(new int(5)), ptr3(new int(4)),
            ptr4(new int(1)), ptr5(new int(2)), ptr6(new int(4)), ptr7(new int(0));

        ptr_t ptrs[8] = {
            ptr0, ptr1, ptr2, ptr3, ptr4, ptr5, ptr6, ptr7
        };

        boost::iterator_range< ptr_t * > workaround(ptrs, ptrs+8);
        ::test_CAutoPtrArray<int>(workaround);
    }

    {
        typedef ATL::CAutoPtr<int> ptr_t;
        ptr_t
            ptr0(new int(3)), ptr1(new int(4)), ptr2(new int(5)), ptr3(new int(4)),
            ptr4(new int(1)), ptr5(new int(2)), ptr6(new int(4)), ptr7(new int(0));

        ptr_t ptrs[8] = {
            ptr0, ptr1, ptr2, ptr3, ptr4, ptr5, ptr6, ptr7
        };

        boost::iterator_range< ptr_t * > workaround(ptrs, ptrs+8);
        ::test_CAutoPtrList<int>(workaround);
    }

    {
        typedef ATL::CHeapPtr<int> ptr_t;
        ptr_t ptrs[5]; {
            ptrs[0].AllocateBytes(sizeof(int));
            ptrs[1].AllocateBytes(sizeof(int));
            ptrs[2].AllocateBytes(sizeof(int));
            ptrs[3].AllocateBytes(sizeof(int));
            ptrs[4].AllocateBytes(sizeof(int));
        }

        boost::iterator_range< ptr_t * > workaround(ptrs, ptrs+5);
        ::test_CHeapPtrList<int>(workaround);
    }


    {
        typedef ATL::CComQIPtr<IDispatch> ptr_t;
        ptr_t ptrs[8];

        boost::iterator_range< ptr_t * > workaround(ptrs, ptrs+8);
        ::test_CInterfaceArray<IDispatch>(workaround);
        ::test_CInterfaceList<IDispatch>(workaround);
    }
#endif

    // maps
    //
    {
#if !(_ATL_VER < 0x0700)
        std::map<int, std::string> sample; {
            sample[0] = "hello";
            sample[1] = "range";
            sample[2] = "atl";
            sample[3] = "mfc";
            sample[4] = "collections";
        }

        ::test_CAtlMap(sample);
        ::test_CRBTree(sample);
        ::test_CRBMap(sample);
        ::test_CRBMultiMap(sample);
#endif
    }


} // test_atl


#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;


test_suite *
init_unit_test_suite(int argc, char* argv[])
{
    test_suite *test = BOOST_TEST_SUITE("ATL Range Test Suite");
    test->add(BOOST_TEST_CASE(&test_atl));

    (void)argc, (void)argv; // unused
    return test;
}

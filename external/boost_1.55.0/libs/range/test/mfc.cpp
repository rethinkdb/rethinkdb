// Boost.Range MFC Extension
//
// Copyright Shunsuke Sogame 2005-2006.
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)


#include <afx.h> // must be here

// #include <pstade/vodka/drink.hpp>

#include <boost/test/test_tools.hpp>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define BOOST_LIB_NAME boost_test_exec_monitor
#include <boost/config/auto_link.hpp>

#define BOOST_RANGE_DETAIL_MICROSOFT_TEST
#include <boost/range/mfc.hpp> // can be placed first


#include <map>
#include <boost/concept_check.hpp>
// #include <boost/foreach.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/end.hpp>
#include <boost/static_assert.hpp>
#include <boost/algorithm/string.hpp>


#include <afx.h>
#include <afxcoll.h>
#include <afxtempl.h>

#if !(_ATL_VER < 0x0700)
    #include <cstringt.h>
    #include <atlsimpstr.h>
    #include <atlstr.h>
#endif


namespace brdm = boost::range_detail_microsoft;


// helpers
//

template< class MfcMapT, class MapT >
bool test_mfc_map(MfcMapT& map, const MapT& sample)
{
    typedef typename boost::range_iterator<MfcMapT>::type iter_t;
    typedef typename boost::range_const_iterator<MapT>::type siter_t;

    bool result = true;

    result = result && (boost::distance(map) == boost::distance(sample));
    if (!result)
        return false;

    {
        for (iter_t it = boost::begin(map), last = boost::end(map); it != last; ++it) {
            result = result && brdm::test_find_key_and_mapped(sample, *it);
        }
    }

    {
        for (siter_t it = boost::begin(sample), last = boost::end(sample); it != last; ++it) {
            result = result && (map[it->first] == it->second);
        }
    }

    return result;
}


template< class MfcMapT, class MapT >
bool test_mfc_cpair_map(MfcMapT& map, const MapT& sample)
{
    typedef typename boost::range_iterator<MfcMapT>::type iter_t;
    typedef typename boost::range_const_iterator<MapT>::type siter_t;

    bool result = true;

    result = result && (boost::distance(map) == boost::distance(sample));
    if (!result)
        return false;

    {
        for (iter_t it = boost::begin(map), last = boost::end(map); it != last; ++it) {
            result = result && brdm::test_find_key_and_mapped(sample, std::make_pair(it->key, it->value));
        }
    }

    {
        for (siter_t it = boost::begin(sample), last = boost::end(sample); it != last; ++it) {
            result = result && (map[it->first] == it->second);
        }
    }

    return result;
}


// arrays
//
template< class Range >
void test_CByteArray(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ::CByteArray rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, BYTE *>::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, BYTE const*>::value ));

    rng_t rng;
    BOOST_CHECK( brdm::test_init_array(rng, sample) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class Range >
void test_CDWordArray(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ::CDWordArray rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, DWORD *>::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, DWORD const*>::value ));

    rng_t rng;
    BOOST_CHECK( brdm::test_init_array(rng, sample) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class Range >
void test_CObArray(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ::CObArray rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, brdm::mfc_ptr_array_iterator<rng_t, ::CObject *> >::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, brdm::mfc_ptr_array_iterator<const rng_t, const ::CObject *> >::value ));

    rng_t rng;
    BOOST_CHECK( brdm::test_init_array(rng, sample) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class Range >
void test_CPtrArray(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ::CPtrArray rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, brdm::mfc_ptr_array_iterator<rng_t, void *> >::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, brdm::mfc_ptr_array_iterator<const rng_t, const void *> >::value ));

    rng_t rng;
    BOOST_CHECK( brdm::test_init_array(rng, sample) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class Range >
void test_CStringArray(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ::CStringArray rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, ::CString *>::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, ::CString const *>::value ));

    rng_t rng;
    BOOST_CHECK( brdm::test_init_array(rng, sample) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class Range >
void test_CUIntArray(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ::CUIntArray rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, UINT *>::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, UINT const *>::value ));

    rng_t rng;
    BOOST_CHECK( brdm::test_init_array(rng, sample) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class Range >
void test_CWordArray(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ::CWordArray rng_t;
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, WORD *>::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, WORD const *>::value ));

    rng_t rng;
    BOOST_CHECK( brdm::test_init_array(rng, sample) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


// lists
//

template< class Range >
void test_CObList(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ::CObList rng_t;

    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter< rng_t, brdm::list_iterator<rng_t,       ::CObject *> >::value ));
#if !defined(BOOST_RANGE_MFC_CONST_COL_RETURNS_NON_REF)
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  < rng_t, brdm::list_iterator<rng_t const, ::CObject const *> >::value ));
#else
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  < rng_t, brdm::list_iterator<rng_t const, ::CObject const * const, ::CObject const * const> >::value ));
#endif

    rng_t rng;
    BOOST_CHECK( brdm::test_init_list(rng, sample) );
    BOOST_CHECK( brdm::test_bidirectional(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class Range >
void test_CPtrList(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ::CPtrList rng_t;

    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter< rng_t, brdm::list_iterator<rng_t,       void *> >::value ));
#if !defined(BOOST_RANGE_MFC_CONST_COL_RETURNS_NON_REF)
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  < rng_t, brdm::list_iterator<rng_t const, void const *> >::value ));
#else
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  < rng_t, brdm::list_iterator<rng_t const, void const * const, void const * const> >::value ));
#endif

    rng_t rng;
    BOOST_CHECK( brdm::test_init_list(rng, sample) );
    BOOST_CHECK( brdm::test_bidirectional(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class Range >
void test_CStringList(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ::CStringList rng_t;

    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter< rng_t, brdm::list_iterator<rng_t,       ::CString> >::value ));
#if !defined(BOOST_RANGE_MFC_CONST_COL_RETURNS_NON_REF)
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  < rng_t, brdm::list_iterator<rng_t const, ::CString const> >::value ));
#else
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  < rng_t, brdm::list_iterator<rng_t const, ::CString const, ::CString const> >::value ));
#endif

    rng_t rng;
    BOOST_CHECK( brdm::test_init_list(rng, sample) );
    BOOST_CHECK( brdm::test_bidirectional(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


// maps
//

template< class MapT >
void test_CMapPtrToWord(const MapT& sample)
{
    typedef ::CMapPtrToWord rng_t;

    rng_t rng;
    boost::function_requires< boost::ForwardRangeConcept<rng_t> >();
    BOOST_CHECK( brdm::test_init_map(rng, sample) );
    BOOST_CHECK( ::test_mfc_map(rng, sample) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class MapT >
void test_CMapPtrToPtr(const MapT& sample)
{
    typedef ::CMapPtrToPtr rng_t;

    rng_t rng;
    boost::function_requires< boost::ForwardRangeConcept<rng_t> >();
    BOOST_CHECK( brdm::test_init_map(rng, sample) );
    BOOST_CHECK( ::test_mfc_map(rng, sample) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class MapT >
void test_CMapStringToOb(const MapT& sample)
{
    typedef ::CMapStringToOb rng_t;

    rng_t rng;
    boost::function_requires< boost::ForwardRangeConcept<rng_t> >();
    BOOST_CHECK( brdm::test_init_map(rng, sample) );
    BOOST_CHECK( ::test_mfc_map(rng, sample) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class MapT >
void test_CMapStringToPtr(const MapT& sample)
{
    typedef ::CMapStringToPtr rng_t;

    rng_t rng;
    boost::function_requires< boost::ForwardRangeConcept<rng_t> >();
    BOOST_CHECK( brdm::test_init_map(rng, sample) );
    BOOST_CHECK( ::test_mfc_map(rng, sample) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class MapT >
void test_CMapStringToString(const MapT& sample)
{
    typedef ::CMapStringToString rng_t;

    rng_t rng;
    boost::function_requires< boost::ForwardRangeConcept<rng_t> >();
    BOOST_CHECK( brdm::test_init_map(rng, sample) );
#if !defined(BOOST_RANGE_MFC_NO_CPAIR)
    BOOST_CHECK( ::test_mfc_cpair_map(rng, sample) );
#endif
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class MapT >
void test_CMapWordToOb(const MapT& sample)
{
    typedef ::CMapWordToOb rng_t;

    rng_t rng;
    boost::function_requires< boost::ForwardRangeConcept<rng_t> >();
    BOOST_CHECK( brdm::test_init_map(rng, sample) );
    BOOST_CHECK( ::test_mfc_map(rng, sample) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class MapT >
void test_CMapWordToPtr(const MapT& sample)
{
    typedef ::CMapWordToPtr rng_t;

    rng_t rng;
    boost::function_requires< boost::ForwardRangeConcept<rng_t> >();
    BOOST_CHECK( brdm::test_init_map(rng, sample) );
    BOOST_CHECK( ::test_mfc_map(rng, sample) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


// templates
//

template< class Range >
void test_CArray(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ::CArray<val_t, const val_t&> rng_t; // An old MFC needs the second template argument.
    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, val_t *>::value ));
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, val_t const*>::value ));

    rng_t rng;
    BOOST_CHECK( brdm::test_init_array(rng, sample) );
    BOOST_CHECK( brdm::test_random_access(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class Range >
void test_CList(const Range& sample)
{
    typedef typename boost::range_value<Range>::type val_t;

    typedef ::CList<val_t, const val_t&> rng_t;

    BOOST_STATIC_ASSERT(( brdm::test_mutable_iter< rng_t, brdm::list_iterator<rng_t,       val_t> >::value ));
#if !defined(BOOST_RANGE_MFC_CONST_COL_RETURNS_NON_REF)
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  < rng_t, brdm::list_iterator<rng_t const, val_t const> >::value ));
#else
    BOOST_STATIC_ASSERT(( brdm::test_const_iter  < rng_t, brdm::list_iterator<rng_t const, val_t const, val_t const> >::value ));
#endif

    rng_t rng;
    BOOST_CHECK( brdm::test_init_list(rng, sample) );
    BOOST_CHECK( brdm::test_bidirectional(rng) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


template< class MapT >
void test_CMap(const MapT& sample)
{
    typedef typename MapT::key_type k_t;
    typedef typename MapT::mapped_type m_t;

    typedef ::CMap<k_t, const k_t&, m_t, const m_t&> rng_t;

    rng_t rng;
    boost::function_requires< boost::ForwardRangeConcept<rng_t> >();
    BOOST_CHECK( brdm::test_init_map(rng, sample) );
#if !defined(BOOST_RANGE_MFC_NO_CPAIR)
    BOOST_CHECK( ::test_mfc_cpair_map(rng, sample) );
#endif
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


void test_CTypedPtrArray()
{
    typedef ::CTypedPtrArray< ::CPtrArray, int * > rng_t;
    boost::function_requires< boost::RandomAccessRangeConcept<rng_t> >();

    rng_t rng;
    int o1, o2, o3, o4, o5;
    int *data[] = { &o1, &o2, &o3, &o4, &o5 };
    BOOST_CHECK( brdm::test_init_array(rng, boost::make_iterator_range(data, data+5)) );

    BOOST_CHECK( *(boost::begin(rng) + 2) == &o3 );
    BOOST_CHECK( *(boost::end(rng) - 1) == &o5 );        

    // BOOST_CHECK( brdm::test_random_access(rng) ); this range is not mutable
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


void test_CTypedPtrList()
{
    typedef ::CTypedPtrList< ::CObList, ::CObList * > rng_t;
    boost::function_requires< boost::BidirectionalRangeConcept<rng_t> >();

    rng_t rng;

    ::CObList o1, o2, o3, o4, o5;
    ::CObList *data[] = { &o1, &o2, &o3, &o4, &o5 };
    BOOST_CHECK( brdm::test_init_list(rng, data) );

    boost::range_iterator<rng_t>::type it = boost::begin(rng);
    std::advance(it, 1);
    BOOST_CHECK( *it == &o2 );
    std::advance(it, 2);
    BOOST_CHECK( *it == &o4 );

    // BOOST_CHECK( brdm::test_bidirectional(rng) ); this range is not mutable
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


void test_CTypedPtrMap()
{
    typedef ::CTypedPtrMap< ::CMapStringToPtr, ::CString, int *> rng_t;
    boost::function_requires< boost::ForwardRangeConcept<rng_t> >();

    rng_t rng;
    ::CString o0(_T('a')), o1(_T('c')), o2(_T('f')), o3(_T('q')), o4(_T('g'));
    int d0, d1, d2, d3, d4;
    std::map< ::CString, int * > data;
    data[o0] = &d0, data[o1] = &d1, data[o2] = &d2, data[o3] = &d3, data[o4] = &d4;

    BOOST_CHECK( brdm::test_init_map(rng, data) );
    BOOST_CHECK( ::test_mfc_map(rng, data) );
    BOOST_CHECK( brdm::test_emptiness(rng) );
}


// strings
//
#if defined(BOOST_RANGE_MFC_HAS_LEGACY_STRING)

    template< class Range >
    void test_CString(const Range& sample)
    {
        typedef typename boost::range_value<Range>::type val_t;

        typedef ::CString rng_t; // An old MFC needs the second template argument.
        BOOST_STATIC_ASSERT(( brdm::test_mutable_iter<rng_t, TCHAR *>::value ));
        BOOST_STATIC_ASSERT(( brdm::test_const_iter  <rng_t, TCHAR const*>::value ));

        rng_t rng;
        BOOST_CHECK( brdm::test_init_string(rng, sample) );
        BOOST_CHECK( brdm::test_random_access(rng) );
        BOOST_CHECK( brdm::test_emptiness(rng) );
    }

#endif


struct CPerson
{
    void hello_range() { };
};


void test_mfc()
{
#if 0
    // overview
    //
    {
        CTypedPtrArray<CPtrArray, CList<CString> *> myArray;
        // ...
        BOOST_FOREACH (CList<CString> *theList, myArray)
        {
            BOOST_FOREACH (CString& str, *theList)
            {
                boost::to_upper(str);
                std::sort(boost::begin(str), boost::end(str));
                // ...
            }
        }
    }
#endif

    // arrays
    //
    {
        BYTE data[] = { 4,5,1,3,5,12,3,1,3,1,6,1,3,60,1,1,5,1,3,1,10 };

        ::test_CByteArray(boost::make_iterator_range(data, data+22));
    }

    {
        DWORD data[] = { 4,5,1,3,5,12,3,1,3,1,6,1,3,60,1,1,5,1,3,1,10 };

        test_CDWordArray(boost::make_iterator_range(data, data+22));
    }

    {
        ::CObArray o1, o2, o3, o4, o5;
        ::CObject *data[] = { &o1, &o2, &o3, &o4, &o5 };

        ::test_CObArray(boost::make_iterator_range(data, data+5));
    }

    {
        ::CPtrArray o1, o2, o3, o4, o5;
        void *data[] = { &o1, &o2, &o3, &o4, &o5 };

        ::test_CPtrArray(boost::make_iterator_range(data, data+5));
    }

    {
        ::CString data[] = {
            ::CString(_T('0')), ::CString(_T('1')), ::CString(_T('2')), ::CString(_T('3')),
            ::CString(_T('4')), ::CString(_T('5')), ::CString(_T('6')), ::CString(_T('7'))
        };

        ::test_CStringArray(boost::make_iterator_range(data, data+8));
    }

    {
        ::CUIntArray rng;
        UINT data[] = { 4,5,1,3,5,12,3,1,3,1,6,1,3,60,1,1,5,1,3,1,10 };

        ::test_CUIntArray(boost::make_iterator_range(data, data+22));
    }

    {
        ::CWordArray rng;
        WORD data[] = { 4,5,1,3,5,12,3,1,3,1,6,1,3,60,1,1,5,1,3,1,10 };

        ::test_CWordArray(boost::make_iterator_range(data, data+22));
    }

    
    // lists
    //
    {
        ::CObList rng;
        ::CObList o1, o2, o3, o4, o5;
        ::CObject *data[] = { &o1, &o2, &o3, &o4, &o5 };

        ::test_CObList(boost::make_iterator_range(data, data+5));
    }

    {
        ::CPtrList rng;
        ::CPtrList o1, o2, o3, o4, o5;
        void *data[] = { &o1, &o2, &o3, &o4, &o5 };

        ::test_CPtrList(boost::make_iterator_range(data, data+5));
    }

    {
        ::CString data[] = {
            ::CString(_T('0')), ::CString(_T('1')), ::CString(_T('2')), ::CString(_T('3')),
            ::CString(_T('4')), ::CString(_T('5')), ::CString(_T('6')), ::CString(_T('7'))
        };

        ::test_CStringList(boost::make_iterator_range(data, data+8));
    }


    // maps
    //
    {
        std::map<void *, WORD> data;
        int o0, o1, o2, o3, o4;
        data[&o0] = 15, data[&o1] = 14, data[&o2] = 3, data[&o3] = 6, data[&o4] = 1;

        ::test_CMapPtrToWord(data);
    }

    {
        std::map<void *, void*> data;
        int o0, o1, o2, o3, o4;
        data[&o0] = &o3, data[&o1] = &o2, data[&o2] = &o1, data[&o3] = &o0, data[&o4] = &o4;

        ::test_CMapPtrToPtr(data);
    }

    {
        std::map< ::CString, CObject * > data;
        CObArray o0, o1, o2, o3, o4;
        data[ ::CString('0') ] = &o0, data[ ::CString('1') ] = &o1, data[ ::CString('2') ] = &o2,
        data[ ::CString('3') ] = &o3, data[ ::CString('4') ] = &o4;

        ::test_CMapStringToOb(data);
    }

    {
        std::map< ::CString, void * > data;
        CObArray o0, o1, o2, o3, o4;
        data[ ::CString('0') ] = &o0, data[ ::CString('1') ] = &o1, data[ ::CString('2') ] = &o2,
        data[ ::CString('3') ] = &o3, data[ ::CString('4') ] = &o4;

        ::test_CMapStringToPtr(data);
    }

    {
        std::map< ::CString, ::CString > data;
        CString o0('a'), o1('b'), o2('c'), o3('d'), o4('e');
        data[ ::CString('0') ] = o0, data[ ::CString('1') ] = o1, data[ ::CString('2') ] = o2,
        data[ ::CString('3') ] = o3, data[ ::CString('4') ] = o4;

        ::test_CMapStringToString(data);
    }

    {
        std::map< WORD, CObject * > data;
        ::CDWordArray o0, o1, o2, o3, o4;
        data[21] = &o3, data[52] = &o2, data[12] = &o1, data[76] = &o0, data[54] = &o4;

        ::test_CMapWordToOb(data);
    }

    {
        std::map< WORD, void * > data;
        ::CDWordArray o0, o1, o2, o3, o4;
        data[21] = &o3, data[52] = &o2, data[12] = &o1, data[76] = &o0, data[54] = &o4;

        ::test_CMapWordToPtr(data);
    }

    // templates
    //
    {
        std::string data("0987654321qwertyuiop");
        ::test_CArray(data);
        ::test_CList(data);
    }    

    {
        std::wstring data(L"asdfghjklzxcvbnm");
        ::test_CArray(data);
        ::test_CList(data);
    }    

    {
        std::map< int, std::string > data;
        data[0] = "abcde", data[1] = "ajfie", data[2] = "lij", data[3] = "abc", data[4] = "ioiu";

        ::test_CMap(data);
    }


    // typed
    //
    {
        ::test_CTypedPtrArray();
        ::test_CTypedPtrList();
        ::test_CTypedPtrMap();
    }


    // strings
    //
#if defined(BOOST_RANGE_MFC_HAS_LEGACY_STRING)
    {
        std::string data("123456789 abcdefghijklmn");
        ::test_CString(data);
    }
#endif


} // test_mfc


#include <boost/test/unit_test.hpp>
using boost::unit_test::test_suite;


test_suite *
init_unit_test_suite(int argc, char* argv[])
{
    test_suite *test = BOOST_TEST_SUITE("MFC Range Test Suite");
    test->add(BOOST_TEST_CASE(&test_mfc));

    (void)argc, (void)argv; // unused
    return test;
}

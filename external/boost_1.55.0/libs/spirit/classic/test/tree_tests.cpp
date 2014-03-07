/*=============================================================================
    Copyright (c) 2003 Giovanni Bajo
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_ast.hpp>
#include <boost/spirit/include/classic_tree_to_xml.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/remove.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/for_each.hpp>

#include <map>
#include <string>

#include <fstream>
#include <boost/detail/lightweight_test.hpp>
#include "impl/string_length.hpp"

#define DEBUG_DUMP_TREES    (1)

//////////////////////////////////////////////////////////////////////////////
// rule_id helper
// http://sf.net/tracker/index.php?func=detail&aid=715483&group_id=28447&atid=393389)

namespace boost { namespace spirit {

    template <
        typename ScannerT, 
        unsigned long ID = 0,
        typename ContextT = parser_context<> >
    class rule_id 
        : public rule<ScannerT, ContextT, parser_tag<ID> >
    {
        typedef rule<ScannerT, ContextT, parser_tag<ID> > base_t;

    public:
        // Forward ctors and operator=.
        rule_id()
        {}

        template <typename T>
        rule_id(const T& a) 
        : base_t(a) {}

        template <typename T>
        rule_id& operator=(const T& a)
        { base_t::operator=(a); return *this; }
    };
}}


//////////////////////////////////////////////////////////////////////////////
// Framework setup

namespace mpl = boost::mpl;
using namespace BOOST_SPIRIT_CLASSIC_NS;
using namespace std;


enum RULE_ID
{
    ID_A = 1,
    ID_B,
    ID_C,
    ID_ROOT
};

map<parser_id, string> rule_names;


//////////////////////////////////////////////////////////////////////////////
// Generic tree manipulation tools

template <typename TreeT>
RULE_ID id(TreeT& t)
{ return (RULE_ID)t.value.id().to_long(); }


template <typename TreeT>
TreeT& child(TreeT& t, unsigned n)
{
    return t.children[n];
}

template <typename TreeT>
size_t num_children(const TreeT& t)
{ return t.children.size(); }

template <typename TreeT>
typename TreeT::parse_node_t::iterator_t ValueBeginIterator(TreeT& t)
{ return t.value.begin(); }

template <typename TreeT>
typename TreeT::parse_node_t::iterator_t ValueEndIterator(TreeT& t)
{ return t.value.end(); }

template <typename TreeT>
bool equal(TreeT& a, TreeT& b)
{
    if (id(a) != id(b))
        return false;

    if (num_children(a) != num_children(b))
        return false;

    unsigned n = num_children(a);
    for (unsigned i=0;i<n;i++)
        if (!equal(child(a, i), child(b, i)))
            return false;

    return true;
}

template <typename TreeT>
void dump(ostream& o, TreeT& t, int level = 0)
{
    string name;
    string value;
    map<parser_id, string>::iterator iter = 
        rule_names.find(id(t));

    if (iter == rule_names.end())
        name = "noname";
    else
        name = iter->second;

    value.assign(ValueBeginIterator(t), ValueEndIterator(t));

    for (int i=0;i<level;i++)
        o << "  ";


    o << name << ": " << value << endl;
    
    unsigned n = num_children(t);
    for (unsigned c=0;c<n;c++)
        dump(o, child(t, c), level+1);
}


//////////////////////////////////////////////////////////////////////////////
// Tree folding

namespace test_impl {

    template <typename ParmT>
    struct fold_node
    {
        // assign a subtree
        void operator()(ParmT& t, ParmT ch) const
        { t = ch; }

        // wrong specialization
        template <typename TreeT>
        void operator()(TreeT& t, ParmT p) const
        { typedef typename TreeT::this_should_never_be_compiled type; }
    };

    template <>
    struct fold_node<nil_t>
    {
        template <typename TreeT>
        void operator()(TreeT& t, nil_t) const
        { typedef typename TreeT::this_should_never_be_compiled type; }
    };

    template <>
    struct fold_node<RULE_ID>
    {
        template <typename TreeT>
        void operator()(TreeT& t, RULE_ID id) const
        { t.value.id(id); }
    };

    template <typename ParmT>
    struct fold_child
    {
        template <typename TreeT>
        void operator()(TreeT& t, ParmT p, unsigned n) const
        { fold_node<ParmT>()(t.children[n], p); }
    };

    template <>
    struct fold_child<nil_t>
    {
        template <typename TreeT>
        void operator()(TreeT& t, nil_t, unsigned n) const
        {}
    };
}

template <typename TreeT,
        typename T, typename T1, typename T2, typename T3, typename T4,
        typename T5, typename T6, typename T7, typename T8>
TreeT fold(T p, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
{
    // Prepare a list with all the template types
    typedef mpl::list<T1,T2,T3,T4,T5,T6,T7,T8> full_list_t;

    // Remove the ones equal to nil_t: they are the default parameters
    //  unspecified from the user
    typedef typename mpl::remove<full_list_t, nil_t>::type parm_list_t;

    // Get the size of the list = number of parameters specified by the user
    typedef typename mpl::size<parm_list_t>::type parm_list_size_t;
    enum { NUM_CHILDREN = parm_list_size_t::value };

    TreeT t;

    // Generate the root of the tree (specialized for the first parameter)
    test_impl::fold_node<T>()(t, p);

    // Make room for the children
    if (NUM_CHILDREN > 0)
        t.children.resize(NUM_CHILDREN);

    // For each children, call the GenerateChild function, which is specialized
    //  on the different types
    test_impl::fold_child<T1>()(t, p1, 0);
    test_impl::fold_child<T2>()(t, p2, 1);
    test_impl::fold_child<T3>()(t, p3, 2);
    test_impl::fold_child<T4>()(t, p4, 3);
    test_impl::fold_child<T5>()(t, p5, 4);
    test_impl::fold_child<T6>()(t, p6, 5);
    test_impl::fold_child<T7>()(t, p7, 6);
    test_impl::fold_child<T8>()(t, p8, 7);

    return t;
}


// Define fold() wrapper for 1->7 parameters: they just call the 8 parameter
//  version passing nil_t for the other arguments
#define PUT_EMPTY(Z, N, _)  nil_t()

#define DEFINE_FOLD(Z, N, _) \
    template <typename TreeT, typename T BOOST_PP_COMMA_IF(N) \
        BOOST_PP_ENUM_PARAMS(N, typename T) > \
    TreeT fold(T p BOOST_PP_COMMA_IF(N) BOOST_PP_ENUM_BINARY_PARAMS(N, T, p)) \
    { \
        return fold<TreeT>(p \
            BOOST_PP_COMMA_IF(N) BOOST_PP_ENUM_PARAMS(N, p) \
            BOOST_PP_COMMA_IF(BOOST_PP_SUB(8,N))  \
            BOOST_PP_ENUM(BOOST_PP_SUB(8,N), PUT_EMPTY, _)); \
    }

BOOST_PP_REPEAT(7, DEFINE_FOLD, _)

#undef PUT_EMPTY
#undef DEFINE_FOLD



//////////////////////////////////////////////////////////////////////////////
// test_banal: simple tree construction

struct test_banal : public grammar<test_banal>
{
    template <class T>
    struct definition
    {
        rule_id<T, ID_ROOT> root;
        rule_id<T, ID_A> a;
        rule_id<T, ID_B> b;
        rule_id<T, ID_C> c;

        definition(const test_banal&)
        {
            root = a >> c;
            a = b;
            b = chlit<>('b');
            c = chlit<>('c');
        }

        const rule_id<T, ID_ROOT>& start() 
        { return root; }
    };

    const char* pattern(void)
    {
        return "bc";
    }

    template <typename TreeT>
    TreeT expected_tree(void)
    {
        return fold<TreeT>(
            ID_ROOT, fold<TreeT>(
                ID_A, 
                    ID_B), 
                ID_C);
    }
};


//////////////////////////////////////////////////////////////////////////////
// All the tests

typedef mpl::list
<
    test_banal

> tests_t;


//////////////////////////////////////////////////////////////////////////////
// run_test - code to run a test

struct run_test
{
    template <typename TestT>
    void operator()(TestT gram)
    {
        typedef const char* iterator_t;
        typedef node_val_data_factory<nil_t> factory_t;
        typedef typename 
            factory_t
            ::BOOST_NESTED_TEMPLATE factory<iterator_t>
            ::node_t node_t;
        typedef tree_node<node_t> tree_t;

        iterator_t text_begin = gram.pattern();
        iterator_t text_end = text_begin + test_impl::string_length(text_begin);

        tree_parse_info<iterator_t, factory_t> info =
            ast_parse(text_begin, text_end, gram);

        BOOST_TEST(info.full);

        tree_t expected = gram.template expected_tree<tree_t>();

#if DEBUG_DUMP_TREES
        dump(cout, info.trees[0]);
        dump(cout, expected);
#endif

        BOOST_TEST(equal(info.trees[0], expected));
    }
};

//////////////////////////////////////////////////////////////////////////////
// main() stuff

#ifdef BOOST_NO_EXCEPTIONS
namespace boost
{
    void throw_exception(std::exception const & )
    {
        std::cerr << "Exception caught" << std::endl;
        BOOST_TEST(0);
    }
}

#endif


void init(void)
{
    rule_names[ID_ROOT] = "ID_ROOT";
    rule_names[ID_A] = "ID_A";
    rule_names[ID_B] = "ID_B";
    rule_names[ID_C] = "ID_C";
}


int main()
{
    init();

    mpl::for_each<tests_t, mpl::_> (run_test());

    return boost::report_errors();
}

/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2011 Daniel James
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef QUICKBOOK_SYMBOLS_IPP
#define QUICKBOOK_SYMBOLS_IPP

///////////////////////////////////////////////////////////////////////////////

#include <boost/spirit/home/classic/symbols.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/scoped_ptr.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace quickbook
{

///////////////////////////////////////////////////////////////////////////////
//
//  tst class
//
//      This it the Ternary Search Tree from
//      <boost/spirit/home/classic/symbols/impl/tst.ipp> adapted to be cheap
//      to copy.
//
//      Ternary Search Tree implementation. The data structure is faster than
//      hashing for many typical search problems especially when the search
//      interface is iterator based. Searching for a string of length k in a
//      ternary search tree with n strings will require at most O(log n+k)
//      character comparisons. TSTs are many times faster than hash tables
//      for unsuccessful searches since mismatches are discovered earlier
//      after examining only a few characters. Hash tables always examine an
//      entire key when searching.
//
//      For details see http://www.cs.princeton.edu/~rs/strings/.
//
//      *** This is a low level class and is
//          not meant for public consumption ***
//
///////////////////////////////////////////////////////////////////////////////

    template <typename T, typename CharT>
    struct tst_node
    {
        tst_node(CharT value_)
        : reference_count(0)
        , left()
        , middle()
        , right()
        , data()
        , value(value_)
        {
        }
        
        tst_node(tst_node const& other)
        : reference_count(0)
        , left(other.left)
        , middle(other.middle)
        , right(other.right)
        , data(other.data ? new T(*other.data) : 0)
        , value(other.value)
        {
        }

        // If you fancy a slight improvement in memory use,
        // reference_count + value could probably be packed
        // in the space for a single int.
        int reference_count;
        boost::intrusive_ptr<tst_node> left;
        boost::intrusive_ptr<tst_node> middle;
        boost::intrusive_ptr<tst_node> right;
        boost::scoped_ptr<T> data;
        CharT value;
    private:
        tst_node& operator=(tst_node const&);
    };

    template <typename T, typename CharT>
    void intrusive_ptr_add_ref(tst_node<T, CharT>* ptr)
        { ++ptr->reference_count; }

    template <typename T, typename CharT>
    void intrusive_ptr_release(tst_node<T, CharT>* ptr)
        { if(--ptr->reference_count == 0) delete ptr; }


    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename CharT>
    class tst
    {
        typedef tst_node<T, CharT> node_t;
        typedef boost::intrusive_ptr<node_t> node_ptr;
        node_ptr root;

    public:

        struct search_info
        {
            T*          data;
            std::size_t length;
        };

        void swap(tst& other)
        {
            root.swap(other.root);
        }

        // Adds symbol to ternary search tree.
        // If it already exists, then replace it with new value.
        //
        // pre: first != last
        template <typename IteratorT>
        T* add(IteratorT first, IteratorT const& last, T const& data)
        {
            assert (first != last);

            node_ptr* np = &root;
            CharT ch = *first;

            for(;;)
            {
                if (!*np)
                {
                    *np = new node_t(ch);
                }
                else if ((*np)->reference_count > 1)
                {
                    *np = new node_t(**np);
                }

                if (ch < (*np)->value)
                {
                    np = &(*np)->left;
                }
                else if (ch == (*np)->value)
                {
                    ++first;
                    if (first == last) break;
                    ch = *first;
                    np = &(*np)->middle;
                }
                else
                {
                    np = &(*np)->right;
                }
            }

            boost::scoped_ptr<T> new_data(new T(data));
            boost::swap((*np)->data, new_data);
            return (*np)->data.get();
        }
        
        template <typename ScannerT>
        search_info find(ScannerT const& scan) const
        {
            search_info result = { 0, 0 };
            if (scan.at_end()) {
                return result;
            }

            typedef typename ScannerT::iterator_t iterator_t;
            node_ptr    np = root;
            CharT       ch = *scan;
            iterator_t  latest = scan.first;
            std::size_t length = 0;

            while (np)
            {
                if (ch < np->value) // => go left!
                {
                    np = np->left;
                }
                else if (ch == np->value) // => go middle!
                {
                    ++scan;
                    ++length;

                    // Found a potential match.
                    if (np->data.get())
                    {
                        result.data = np->data.get();
                        result.length = length;
                        latest = scan.first;
                    }

                    if (scan.at_end()) break;
                    ch = *scan;
                    np = np->middle;
                }
                else // (ch > np->value) => go right!
                {
                    np = np->right;
                }
            }

            scan.first = latest;
            return result;
        }
    };

    typedef boost::spirit::classic::symbols<
        std::string,
        char,
        quickbook::tst<std::string, char>
    > string_symbols;
} // namespace quickbook

#endif

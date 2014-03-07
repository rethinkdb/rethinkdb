/*=============================================================================
    Copyright (c) 2004 Angus Leeming
    Copyright (c) 2004 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_STL_CONTAINER_CONTAINER_HPP
#define PHOENIX_STL_CONTAINER_CONTAINER_HPP

#include <boost/spirit/home/phoenix/stl/container/detail/container.hpp>
#include <boost/spirit/home/phoenix/function/function.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/or.hpp>
#include <boost/type_traits/is_const.hpp>

namespace boost { namespace phoenix
{
///////////////////////////////////////////////////////////////////////////////
//
//  STL container member functions
//
//      Lazy functions for STL container member functions
//
//      These functions provide a mechanism for the lazy evaluation of the
//      public member functions of the STL containers. For an overview of
//      what is meant by 'lazy evaluation', see the comments in operators.hpp
//      and functions.hpp.
//
//      Lazy functions are provided for all of the member functions of the
//      following containers:
//
//      deque - list - map - multimap - vector.
//
//      Indeed, should *your* class have member functions with the same names
//      and signatures as those listed below, then it will automatically be
//      supported. To summarize, lazy functions are provided for member
//      functions:
//
//          assign - at - back - begin - capacity - clear - empty - end -
//          erase - front - get_allocator - insert - key_comp - max_size -
//          pop_back - pop_front - push_back - push_front - rbegin - rend -
//          reserve - resize . size - splice - value_comp.
//
//      The lazy functions' names are the same as the corresponding member
//      function. Sample usage:
//
//      "Normal" version                 "Lazy" version
//      ----------------                 --------------
//      my_vector.at(5)                  phoenix::at(arg1, 5)
//      my_list.size()                   phoenix::size(arg1)
//      my_vector1.swap(my_vector2)      phoenix::swap(arg1, arg2)
//
//      Notice that member functions with names that clash with a
//      function in stl algorithms are absent. This will be provided
//      in Phoenix's algorithm module.
//
//      No support is provided here for lazy versions of operator+=,
//      operator[] etc. Such operators are not specific to STL containers and
//      lazy versions can therefore be found in operators.hpp.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  Lazy member function implementaions.
//
//      The structs below provide the guts of the implementation. Thereafter,
//      the corresponding lazy function itself is simply:
//
//          function<stl::assign> const assign = stl::assign();
//
//      The structs provide a nested "result" class template whose
//      "type" typedef enables the lazy function to ascertain the type
//      to be returned when it is invoked.
//
//      They also provide operator() member functions with signatures
//      corresponding to those of the underlying member function of
//      the STL container.
//
///////////////////////////////////////////////////////////////////////////////
    namespace stl
    {
        struct assign
        {
            template <
                typename C
              , typename Arg1 = fusion::void_
              , typename Arg2 = fusion::void_
              , typename Arg3 = fusion::void_
            >
            struct result
            {
                typedef typename add_reference<C>::type type;
            };

            template <typename C, typename Arg1>
            C& operator()(C& c, Arg1 const& arg1) const
            {
                c.assign(arg1);
                return c;
            }

            template <typename C, typename Arg1, typename Arg2>
            C& operator()(C& c, Arg1 const& arg1, Arg2 const& arg2) const
            {
                c.assign(arg1, arg2);
                return c;
            }

            template <typename C, typename Arg1, typename Arg2, typename Arg3>
            C& operator()(
                C& c
              , Arg1 const& arg1
              , Arg2 const& arg2
              , Arg3 const& arg3) const
            {
                return c.assign(arg1, arg2, arg3);
            }
        };

        struct at
        {
            template <typename C, typename Index>
            struct result
            {
                typedef typename const_qualified_reference_of<C>::type type;
            };

            template <typename C, typename Index>
            typename result<C, Index>::type
            operator()(C& c, Index const& i) const
            {
                return c.at(i);
            }
        };

        struct back
        {
            template <typename C>
            struct result
            {
                typedef 
                    typename const_qualified_reference_of<C>::type
                type;
            };

            template <typename C>
            typename result<C>::type
            operator()(C& c) const
            {
                return c.back();
            }
        };

        struct begin
        {
            template <typename C>
            struct result
            {
                typedef typename const_qualified_iterator_of<C>::type type;
            };

            template <typename C>
            typename result<C>::type
            operator()(C& c) const
            {
                return c.begin();
            }
        };

        struct capacity
        {
            template <typename C>
            struct result
            {
                typedef typename size_type_of<C>::type type;
            };

            template <typename C>
            typename result<C>::type
            operator()(C const& c) const
            {
                return c.capacity();
            }
        };

        struct clear
        {
            template <typename C>
            struct result
            {
                typedef void type;
            };

            template <typename C>
            void operator()(C& c) const
            {
                return c.clear();
            }
        };

        struct empty
        {
            template <typename C>
            struct result
            {
                typedef bool type;
            };

            template <typename C>
            bool operator()(C const& c) const
            {
                return c.empty();
            }
        };

        struct end
        {
            template <typename C>
            struct result
            {
                typedef typename const_qualified_iterator_of<C>::type type;
            };

            template <typename C>
            typename result<C>::type
            operator()(C& c) const
            {
                return c.end();
            }
        };

        struct erase
        {
            //  This mouthful can differentiate between the generic erase
            //  functions (Container == std::deque, std::list, std::vector) and
            //  that specific to the two map-types, std::map and std::multimap.
            //
            //  where C is a std::deque, std::list, std::vector:
            //
            //      1) iterator C::erase(iterator where);
            //      2) iterator C::erase(iterator first, iterator last);
            //
            //  where M is a std::map or std::multimap:
            //
            //      3) size_type M::erase(const Key& keyval);
            //      4) void M::erase(iterator where);
            //      5) void M::erase(iterator first, iterator last);

            template <typename C, typename Arg1, typename Arg2 = fusion::void_>
            struct result
            {
                //  BOOST_MSVC #if branch here in map_erase_result non-
                //  standard behavior. The return type should be void but
                //  VC7.1 prefers to return iterator_of<C>. As a result,
                //  VC7.1 complains of error C2562:
                //  boost::phoenix::stl::erase::operator() 'void' function
                //  returning a value. Oh well... :*

                typedef
                    boost::mpl::eval_if<
                        boost::is_same<Arg1, typename iterator_of<C>::type>
#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1500)
                      , iterator_of<C>
#else
                      , boost::mpl::identity<void>
#endif
                      , size_type_of<C>
                    >
                map_erase_result;

                typedef typename
                    boost::mpl::eval_if<
                        has_mapped_type<C>
                      , map_erase_result
                      , iterator_of<C>
                    >::type
                type;
            };

            template <typename C, typename Arg1>
            typename result<C, Arg1>::type
            operator()(C& c, Arg1 const& arg1) const
            {
                return c.erase(arg1);
            }

            template <typename C, typename Arg1, typename Arg2>
            typename result<C, Arg1, Arg2>::type
            operator()(C& c, Arg1 const& arg1, Arg2 const& arg2) const
            {
                return c.erase(arg1, arg2);
            }
        };

        struct front
        {
            template <typename C>
            struct result
            {
                typedef typename const_qualified_reference_of<C>::type type;
            };

            template <typename C>
            typename result<C>::type
            operator()(C& c) const
            {
                return c.front();
            }
        };

        struct get_allocator
        {
            template <typename C>
            struct result
            {
                typedef typename allocator_type_of<C>::type type;
            };

            template <typename C>
            typename result<C>::type
            operator()(C const& c) const
            {
                return c.get_allocator();
            }
        };

        struct insert
        {
            //  This mouthful can differentiate between the generic insert
            //  functions (Container == deque, list, vector) and those
            //  specific to the two map-types, std::map and std::multimap.
            //
            //  where C is a std::deque, std::list, std::vector:
            //
            //      1) iterator C::insert(iterator where, value_type value);
            //      2) void C::insert(
            //          iterator where, size_type count, value_type value);
            //      3) template <typename Iter>
            //         void C::insert(iterator where, Iter first, Iter last);
            //
            //  where M is a std::map and MM is a std::multimap:
            //
            //      4) pair<iterator, bool> M::insert(value_type const&);
            //      5) iterator MM::insert(value_type const&);
            //
            //  where M is a std::map or std::multimap:
            //
            //      6) template <typename Iter>
            //         void M::insert(Iter first, Iter last);

            template <
                typename C
              , typename Arg1
              , typename Arg2 = fusion::void_
              , typename Arg3 = fusion::void_
            >
            class result
            {
                struct pair_iterator_bool
                {
                    typedef typename std::pair<typename C::iterator, bool> type;
                };

                typedef
                    boost::mpl::eval_if<
                        map_insert_returns_pair<C>
                      , pair_iterator_bool
                      , iterator_of<C>
                    >
                choice_1;

                typedef
                    boost::mpl::eval_if<
                        boost::mpl::and_<
                            boost::is_same<Arg3, fusion::void_>
                          , boost::mpl::not_<boost::is_same<Arg1, Arg2> > >
                      , iterator_of<C>
                      , boost::mpl::identity<void>
                    >
                choice_2;

            public:

                typedef typename
                    boost::mpl::eval_if<
                        boost::is_same<Arg2, fusion::void_>
                      , choice_1
                      , choice_2
                    >::type
                type;
            };

            template <typename C, typename Arg1>
            typename result<C, Arg1>::type
            operator()(C& c, Arg1 const& arg1) const
            {
                return c.insert(arg1);
            }

            template <typename C, typename Arg1, typename Arg2>
            typename result<C, Arg1, Arg2>::type
            operator()(C& c, Arg1 const& arg1, Arg2 const& arg2) const
            {
                return c.insert(arg1, arg2);
            }

            template <typename C, typename Arg1, typename Arg2, typename Arg3>
            typename result<C, Arg1, Arg2, Arg3>::type
            operator()(
                C& c, Arg1 const& arg1, Arg2 const& arg2, Arg3 const& arg3) const
            {
                return c.insert(arg1, arg2, arg3);
            }
        };

        struct key_comp
        {
            template <typename C>
            struct result
            {
                typedef typename key_compare_of<C>::type type;
            };

            template <typename C>
            typename result<C>::type
            operator()(C const& c) const
            {
                return c.key_comp();
            }
        };

        struct max_size
        {
            template <typename C>
            struct result
            {
                typedef typename size_type_of<C>::type type;
            };

            template <typename C>
            typename result<C>::type
            operator()(C const& c) const
            {
                return c.max_size();
            }
        };

        struct pop_back
        {
            template <typename C>
            struct result
            {
                typedef void type;
            };

            template <typename C>
            void operator()(C& c) const
            {
                return c.pop_back();
            }
        };

        struct pop_front
        {
            template <typename C>
            struct result
            {
                typedef void type;
            };

            template <typename C>
            void operator()(C& c) const
            {
                return c.pop_front();
            }
        };

        struct push_back
        {
            template <typename C, typename Arg>
            struct result
            {
                typedef void type;
            };

            template <typename C, typename Arg>
            void operator()(C& c, Arg const& data) const
            {
                return c.push_back(data);
            }
        };

        struct push_front
        {
            template <typename C, typename Arg>
            struct result
            {
                typedef void type;
            };

            template <typename C, typename Arg>
            void operator()(C& c, Arg const& data) const
            {
                return c.push_front(data);
            }
        };

        struct rbegin
        {
            template <typename C>
            struct result
            {
                typedef typename
                    const_qualified_reverse_iterator_of<C>::type
                type;
            };

            template <typename C>
            typename result<C>::type
            operator()(C& c) const
            {
                return c.rbegin();
            }
        };

        struct rend
        {
            template <typename C>
            struct result
            {
                typedef typename
                    const_qualified_reverse_iterator_of<C>::type
                type;
            };

            template <typename C>
            typename result<C>::type
            operator()(C& c) const
            {
                return c.rend();
            }
        };

        struct reserve
        {

            template <typename C, typename Arg>
            struct result
            {
                typedef void type;
            };

            template <typename C, typename Arg>
            void operator()(C& c, Arg const& count) const
            {
                return c.reserve(count);
            }
        };

        struct resize
        {
            template <typename C, typename Arg1, typename Arg2 = fusion::void_>
            struct result
            {
                typedef void type;
            };

            template <typename C, typename Arg1>
            void operator()(C& c, Arg1 const& arg1) const
            {
                return c.resize(arg1);
            }

            template <typename C, typename Arg1, typename Arg2>
            void operator()(C& c, Arg1 const& arg1, Arg2 const& arg2) const
            {
                return c.resize(arg1, arg2);
            }
        };

        struct size
        {
            template <typename C>
            struct result
            {
                typedef typename size_type_of<C>::type type;
            };

            template <typename C>
            typename result<C>::type
            operator()(C const& c) const
            {
                return c.size();
            }
        };

    struct splice
    {
        template <
            typename C
          , typename Arg1
          , typename Arg2
          , typename Arg3 = fusion::void_
          , typename Arg4 = fusion::void_
        >
        struct result
        {
            typedef void type;
        };

        template <typename C, typename Arg1, typename Arg2>
        void operator()(C& c, Arg1 const& arg1, Arg2& arg2) const
        {
            c.splice(arg1, arg2);
        }

        template <
            typename C
          , typename Arg1
          , typename Arg2
          , typename Arg3
        >
        void operator()(
            C& c
          , Arg1 const& arg1
          , Arg2& arg2
          , Arg3 const& arg3
        ) const
        {
            c.splice(arg1, arg2, arg3);
        }

        template <
            typename C
          , typename Arg1
          , typename Arg2
          , typename Arg3
          , typename Arg4
        >
        void operator()(
            C& c
          , Arg1 const& arg1
          , Arg2& arg2
          , Arg3 const& arg3
          , Arg4 const& arg4
        ) const
        {
            c.splice(arg1, arg2, arg3, arg4);
        }
    };

    struct value_comp
    {
        template <typename C>
        struct result
        {
            typedef typename value_compare_of<C>::type type;
        };

        template <typename C>
        typename result<C>::type
        operator()(C const& c) const
        {
            return c.value_comp();
        }
    };

} // namespace stl

///////////////////////////////////////////////////////////////////////////////
//
//  The lazy functions themselves.
//
///////////////////////////////////////////////////////////////////////////////
function<stl::assign> const         assign = stl::assign();
function<stl::at> const             at = stl::at();
function<stl::back> const           back = stl::back();
function<stl::begin> const          begin = stl::begin();
function<stl::capacity> const       capacity = stl::capacity();
function<stl::clear> const          clear = stl::clear();
function<stl::empty> const          empty = stl::empty();
function<stl::end> const            end = stl::end();
function<stl::erase> const          erase = stl::erase();
function<stl::front> const          front = stl::front();
function<stl::get_allocator> const  get_allocator = stl::get_allocator();
function<stl::insert> const         insert = stl::insert();
function<stl::key_comp> const       key_comp = stl::key_comp();
function<stl::max_size> const       max_size = stl::max_size();
function<stl::pop_back> const       pop_back  = stl::pop_back();
function<stl::pop_front> const      pop_front = stl::pop_front();
function<stl::push_back> const      push_back  = stl::push_back();
function<stl::push_front> const     push_front = stl::push_front();
function<stl::rbegin> const         rbegin = stl::rbegin();
function<stl::rend> const           rend = stl::rend();
function<stl::reserve> const        reserve = stl::reserve();
function<stl::resize> const         resize = stl::resize();
function<stl::size> const           size = stl::size();
function<stl::splice> const         splice = stl::splice();
function<stl::value_comp> const     value_comp = stl::value_comp();

}} // namespace boost::phoenix

#endif // PHOENIX_STL_CONTAINERS_HPP

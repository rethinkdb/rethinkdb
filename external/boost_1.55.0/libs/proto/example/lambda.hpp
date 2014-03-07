#ifndef BOOST_PP_IS_ITERATING
    ///////////////////////////////////////////////////////////////////////////////
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
    //
    // This example contains a full-featured reimplementation of the old,
    // now-deprecated Boost Lambda Library (BLL) on top of Boost.Proto. It
    // is necessarily complex to accomodate all the quirks and inconsistencies
    // of that old library, but it is a good example of how to build a
    // complete and full-featured EDLS using Proto.
    #ifndef BOOST_LAMBDA_HPP_EAN_04_19_2008
    #define BOOST_LAMBDA_HPP_EAN_04_19_2008

    #include <iosfwd>
    #include <typeinfo>
    #include <algorithm>
    #include <boost/ref.hpp>
    #include <boost/assert.hpp>
    #include <boost/mpl/or.hpp>
    #include <boost/mpl/int.hpp>
    #include <boost/mpl/void.hpp>
    #include <boost/mpl/identity.hpp>
    #include <boost/mpl/next_prior.hpp>
    #include <boost/mpl/min_max.hpp>
    #include <boost/mpl/assert.hpp>
    #include <boost/preprocessor.hpp>
    #include <boost/utility/enable_if.hpp>
    #include <boost/utility/result_of.hpp>
    #include <boost/fusion/include/vector.hpp>
    #include <boost/type_traits/add_reference.hpp>
    #include <boost/type_traits/remove_reference.hpp>
    #include <boost/type_traits/remove_const.hpp>
    #include <boost/type_traits/is_same.hpp>
    #include <boost/proto/proto.hpp>

    #ifndef BOOST_LAMBDA_MAX_ARITY
    # define BOOST_LAMBDA_MAX_ARITY 3
    #endif

    #ifdef _MSC_VER
    # pragma warning(push)
    # pragma warning(disable: 4355) // 'this' : used in base member initializer list
    # pragma warning(disable: 4065) // switch statement contains 'default' but no 'case' labels
    #endif

    namespace boost { namespace lambda
    {
        namespace tag
        {
            struct if_ {};
            struct if_else_ {};
            struct for_ {};
            struct while_ {};
            struct do_while_ {};
            struct protect {};
            struct try_ {};
            struct throw_ {};
            struct rethrow_ {};
            struct switch_ {};
            struct default_ {};
            template<int I> struct case_ { static const int value = I; };
            template<typename E> struct catch_ { typedef E exception_type; };
            struct catch_all_ { typedef catch_all_ exception_type; };
        };

        template<typename Int>
        struct placeholder
        {
            typedef typename Int::tag tag;
            typedef typename Int::value_type value_type;
            typedef placeholder<Int> type;
            typedef placeholder<typename Int::next> next;
            typedef placeholder<typename Int::prior> prior;
            static const value_type value = Int::value;

            friend std::ostream &operator<<(std::ostream &sout, placeholder)
            {
                return sout << "boost::lambda::_" << (Int::value+1);
            }
        };

        struct exception_placeholder
        {};

        struct no_exception_type {};
        no_exception_type const no_exception = {};

        // Calculate the arity of a lambda expression
        struct Arity
          : proto::or_<
                proto::when<proto::terminal<placeholder<proto::_> >, mpl::next<proto::_value>()>
              , proto::when<proto::terminal<proto::_>, mpl::int_<0>()>
              , proto::otherwise<proto::fold<proto::_, mpl::int_<0>(), mpl::max<proto::_state, Arity>()> >
            >
        {};

        // True when a lambda expression can be applied with no arguments and
        // without an active exception object
        struct IsNullary
          : proto::or_<
                proto::when<proto::terminal<placeholder<proto::_> >, mpl::false_()>
              , proto::when<proto::terminal<exception_placeholder>, mpl::false_()>
              , proto::when<proto::terminal<proto::_>, mpl::true_()>
              , proto::otherwise<proto::fold<proto::_, mpl::true_(), mpl::and_<proto::_state, IsNullary>()> >
            >
        {};

        struct Eval;

        struct EvalWhile : proto::transform<EvalWhile>
        {
            template<typename Expr, typename State, typename Data>
            struct impl : proto::transform_impl<Expr, State, Data>
            {
                typedef mpl::void_ result_type;

                result_type operator()(
                    typename impl::expr_param expr
                  , typename impl::state_param state
                  , typename impl::data_param data
                ) const
                {
                    while(Eval()(proto::left(expr), state, data))
                    {
                        Eval()(proto::right(expr), state, data);
                    }
                    return result_type();
                }
            };
        };

        struct EvalDoWhile : proto::transform<EvalDoWhile>
        {
            template<typename Expr, typename State, typename Data>
            struct impl : proto::transform_impl<Expr, State, Data>
            {
                typedef mpl::void_ result_type;

                result_type operator()(
                    typename impl::expr_param expr
                  , typename impl::state_param state
                  , typename impl::data_param data
                ) const
                {
                    do
                    {
                        Eval()(proto::child_c<0>(expr), state, data);
                    }
                    while(Eval()(proto::child_c<1>(expr), state, data));

                    return result_type();
                }
            };
        };

        struct EvalFor : proto::transform<EvalFor>
        {
            template<typename Expr, typename State, typename Data>
            struct impl : proto::transform_impl<Expr, State, Data>
            {
                typedef mpl::void_ result_type;

                result_type operator()(
                    typename impl::expr_param expr
                  , typename impl::state_param state
                  , typename impl::data_param data
                ) const
                {
                    for(Eval()(proto::child_c<0>(expr), state, data)
                      ; Eval()(proto::child_c<1>(expr), state, data)
                      ; Eval()(proto::child_c<2>(expr), state, data))
                    {
                        Eval()(proto::child_c<3>(expr), state, data);
                    }
                    return result_type();
                }
            };
        };

        struct EvalIf : proto::transform<EvalIf>
        {
            template<typename Expr, typename State, typename Data>
            struct impl : proto::transform_impl<Expr, State, Data>
            {
                typedef mpl::void_ result_type;

                result_type operator()(
                    typename impl::expr_param expr
                  , typename impl::state_param state
                  , typename impl::data_param data
                ) const
                {
                    if(Eval()(proto::left(expr), state, data))
                    {
                        Eval()(proto::right(expr), state, data);
                    }
                    return result_type();
                }
            };
        };

        struct EvalIfElse : proto::transform<EvalIfElse>
        {
            template<typename Expr, typename State, typename Data>
            struct impl : proto::transform_impl<Expr, State, Data>
            {
                typedef mpl::void_ result_type;

                result_type operator()(
                    typename impl::expr_param expr
                  , typename impl::state_param state
                  , typename impl::data_param data
                ) const
                {
                    if(Eval()(proto::child_c<0>(expr), state, data))
                    {
                        Eval()(proto::child_c<1>(expr), state, data);
                    }
                    else
                    {
                        Eval()(proto::child_c<2>(expr), state, data);
                    }
                    return result_type();
                }
            };
        };

        struct EvalException : proto::transform<EvalException>
        {
            template<typename Expr, typename State, typename Data>
            struct impl : proto::transform_impl<Expr, State, Data>
            {
                typedef typename remove_const<typename impl::state>::type result_type;
                BOOST_MPL_ASSERT_NOT((is_same<result_type, no_exception_type>));
                BOOST_MPL_ASSERT_NOT((is_same<result_type, tag::catch_all_>));

                typename impl::state_param operator()(
                    typename impl::expr_param
                  , typename impl::state_param state
                  , typename impl::data_param
                ) const
                {
                    return state;
                }
            };
        };

        struct EvalSwitch : proto::transform<EvalSwitch>
        {
            template<typename Expr, typename State, typename Data, long Arity, typename BackTag>
            struct impl2;

            #define M0(Z, N, DATA)                                                                  \
                case proto::tag_of<typename proto::result_of::child_c<Expr, N>::type>::type::value: \
                    Eval()(proto::child_c<N>(expr), state, data);                                   \
                    break;                                                                          \
                    /**/

            #define M1(Z, N, DATA)                                                                  \
            template<typename Expr, typename State, typename Data, typename BackTag>                \
            struct impl2<Expr, State, Data, N, BackTag>                                             \
              : proto::transform_impl<Expr, State, Data>                                            \
            {                                                                                       \
                typedef void result_type;                                                           \
                                                                                                    \
                void operator()(                                                                    \
                    typename impl2::expr_param expr                                                 \
                  , typename impl2::state_param state                                               \
                  , typename impl2::data_param data                                                 \
                ) const                                                                             \
                {                                                                                   \
                    switch(Eval()(proto::child_c<0>(expr), state, data))                            \
                    {                                                                               \
                        BOOST_PP_REPEAT_FROM_TO_ ## Z(1, N, M0, ~)                                  \
                    default:                                                                        \
                        break;                                                                      \
                    }                                                                               \
                }                                                                                   \
            };                                                                                      \
                                                                                                    \
            template<typename Expr, typename State, typename Data>                                  \
            struct impl2<Expr, State, Data, N, tag::default_>                                       \
              : proto::transform_impl<Expr, State, Data>                                            \
            {                                                                                       \
                typedef void result_type;                                                           \
                                                                                                    \
                void operator()(                                                                    \
                    typename impl2::expr_param expr                                                 \
                  , typename impl2::state_param state                                               \
                  , typename impl2::data_param data                                                 \
                ) const                                                                             \
                {                                                                                   \
                    switch(Eval()(proto::child_c<0>(expr), state, data))                            \
                    {                                                                               \
                        BOOST_PP_REPEAT_FROM_TO_ ## Z(1, BOOST_PP_DEC(N), M0, ~)                    \
                    default:;                                                                       \
                        Eval()(proto::child_c<BOOST_PP_DEC(N)>(expr), state, data);                 \
                        break;                                                                      \
                    }                                                                               \
                }                                                                                   \
            };                                                                                      \
            /**/
            BOOST_PP_REPEAT_FROM_TO(2, BOOST_PP_INC(BOOST_PROTO_MAX_ARITY), M1, ~)
            #undef M0
            #undef M1

            template<typename Expr, typename State, typename Data>
            struct impl
              : impl2<
                    Expr
                  , State
                  , Data
                  , proto::arity_of<Expr>::value
                  , typename proto::tag_of<
                        typename proto::result_of::child_c<
                            Expr
                          , proto::arity_of<Expr>::value-1
                        >::type
                    >::type
                >
            {};
        };

        struct throw_fun
        {
            BOOST_PROTO_CALLABLE()
            typedef void result_type;
            template<typename Expr>
            void operator()(Expr const &e) const
            {
                throw e;
            }
        };

        struct unwrap_ref : proto::callable
        {
            template<typename Sig>
            struct result;

            template<typename This, typename T>
            struct result<This(reference_wrapper<T>)>
            {
                typedef T &type;
            };

            template<typename This, typename T>
            struct result<This(T &)>
              : result<This(T)>
            {};

            template<typename T>
            T &operator()(reference_wrapper<T> const &ref) const
            {
                return ref;
            }
        };

        struct anytype
        {
            template<typename T>
            anytype(T &) { BOOST_ASSERT(false); }
            template<typename T>
            operator T &() const { BOOST_ASSERT(false); throw; }
        private:
            anytype();
        };

        struct rethrow_fun
        {
            BOOST_PROTO_CALLABLE()
            typedef anytype result_type;
            template<typename State>
            anytype operator()(State const &) const
            {
                BOOST_MPL_ASSERT_NOT((is_same<State, no_exception_type>));
                throw;
            }
        };

        struct Cases
        {
            template<typename Tag>
            struct case_
              : proto::otherwise<proto::_default<Eval> >
            {};

            template<typename E>
            struct case_<tag::catch_<E> >
              : proto::otherwise<Eval(proto::_child)>
            {};

            template<int I>
            struct case_<tag::case_<I> >
              : proto::otherwise<Eval(proto::_child)>
            {};
        };

        template<> struct Cases::case_<tag::while_>     : proto::otherwise<EvalWhile> {};
        template<> struct Cases::case_<tag::for_>       : proto::otherwise<EvalFor> {};
        template<> struct Cases::case_<tag::if_>        : proto::otherwise<EvalIf> {};
        template<> struct Cases::case_<tag::if_else_>   : proto::otherwise<EvalIfElse> {};
        template<> struct Cases::case_<tag::do_while_>  : proto::otherwise<EvalDoWhile> {};
        template<> struct Cases::case_<tag::switch_>    : proto::otherwise<EvalSwitch> {};
        template<> struct Cases::case_<tag::protect>    : proto::otherwise<proto::_child> {};
        template<> struct Cases::case_<tag::default_>   : proto::otherwise<Eval(proto::_child)> {};
        template<> struct Cases::case_<tag::catch_all_> : proto::otherwise<Eval(proto::_child)> {};

        template<>
        struct Cases::case_<proto::tag::terminal>
          : proto::or_<
                proto::when<
                    proto::terminal<placeholder<proto::_> >
                  , proto::functional::at(proto::_data, proto::_value)
                >
              , proto::when<
                    proto::terminal<exception_placeholder>
                  , EvalException
                >
              , proto::when<
                    proto::terminal<reference_wrapper<proto::_> >
                  , unwrap_ref(proto::_value)
                >
              , proto::otherwise<proto::_default<Eval> >
            >
        {};

        template<>
        struct Cases::case_<proto::tag::function>
          : proto::or_<
                proto::when<
                    proto::function<proto::terminal<rethrow_fun> >
                  , rethrow_fun(proto::_state)
                >
              , proto::otherwise<proto::_default<Eval> >
            >
        {};

        struct Eval
          : proto::switch_<Cases>
        {};

        // Use a grammar to disable Proto's assignment operator overloads.
        // We'll define our own because we want (x+=_1) to store x by
        // reference. (In all other cases, variables are stored by value
        // within lambda expressions.)
        struct Grammar
          : proto::switch_<struct AssignOps>
        {};

        struct AssignOps
        {
            template<typename Tag> struct case_ : proto::_ {};
        };

        template<> struct AssignOps::case_<proto::tag::shift_left_assign>   : proto::not_<proto::_> {};
        template<> struct AssignOps::case_<proto::tag::shift_right_assign>  : proto::not_<proto::_> {};
        template<> struct AssignOps::case_<proto::tag::multiplies_assign>   : proto::not_<proto::_> {};
        template<> struct AssignOps::case_<proto::tag::divides_assign>      : proto::not_<proto::_> {};
        template<> struct AssignOps::case_<proto::tag::modulus_assign>      : proto::not_<proto::_> {};
        template<> struct AssignOps::case_<proto::tag::plus_assign>         : proto::not_<proto::_> {};
        template<> struct AssignOps::case_<proto::tag::minus_assign>        : proto::not_<proto::_> {};
        template<> struct AssignOps::case_<proto::tag::bitwise_and_assign>  : proto::not_<proto::_> {};
        template<> struct AssignOps::case_<proto::tag::bitwise_or_assign>   : proto::not_<proto::_> {};
        template<> struct AssignOps::case_<proto::tag::bitwise_xor_assign>  : proto::not_<proto::_> {};

        namespace exprns_
        {
            template<typename Expr>
            struct llexpr;
        }

        using exprns_::llexpr;

        // Wrap expressions in lambda::llexpr<>.
        struct Generator
          : proto::pod_generator<llexpr>
        {};

        // The domain for the lambda library.
        struct lldomain
          : proto::domain<Generator, Grammar, proto::default_domain>
        {
            // Make all terminals and children held by value instead of by reference.
            // Proto::domain<>::as_expr<> holds everything it can by value; the only
            // exceptions are function types, abstract types, and iostreams.
            template<typename T>
            struct as_child
              : proto_base_domain::as_expr<T>
            {};

            // The exception is arrays, which should still be held by reference
            template<typename T, std::size_t N>
            struct as_child<T[N]>
              : proto_base_domain::as_child<T[N]>
            {};
        };

        template<typename Sig>
        struct llresult;

        template<typename This>
        struct llresult<This()>
          : mpl::if_c<
                result_of<IsNullary(This &)>::type::value
              , result_of<Eval(This &, no_exception_type const &, fusion::vector0 &)>
              , mpl::identity<void>
            >::type
        {};

        #define M0(Z, N, DATA)                                                                      \
        template<typename This BOOST_PP_ENUM_TRAILING_PARAMS_Z(Z, N, typename A)>                   \
        struct llresult<This(BOOST_PP_ENUM_PARAMS_Z(Z, N, A))>                                      \
          : result_of<                                                                              \
                Eval(                                                                               \
                    This &                                                                          \
                  , no_exception_type const &                                                       \
                  , BOOST_PP_CAT(fusion::vector, N)<BOOST_PP_ENUM_PARAMS_Z(Z, N, A)> &              \
                )                                                                                   \
            >                                                                                       \
        {};                                                                                         \
        /**/
        BOOST_PP_REPEAT_FROM_TO(1, BOOST_PP_INC(BOOST_LAMBDA_MAX_ARITY), M0, ~)
        #undef M0

        template<typename Expr>
        struct llexpr
        {
            BOOST_PROTO_BASIC_EXTENDS(Expr, llexpr<Expr>, lldomain)
            BOOST_PROTO_EXTENDS_ASSIGN()
            BOOST_PROTO_EXTENDS_SUBSCRIPT()

            template<typename Sig>
            struct result
              : llresult<Sig>
            {};

            typename result<llexpr const()>::type
            operator()() const
            {
                fusion::vector0 args;
                return Eval()(*this, no_exception, args);
            }

            #define M1(Z, N, _) ((0)(1))

            #define M2(R, PRODUCT) M3(R, BOOST_PP_SEQ_SIZE(PRODUCT), PRODUCT)

            #define M3(R, SIZE, PRODUCT)                                                                    \
                template<BOOST_PP_ENUM_PARAMS(SIZE, typename A)>                                            \
                typename result<llexpr const(BOOST_PP_SEQ_FOR_EACH_I_R(R, M5, ~, PRODUCT))>::type           \
                operator ()(BOOST_PP_SEQ_FOR_EACH_I_R(R, M4, ~, PRODUCT)) const                             \
                {                                                                                           \
                    BOOST_MPL_ASSERT_RELATION(result_of<Arity(Expr const &)>::type::value, <=, SIZE);       \
                    BOOST_PP_CAT(fusion::vector, SIZE)<BOOST_PP_SEQ_FOR_EACH_I_R(R, M5, ~, PRODUCT)> args   \
                        (BOOST_PP_SEQ_FOR_EACH_I_R(R, M6, ~, PRODUCT));                                     \
                    return Eval()(*this, no_exception, args);                                               \
                }                                                                                           \
                /**/

            #define M4(R, _, I, ELEM)                                                                       \
                BOOST_PP_COMMA_IF(I) BOOST_PP_CAT(A, I) BOOST_PP_CAT(C, ELEM) &BOOST_PP_CAT(a, I)           \
                /**/

            #define M5(R, _, I, ELEM)                                                                       \
                BOOST_PP_COMMA_IF(I) BOOST_PP_CAT(A, I) BOOST_PP_CAT(C, ELEM)&                              \
                /**/

            #define M6(R, _, I, ELEM)                                                                       \
                BOOST_PP_COMMA_IF(I) BOOST_PP_CAT(a, I)                                                     \
                /**/

            #define C0

            #define C1 const

            #define BOOST_PP_ITERATION_PARAMS_1 (3, (1, BOOST_LAMBDA_MAX_ARITY, "./lambda.hpp"))
            #include BOOST_PP_ITERATE()

            #undef C0
            #undef C1
            #undef M1
            #undef M2
            #undef M3
            #undef M4
            #undef M5
            #undef M6
        };

        typedef llexpr<proto::terminal<placeholder<mpl::int_<0> > >::type> placeholder1_type;
        typedef llexpr<proto::terminal<placeholder<mpl::int_<1> > >::type> placeholder2_type;
        typedef llexpr<proto::terminal<placeholder<mpl::int_<2> > >::type> placeholder3_type;

        placeholder1_type const _1 = {{{}}};
        placeholder2_type const _2 = {{{}}};
        placeholder3_type const _3 = {{{}}};

        placeholder1_type const free1 = {{{}}};
        placeholder2_type const free2 = {{{}}};
        placeholder3_type const free3 = {{{}}};

        typedef llexpr<proto::terminal<exception_placeholder>::type> placeholderE_type;
        placeholderE_type const _e = {{{}}};

        struct byref
        {
            template<typename Sig>
            struct result;

            template<typename This, typename T>
            struct result<This(T &)>
            {
                typedef llexpr<typename proto::terminal<T &>::type> type;
            };

            template<typename This, typename T>
            struct result<This(llexpr<T> &)>
            {
                typedef boost::reference_wrapper<llexpr<T> > type;
            };

            template<typename This, typename T>
            struct result<This(llexpr<T> const &)>
            {
                typedef boost::reference_wrapper<llexpr<T> const> type;
            };

            template<typename T>
            typename result<byref(T &)>::type operator()(T &t) const
            {
                typename result<byref(T &)>::type that = {{t}};
                return that;
            }

            template<typename T>
            typename result<byref(T const &)>::type operator()(T const &t) const
            {
                typename result<byref(T const &)>::type that = {{t}};
                return that;
            }

            template<typename T>
            boost::reference_wrapper<llexpr<T> > operator()(llexpr<T> &t) const
            {
                return boost::ref(t);
            }

            template<typename T>
            boost::reference_wrapper<llexpr<T> const> operator()(llexpr<T> const &t) const
            {
                return boost::ref(t);
            }
        };

        namespace exprns_
        {
            // Ugh, the assign operators (and only the assign operators) store
            // their left terminals by reference. That requires this special handling.
            #define BOOST_LAMBDA_DEFINE_ASSIGN_OP(OP, TAG)                                          \
            template<typename T, typename U>                                                        \
            typename proto::result_of::make_expr<                                                   \
                TAG                                                                                 \
              , lldomain                                                                            \
              , typename boost::result_of<byref(T &)>::type                                         \
              , U &                                                                                 \
            >::type const                                                                           \
            operator OP(T &t, U &u)                                                                 \
            {                                                                                       \
                return proto::make_expr<TAG, lldomain>(byref()(t), boost::ref(u));                  \
            }                                                                                       \
            template<typename T, typename U>                                                        \
            typename proto::result_of::make_expr<                                                   \
                TAG                                                                                 \
              , lldomain                                                                            \
              , typename boost::result_of<byref(T &)>::type                                         \
              , U const &                                                                           \
            >::type const                                                                           \
            operator OP(T &t, U const &u)                                                           \
            {                                                                                       \
                return proto::make_expr<TAG, lldomain>(byref()(t), boost::ref(u));                  \
            }                                                                                       \
            /**/

            BOOST_LAMBDA_DEFINE_ASSIGN_OP(<<=, boost::proto::tag::shift_left_assign)
            BOOST_LAMBDA_DEFINE_ASSIGN_OP(>>=, boost::proto::tag::shift_right_assign)
            BOOST_LAMBDA_DEFINE_ASSIGN_OP(*= , boost::proto::tag::multiplies_assign)
            BOOST_LAMBDA_DEFINE_ASSIGN_OP(/= , boost::proto::tag::divides_assign)
            BOOST_LAMBDA_DEFINE_ASSIGN_OP(%= , boost::proto::tag::modulus_assign)
            BOOST_LAMBDA_DEFINE_ASSIGN_OP(+= , boost::proto::tag::plus_assign)
            BOOST_LAMBDA_DEFINE_ASSIGN_OP(-= , boost::proto::tag::minus_assign)
            BOOST_LAMBDA_DEFINE_ASSIGN_OP(&= , boost::proto::tag::bitwise_and_assign)
            BOOST_LAMBDA_DEFINE_ASSIGN_OP(|= , boost::proto::tag::bitwise_or_assign)
            BOOST_LAMBDA_DEFINE_ASSIGN_OP(^= , boost::proto::tag::bitwise_xor_assign)
        }

        template<typename T>
        struct var_type
        {
            typedef llexpr<typename proto::terminal<T &>::type> type;
        };

        template<typename T>
        llexpr<typename proto::terminal<T &>::type> const
        var(T &t)
        {
            llexpr<typename proto::terminal<T &>::type> that = {{t}};
            return that;
        }

        template<typename T>
        struct constant_type
          : proto::result_of::make_expr<
                proto::tag::terminal
              , lldomain
              , T const &
            >
        {};

        template<typename T>
        typename constant_type<T>::type const
        constant(T const &t)
        {
            typename constant_type<T>::type that = {{t}};
            return that;
        }

        template<typename T>
        struct constant_ref_type
        {
            typedef llexpr<typename proto::terminal<T const &>::type> type;
        };

        template<typename T>
        llexpr<typename proto::terminal<T const &>::type> const
        constant_ref(T const &t)
        {
            llexpr<typename proto::terminal<T const &>::type> that = {{t}};
            return that;
        }

        template<typename Cond>
        struct while_generator
        {
            explicit while_generator(Cond const &c)
              : cond(c)
            {}

            template<typename Body>
            typename proto::result_of::make_expr<
                tag::while_
              , lldomain
              , Cond const &
              , Body const &
            >::type const
            operator[](Body const &body) const
            {
                return proto::make_expr<tag::while_, lldomain>(
                    boost::ref(this->cond)
                  , boost::ref(body)
                );
            }

        private:
            Cond const &cond;
        };

        template<typename Expr>
        while_generator<Expr> while_(Expr const &expr)
        {
            return while_generator<Expr>(expr);
        }

        template<typename Expr>
        struct else_generator
        {
            typedef typename proto::result_of::left<Expr const &>::type condition_type;
            typedef typename proto::result_of::right<Expr const &>::type body1_type;

            explicit else_generator(Expr const &expr)
              : if_(expr)
            {}

            template<typename Body2>
            typename proto::result_of::make_expr<
                tag::if_else_
              , lldomain
              , condition_type
              , body1_type
              , Body2 const &
            >::type const
            operator[](Body2 const &body2) const
            {
                return proto::make_expr<tag::if_else_, lldomain>(
                    boost::ref(proto::left(this->if_))
                  , boost::ref(proto::right(this->if_))
                  , boost::ref(body2)
                );
            }

        private:
            Expr const &if_;
        };

        template<typename Expr>
        struct with_else : Expr
        {
            template<typename T>
            with_else(T const &expr)
              : Expr(expr)
              , else_(*this)
            {}

            else_generator<Expr> else_;
        };

        template<typename Cond>
        struct if_generator
        {
            explicit if_generator(Cond const &c)
              : cond(c)
            {}

            template<typename Body>
            with_else<
                typename proto::result_of::make_expr<
                    tag::if_
                  , lldomain
                  , Cond const &
                  , Body const &
                >::type
            > const
            operator[](Body const &body) const
            {
                return proto::make_expr<tag::if_, lldomain>(
                    boost::ref(this->cond)
                  , boost::ref(body)
                );
            }

        private:
            Cond const &cond;
        };

        template<typename Expr>
        if_generator<Expr> if_(Expr const &expr)
        {
            return if_generator<Expr>(expr);
        }

        template<typename Init, typename Cond, typename Oper>
        struct for_generator
        {
            explicit for_generator(Init const &i, Cond const &c, Oper const &o)
              : init(i)
              , cond(c)
              , oper(o)
            {}

            template<typename Body>
            typename proto::result_of::make_expr<
                tag::for_
              , lldomain
              , Init const &
              , Cond const &
              , Oper const &
              , Body const &
            >::type const
            operator[](Body const &body) const
            {
                return proto::make_expr<tag::for_, lldomain>(
                    boost::ref(this->init)
                  , boost::ref(this->cond)
                  , boost::ref(this->oper)
                  , boost::ref(body)
                );
            }

        private:
            Init const &init;
            Cond const &cond;
            Oper const &oper;
        };

        template<typename Init, typename Cond, typename Oper>
        for_generator<Init, Cond, Oper> for_(Init const &i, Cond const &c, Oper const &o)
        {
            return for_generator<Init, Cond, Oper>(i, c, o);
        }

        template<typename Body>
        struct do_while_generator
        {
            explicit do_while_generator(Body const &b)
              : body(b)
            {}

            template<typename Cond>
            typename proto::result_of::make_expr<
                tag::do_while_
              , lldomain
              , Body const &
              , Cond const &
            >::type const
            operator()(Cond const &cond) const
            {
                return proto::make_expr<tag::do_while_, lldomain>(
                    boost::ref(this->body)
                  , boost::ref(cond)
                );
            }

        private:
            Body const &body;
        };

        template<typename Body>
        struct do_body
        {
            explicit do_body(Body const &body)
              : while_(body)
            {}

            do_while_generator<Body> while_;
        };

        struct do_generator
        {
            template<typename Body>
            do_body<Body> operator[](Body const &body) const
            {
                return do_body<Body>(body);
            }
        };

        do_generator const do_ = {};

        struct noop_fun
        {
            typedef void result_type;
            void operator()() const {}
        };

        typedef llexpr<proto::function<llexpr<proto::terminal<noop_fun>::type> >::type> noop_type;
        noop_type const noop = {{{{{}}}}};

        template<typename Init, typename Cond, typename Oper>
        typename proto::result_of::make_expr<
            tag::for_
          , lldomain
          , Init const &
          , Cond const &
          , Oper const &
          , noop_type const &
        >::type const
        for_loop(Init const &init, Cond const &cond, Oper const &oper)
        {
            return proto::make_expr<tag::for_, lldomain>(
                boost::ref(init)
              , boost::ref(cond)
              , boost::ref(oper)
              , boost::ref(noop)
            );
        }

        template<typename Init, typename Cond, typename Oper, typename Body>
        typename proto::result_of::make_expr<
            tag::for_
          , lldomain
          , Init const &
          , Cond const &
          , Oper const &
          , Body const &
        >::type const
        for_loop(Init const &init, Cond const &cond, Oper const &oper, Body const &body)
        {
            return proto::make_expr<tag::for_>(
                boost::ref(init)
              , boost::ref(cond)
              , boost::ref(oper)
              , boost::ref(body)
            );
        }

        template<typename Cond, typename Body>
        typename proto::result_of::make_expr<
            tag::while_
          , lldomain
          , Cond const &
          , Body const &
        >::type const
        while_loop(Cond const &cond, Body const &body)
        {
            return proto::make_expr<tag::while_, lldomain>(
                boost::ref(cond)
              , boost::ref(body)
            );
        }

        template<typename Cond>
        typename proto::result_of::make_expr<
            tag::while_
          , lldomain
          , Cond const &
          , noop_type const &
        >::type const
        while_loop(Cond const &cond)
        {
            return proto::make_expr<tag::while_, lldomain>(
                boost::ref(cond)
              , boost::ref(noop)
            );
        }

        template<typename Cond, typename Body>
        typename proto::result_of::make_expr<
            tag::do_while_
          , lldomain
          , Body const &
          , Cond const &
        >::type const
        do_while_loop(Cond const &cond, Body const &body)
        {
            return proto::make_expr<tag::do_while_, lldomain>(
                boost::ref(body)
              , boost::ref(cond)
            );
        }

        template<typename Cond>
        typename proto::result_of::make_expr<
            tag::do_while_
          , lldomain
          , noop_type const &
          , Cond const &
        >::type const
        do_while_loop(Cond const &cond)
        {
            return proto::make_expr<tag::do_while_, lldomain>(
                boost::ref(noop)
              , boost::ref(cond)
            );
        }

        template<typename Cond, typename Body1>
        typename proto::result_of::make_expr<
            tag::if_
          , lldomain
          , Cond const &
          , Body1 const &
        >::type const
        if_then(Cond const &cond, Body1 const &body1)
        {
            return proto::make_expr<tag::if_, lldomain>(
                boost::ref(cond)
              , boost::ref(body1)
            );
        }

        template<typename Cond, typename Body1, typename Body2>
        typename proto::result_of::make_expr<
            tag::if_else_
          , lldomain
          , Cond const &
          , Body1 const &
          , Body2 const &
        >::type const
        if_then_else(Cond const &cond, Body1 const &body1, Body2 const &body2)
        {
            return proto::make_expr<tag::if_else_, lldomain>(
                boost::ref(cond)
              , boost::ref(body1)
              , boost::ref(body2)
            );
        }

        template<typename Cond, typename Body1, typename Body2>
        typename proto::result_of::make_expr<
            proto::tag::if_else_
          , lldomain
          , Cond const &
          , Body1 const &
          , Body2 const &
        >::type const
        if_then_else_return(Cond const &cond, Body1 const &body1, Body2 const &body2)
        {
            return proto::make_expr<proto::tag::if_else_, lldomain>(
                boost::ref(cond)
              , boost::ref(body1)
              , boost::ref(body2)
            );
        }

        template<typename T>
        T const &make_const(T const &t)
        {
            return t;
        }

        #define M1(N, typename_A, A_const_ref, A_const_ref_a, ref_a)                                \
        template<typename_A(N)>                                                                     \
        typename proto::result_of::make_expr<                                                       \
            proto::tag::function                                                                    \
          , lldomain                                                                                \
          , A_const_ref(N)                                                                          \
        >::type const                                                                               \
        bind(A_const_ref_a(N))                                                                      \
        {                                                                                           \
            return proto::make_expr<proto::tag::function, lldomain>(ref_a(N));                      \
        }                                                                                           \
                                                                                                    \
        template<typename Ret, typename_A(N)>                                                       \
        typename proto::result_of::make_expr<                                                       \
            proto::tag::function                                                                    \
          , lldomain                                                                                \
          , A_const_ref(N)                                                                          \
        >::type const                                                                               \
        bind(A_const_ref_a(N))                                                                      \
        {                                                                                           \
            return proto::make_expr<proto::tag::function, lldomain>(ref_a(N));                      \
        }                                                                                           \
        /**/
        BOOST_PROTO_REPEAT_FROM_TO(1, BOOST_PP_INC(BOOST_PROTO_MAX_ARITY), M1)
        #undef M1

        template<typename Ret, typename Expr>
        Expr const &ret(Expr const &expr)
        {
            return expr;
        }

        template<typename Expr>
        Expr const &const_parameters(Expr const &expr)
        {
            return expr;
        }

        template<typename Expr>
        Expr const &break_const(Expr const &expr)
        {
            return expr;
        }

        template<typename Lambda>
        proto::unexpr<Lambda> const
        unlambda(Lambda const &lambda)
        {
            return proto::unexpr<Lambda>(lambda);
        }

        template<typename Lambda>
        typename proto::result_of::make_expr<
            tag::protect
          , lldomain
          , Lambda const &
        >::type const
        protect(Lambda const &lambda)
        {
            return proto::make_expr<tag::protect, lldomain>(boost::ref(lambda));
        }

        template<typename T>
        T const std_functor(T const &t)
        {
            return t;
        }

        template<typename T>
        struct ll_static_cast_fun
        {
            typedef T result_type;

            template<typename U>
            T operator()(U &u) const
            {
                return static_cast<T>(u);
            }

            template<typename U>
            T operator()(U const &u) const
            {
                return static_cast<T>(u);
            }
        };

        template<typename T, typename U>
        typename proto::result_of::make_expr<
            proto::tag::function
          , lldomain
          , ll_static_cast_fun<T>
          , U const &
        >::type
        ll_static_cast(U const &u)
        {
            ll_static_cast_fun<T> fun;
            return proto::make_expr<proto::tag::function, lldomain>(fun, boost::ref(u));
        }

        template<typename T>
        struct ll_const_cast_fun
        {
            typedef T result_type;

            template<typename U>
            T operator()(U &u) const
            {
                return const_cast<T>(u);
            }

            template<typename U>
            T operator()(U const &u) const
            {
                return const_cast<T>(u);
            }
        };

        template<typename T, typename U>
        typename proto::result_of::make_expr<
            proto::tag::function
          , lldomain
          , ll_const_cast_fun<T>
          , U const &
        >::type
        ll_const_cast(U const &u)
        {
            ll_const_cast_fun<T> fun;
            return proto::make_expr<proto::tag::function, lldomain>(fun, boost::ref(u));
        }

        template<typename T>
        struct ll_dynamic_cast_fun
        {
            typedef T result_type;

            template<typename U>
            T operator()(U &u) const
            {
                return dynamic_cast<T>(u);
            }

            template<typename U>
            T operator()(U const &u) const
            {
                return dynamic_cast<T>(u);
            }
        };

        template<typename T, typename U>
        typename proto::result_of::make_expr<
            proto::tag::function
          , lldomain
          , ll_dynamic_cast_fun<T>
          , U const &
        >::type
        ll_dynamic_cast(U const &u)
        {
            ll_dynamic_cast_fun<T> fun;
            return proto::make_expr<proto::tag::function, lldomain>(fun, boost::ref(u));
        }

        template<typename T>
        struct ll_reinterpret_cast_fun
        {
            typedef T result_type;

            template<typename U>
            T operator()(U &u) const
            {
                return reinterpret_cast<T>(u);
            }

            template<typename U>
            T operator()(U const &u) const
            {
                return reinterpret_cast<T>(u);
            }
        };

        template<typename T, typename U>
        typename proto::result_of::make_expr<
            proto::tag::function
          , lldomain
          , ll_reinterpret_cast_fun<T>
          , U const &
        >::type
        ll_reinterpret_cast(U const &u)
        {
            ll_reinterpret_cast_fun<T> fun;
            return proto::make_expr<proto::tag::function, lldomain>(fun, boost::ref(u));
        }

        struct ll_sizeof_fun
        {
            typedef std::size_t result_type;

            template<typename U>
            std::size_t operator()(U const &) const
            {
                return sizeof(U);
            }
        };

        template<typename U>
        typename proto::result_of::make_expr<
            proto::tag::function
          , lldomain
          , ll_sizeof_fun
          , U const &
        >::type
        ll_sizeof(U const &u)
        {
            ll_sizeof_fun fun;
            return proto::make_expr<proto::tag::function, lldomain>(fun, boost::ref(u));
        }

        struct ll_typeid_fun
        {
            typedef std::type_info const &result_type;

            template<typename U>
            std::type_info const &operator()(U const &) const
            {
                return typeid(U);
            }
        };

        template<typename U>
        typename proto::result_of::make_expr<
            proto::tag::function
          , lldomain
          , ll_typeid_fun
          , U const &
        >::type
        ll_typeid(U const &u)
        {
            ll_typeid_fun fun;
            return proto::make_expr<proto::tag::function, lldomain>(fun, boost::ref(u));
        }

        template<typename T>
        struct constructor
        {
            typedef T result_type;

            T operator()() const
            {
                return T();
            }

            #define M0(Z, N, DATA)                                                                  \
            template<BOOST_PP_ENUM_PARAMS_Z(Z, N, typename A)>                                      \
            T operator()(BOOST_PP_ENUM_BINARY_PARAMS_Z(Z, N, A, const &a)) const                    \
            {                                                                                       \
                return T(BOOST_PP_ENUM_PARAMS_Z(Z, N, a));                                          \
            }                                                                                       \
            /**/
            BOOST_PP_REPEAT_FROM_TO(1, BOOST_PROTO_MAX_ARITY, M0, ~)
            #undef M0
        };

        template<typename T>
        struct new_ptr
        {
            typedef T *result_type;

            T *operator()() const
            {
                return new T();
            }

            #define M0(Z, N, DATA)                                                                  \
            template<BOOST_PP_ENUM_PARAMS_Z(Z, N, typename A)>                                      \
            T *operator()(BOOST_PP_ENUM_BINARY_PARAMS_Z(Z, N, A, const &a)) const                   \
            {                                                                                       \
                return new T(BOOST_PP_ENUM_PARAMS_Z(Z, N, a));                                      \
            }                                                                                       \
            /**/
            BOOST_PP_REPEAT_FROM_TO(1, BOOST_PROTO_MAX_ARITY, M0, ~)
            #undef M0
        };

        struct destructor
        {
            typedef void result_type;

            template<typename T>
            void operator()(T const &t) const
            {
                t.~T();
            }

            template<typename T>
            void operator()(T *const &t) const
            {
                (*t).~T();
            }
        };

        struct delete_ptr
        {
            typedef void result_type;
            template<typename T>
            void operator()(T *t) const
            {
                delete t;
            }
        };

        template<typename T>
        struct new_array
        {
            typedef T *result_type;
            T *operator()(std::size_t n) const
            {
                return new T[n];
            }
        };

        struct delete_array
        {
            typedef void result_type;
            template<typename T>
            void operator()(T *t) const
            {
                delete[] t;
            }
        };

        template<typename T>
        struct type2type {};

        struct try_catch_nil {};

        template<typename Head, typename Tail>
        struct try_catch_cons : Tail
        {
            typedef typename Head::proto_tag::exception_type exception_type;

            try_catch_cons(Head const &head, Tail const &tail)
              : Tail(tail)
              , head(head)
            {}

            template<typename State, typename Data>
            typename result_of<Tail const(State const &, Data &)>::type
            operator()(State const &state, Data &data) const
            {
                return this->invoke(state, data, type2type<exception_type>());
            }

        private:
            // catch(Exception const &)
            template<typename State, typename Data, typename Exception>
            typename result_of<Tail const(State const &, Data &)>::type
            invoke(State const &state, Data &data, type2type<Exception>) const
            {
                typedef typename result_of<Tail const(State const &, Data &)>::type result_type;
                try
                {
                    return static_cast<result_type>(this->Tail::operator()(state, data));
                }
                catch(Exception const &e)
                {
                    return static_cast<result_type>(Eval()(this->head, e, data));
                }
            }

            // catch(...)
            template<typename State, typename Data>
            typename result_of<Tail const(State const &, Data &)>::type
            invoke(State const &state, Data &data, type2type<tag::catch_all_>) const
            {
                typedef typename result_of<Tail const(State const &, Data &)>::type result_type;
                try
                {
                    return static_cast<result_type>(this->Tail::operator()(state, data));
                }
                catch(...)
                {
                    return static_cast<result_type>(Eval()(this->head, tag::catch_all_(), data));
                }
            }

            Head const &head;
        };

        template<typename Head>
        struct try_catch_cons<Head, try_catch_nil> : proto::callable
        {
            try_catch_cons(Head const &head, try_catch_nil const &)
              : head(head)
            {}

            template<typename Sig>
            struct result;

            template<typename This, typename State, typename Data>
            struct result<This(State, Data)>
              : result_of<Eval(Head const &, State, Data)>
            {};

            template<typename State, typename Data>
            typename result_of<Eval(Head const &, State, Data)>::type
            operator()(State const &state, Data &data) const
            {
                return Eval()(this->head, state, data);
            }

        private:
            Head const &head;
        };

        struct try_catch_fun : proto::callable
        {
            template<typename Sig>
            struct result;

            template<typename This, typename Fun, typename State, typename Data>
            struct result<This(Fun, State, Data)>
              : result_of<Fun(State, Data)>
            {};

            template<typename Fun, typename State, typename Data>
            typename result_of<Fun(State const &, Data &)>::type
            operator()(Fun const &fun, State const &state, Data &data) const
            {
                return fun(state, data);
            }
        };

        template<>
        struct Cases::case_<tag::try_>
          : proto::otherwise<
                try_catch_fun(
                    proto::fold<
                        proto::_
                      , try_catch_nil()
                      , try_catch_cons<proto::_, proto::_state>(proto::_, proto::_state)
                    >
                  , proto::_state
                  , proto::_data
                )
            >
        {};

        template<typename E, typename Expr>
        typename proto::result_of::make_expr<tag::catch_<E>, lldomain, Expr const &>::type const
        catch_exception(Expr const &expr)
        {
            return proto::make_expr<tag::catch_<E>, lldomain>(boost::ref(expr));
        }

        template<typename E>
        typename proto::result_of::make_expr<tag::catch_<E>, lldomain, noop_type const &>::type const
        catch_exception()
        {
            return proto::make_expr<tag::catch_<E>, lldomain>(boost::ref(noop));
        }

        template<typename Expr>
        typename proto::result_of::make_expr<
            tag::catch_all_
          , lldomain
          , Expr const &
        >::type const
        catch_all(Expr const &expr)
        {
            return proto::make_expr<tag::catch_all_, lldomain>(boost::ref(expr));
        }

        inline
        proto::result_of::make_expr<tag::catch_all_, lldomain, noop_type const &>::type const
        catch_all()
        {
            return proto::make_expr<tag::catch_all_, lldomain>(boost::ref(noop));
        }

        #define M1(N, typename_A, A_const_ref, A_const_ref_a, ref_a)                                \
        template<typename_A(N)>                                                                     \
        typename proto::result_of::make_expr<                                                       \
            tag::try_                                                                               \
          , lldomain                                                                                \
          , A_const_ref(N)                                                                          \
        >::type const                                                                               \
        try_catch(A_const_ref_a(N))                                                                 \
        {                                                                                           \
            return proto::make_expr<tag::try_, lldomain>(ref_a(N));                                 \
        }                                                                                           \
        /**/
        BOOST_PROTO_REPEAT_FROM_TO(2, BOOST_PP_INC(BOOST_PROTO_MAX_ARITY), M1)
        #undef M1

        template<typename Expr>
        typename proto::result_of::make_expr<
            proto::tag::function
          , lldomain
          , throw_fun
          , Expr const &
        >::type const
        throw_exception(Expr const &expr)
        {
            throw_fun fun;
            return proto::make_expr<proto::tag::function, lldomain>(fun, boost::ref(expr));
        }

        inline
        proto::result_of::make_expr<proto::tag::function, lldomain, rethrow_fun>::type const
        rethrow()
        {
            return proto::make_expr<proto::tag::function, lldomain>(rethrow_fun());
        }

        struct make_void_fun
        {
            typedef void result_type;
            template<typename T>
            void operator()(T const &) const
            {}
        };

        template<typename Expr>
        typename proto::result_of::make_expr<
            proto::tag::function
          , lldomain
          , make_void_fun
          , Expr const &
        >::type const
        make_void(Expr const &expr)
        {
            make_void_fun fun;
            return proto::make_expr<proto::tag::function, lldomain>(fun, boost::ref(expr));
        }

        #define M1(N, typename_A, A_const_ref, A_const_ref_a, ref_a)                                \
        template<typename_A(N)>                                                                     \
        typename proto::result_of::make_expr<                                                       \
            tag::switch_                                                                            \
          , lldomain                                                                                \
          , A_const_ref(N)                                                                          \
        >::type const                                                                               \
        switch_statement(A_const_ref_a(N))                                                          \
        {                                                                                           \
            return proto::make_expr<tag::switch_, lldomain>(ref_a(N));                              \
        }                                                                                           \
        /**/
        BOOST_PROTO_REPEAT_FROM_TO(2, BOOST_PP_INC(BOOST_PROTO_MAX_ARITY), M1)
        #undef M1

        template<int I, typename Expr>
        typename proto::result_of::make_expr<tag::case_<I>, lldomain, Expr const &>::type const
        case_statement(Expr const &expr)
        {
            return proto::make_expr<tag::case_<I>, lldomain>(boost::ref(expr));
        }

        template<int I>
        typename proto::result_of::make_expr<tag::case_<I>, lldomain, noop_type const &>::type const
        case_statement()
        {
            return proto::make_expr<tag::case_<I>, lldomain>(boost::ref(noop));
        }

        template<typename Expr>
        typename proto::result_of::make_expr<tag::default_, lldomain, Expr const &>::type const
        default_statement(Expr const &expr)
        {
            return proto::make_expr<tag::default_, lldomain>(boost::ref(expr));
        }

        inline
        proto::result_of::make_expr<tag::default_, lldomain, noop_type const &>::type const
        default_statement()
        {
            return proto::make_expr<tag::default_, lldomain>(boost::ref(noop));
        }

        namespace ll
        {
            struct for_each
            {
                template<typename Sig>
                struct result;

                template<typename This, typename Begin, typename End, typename Fun>
                struct result<This(Begin, End, Fun)>
                  : remove_const<typename remove_reference<Fun>::type>
                {};

                template<typename InIter, typename Fun>
                Fun operator()(InIter begin, InIter end, Fun fun) const
                {
                    return std::for_each(begin, end, fun);
                }
            };
        }

    }}

    namespace boost
    {
        template<typename Expr>
        struct result_of<lambda::llexpr<Expr>()>
          : lambda::llexpr<Expr>::template result<lambda::llexpr<Expr>()>
        {};

        template<typename Expr>
        struct result_of<lambda::llexpr<Expr> const()>
          : lambda::llexpr<Expr>::template result<lambda::llexpr<Expr> const()>
        {};
    }

    #ifdef _MSC_VER
    # pragma warning(pop)
    #endif

    #endif

#else

    BOOST_PP_SEQ_FOR_EACH_PRODUCT(
        M2,
        BOOST_PP_REPEAT(BOOST_PP_ITERATION(), M1, ~)
    )

#endif

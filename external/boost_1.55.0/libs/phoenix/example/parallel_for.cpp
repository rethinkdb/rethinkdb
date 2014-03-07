
#include <boost/phoenix.hpp>

struct omp_for_eval
{
    typedef void result_type;

    template <typename Init, typename Cond, typename Step, typename Do, typename Context>
    result_type
    operator()(
        Init const& init
      , Cond const& cond
      , Step const& step
      , Do const& do_
      , Context & ctx
    ) const
    {
#pragma omp parallel
        for(
            boost::phoenix::eval(init, ctx);
            boost::phoenix::eval(cond, ctx);
            boost::phoenix::eval(step, ctx)
        )
        {
            boost::phoenix::eval(do_, ctx);
        }
    }
};


////////////////////////////////////////////////////////////////////////////////
// Define new custom expression
BOOST_PHOENIX_DEFINE_EXPRESSION(
    (omp_for)
  , (boost::phoenix::meta_grammar) // Cond
    (boost::phoenix::meta_grammar) // Init
    (boost::phoenix::meta_grammar) // Step
    (boost::phoenix::meta_grammar) // Do
)

namespace boost { namespace phoenix
{
    template <>
    struct default_actions::when< ::rule::omp_for>
        : boost::phoenix::call< ::omp_for_eval>
    {};
}}

template <typename Init, typename Cond, typename Step>
struct omp_for_gen
{
    omp_for_gen(Init const& init, Cond const& cond, Step const& step)
        : init(init), cond(cond), step(step) {}
    
    template <typename Do>
    typename result_of::make_omp_for<Init, Cond, Step, Do>::type const
    operator[](Do const& do_) const
    {
        return make_omp_for(init, cond, step, do_);
    }
    
    Init init;
    Cond cond;
    Step step;
};

template <typename Init, typename Cond, typename Step>
inline
omp_for_gen<Init, Cond, Step> const
omp_for(Init const& init, Cond const& cond, Step const& step)
{
    return omp_for_gen<Init, Cond, Step>(init, cond, step);
}
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Define new evaluation scheme

struct parallel_actions
{
    template <typename Rule>
    struct when
        : boost::phoenix::default_actions::when<Rule>
    {};
};

template <>
struct parallel_actions::when<boost::phoenix::rule::for_>
    : boost::phoenix::call<omp_for_eval>
{};

// Doing the same as actor<Expr>::operator
template <typename Expr, typename A0, typename A1, typename A2>
typename boost::phoenix::result_of::eval<
    Expr const &
  , typename boost::phoenix::result_of::make_context<
        typename boost::phoenix::result_of::make_env<
            Expr const *
          , A0 &
          , A1 &
          , A2 &
        >::type
      , parallel_actions
    >::type
>::type
parallel_eval(Expr & expr, A0 & a0, A1 & a1, A2 & a2)
{
    Expr const * this_ = boost::addressof(expr);
    return
        boost::phoenix::eval(
            expr
          , boost::phoenix::make_context(
                boost::phoenix::make_env(this_, a0, a1, a2)
              , parallel_actions()
            )
        );
}

// changing evaluation mechanism on the fly
BOOST_PHOENIX_DEFINE_EXPRESSION(
    (parallel)
  , (boost::phoenix::meta_grammar)
)

namespace boost { namespace phoenix
{
    template <>
    struct default_actions::when< ::rule::parallel>
        : proto::call<
            evaluator(
                proto::_child0
              , functional::make_context(
                    _env
                  , parallel_actions()
                )
              , unused()//mpl::void_()
            )
        >
    {};
}}

template <typename Expr>
typename result_of::make_parallel<Expr>::type
parallel(Expr const & expr)
{
    return make_parallel(expr);
}
////////////////////////////////////////////////////////////////////////////////


#include <vector>
#include <iostream>

int main()
{
    using boost::phoenix::arg_names::_1;
    using boost::phoenix::arg_names::_2;
    using boost::phoenix::arg_names::_3;
    using boost::phoenix::local_names::_a;
    using boost::phoenix::local_names::_b;
    using boost::phoenix::local_names::_c;
    using boost::phoenix::let;
    using boost::phoenix::bind;
    using boost::phoenix::lambda;
    using boost::phoenix::nothing;

    const int NUM = 1;

    {
        std::vector<int> a(NUM, 1);
        std::vector<int> b(NUM, 2);
        std::vector<int> c(NUM, 0);

        (
            let(_a = begin(_1), _b = begin(_2), _c = begin(_3))
            [
                for_(nothing, _a != end(_1), (++_a, ++_b, ++_c))
                [
                    *_c = *_a + *_b
                ]
            ]
          , std::cout << accumulate(_3, 0) << "\n"
        )(a, b, c);
    }

    {
        std::vector<int> a(NUM, 1);
        std::vector<int> b(NUM, 2);
        std::vector<int> c(NUM, 0);

        (
            let(_a = begin(_1), _b = begin(_2), _c = begin(_3))
            [
                omp_for(nothing, _a != end(_1), (++_a, ++_b, ++_c))
                [
                    *_c = *_a + *_b
                ]
              , std::cout << accumulate(_3, 0) << "\n"
            ]
        )(a, b, c);
    }
    
    {
        std::vector<int> a(NUM, 1);
        std::vector<int> b(NUM, 2);
        std::vector<int> c(NUM, 0);

        parallel_eval(
            let(_a = begin(_1), _b = begin(_2), _c = begin(_3))
            [
                for_(nothing, _a != end(_1), (++_a, ++_b, ++_c))
                [
                    *_c = *_a + *_b
                ]
              , std::cout << accumulate(_3, 0) << "\n"
            ]
          , a, b, c);
    }
    
    {
        std::vector<int> a(NUM, 1);
        std::vector<int> b(NUM, 2);
        std::vector<int> c(NUM, 0);

        (
            let(_a = begin(_1), _b = begin(_2), _c = begin(_3))
            [
                parallel(
                    for_(nothing, _a != end(_1), (++_a, ++_b, ++_c))
                    [
                        *_c = *_a + *_b
                    ]
                )
            ]
          , std::cout << accumulate(_3, 0) << "\n"
        )(a, b, c);
    }
}

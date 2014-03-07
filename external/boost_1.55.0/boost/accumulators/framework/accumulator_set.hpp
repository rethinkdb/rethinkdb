///////////////////////////////////////////////////////////////////////////////
// accumulator_set.hpp
//
//  Copyright 2005 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ACCUMULATORS_FRAMEWORK_ACCUMULATOR_SET_HPP_EAN_28_10_2005
#define BOOST_ACCUMULATORS_FRAMEWORK_ACCUMULATOR_SET_HPP_EAN_28_10_2005

#include <boost/version.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/protect.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/parameter/parameters.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/accumulators/accumulators_fwd.hpp>
#include <boost/accumulators/framework/depends_on.hpp>
#include <boost/accumulators/framework/accumulator_concept.hpp>
#include <boost/accumulators/framework/parameters/accumulator.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>
#include <boost/accumulators/framework/accumulators/external_accumulator.hpp>
#include <boost/accumulators/framework/accumulators/droppable_accumulator.hpp>
#include <boost/fusion/include/any.hpp>
#include <boost/fusion/include/find_if.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/filter_view.hpp>

namespace boost { namespace accumulators
{

namespace detail
{
    ///////////////////////////////////////////////////////////////////////////////
    // accumulator_visitor
    //   wrap a boost::parameter argument pack in a Fusion extractor object
    template<typename Args>
    struct accumulator_visitor
    {
        explicit accumulator_visitor(Args const &a)
          : args(a)
        {
        }

        template<typename Accumulator>
        void operator ()(Accumulator &accumulator) const
        {
            accumulator(this->args);
        }

    private:
        accumulator_visitor &operator =(accumulator_visitor const &);
        Args const &args;
    };

    template<typename Args>
    inline accumulator_visitor<Args> const make_accumulator_visitor(Args const &args)
    {
        return accumulator_visitor<Args>(args);
    }

    typedef
        parameter::parameters<
            parameter::required<tag::accumulator>
          , parameter::optional<tag::sample>
          // ... and others which are not specified here...
        >
    accumulator_params;

    ///////////////////////////////////////////////////////////////////////////////
    // accumulator_set_base
    struct accumulator_set_base
    {
    };

    ///////////////////////////////////////////////////////////////////////////////
    // is_accumulator_set
    template<typename T>
    struct is_accumulator_set
      : is_base_and_derived<accumulator_set_base, T>
    {
    };

} // namespace detail

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4355) // warning C4355: 'this' : used in base member initializer list
#endif

///////////////////////////////////////////////////////////////////////////////
/// \brief A set of accumulators.
///
/// accumulator_set resolves the dependencies between features and ensures that
/// the accumulators in the set are updated in the proper order.
///
/// acccumulator_set provides a general mechanism to visit the accumulators
/// in the set in order, with or without a filter. You can also fetch a reference
/// to an accumulator that corresponds to a feature.
///
template<typename Sample, typename Features, typename Weight>
struct accumulator_set
  : detail::accumulator_set_base
{
    typedef Sample sample_type;     ///< The type of the samples that will be accumulated
    typedef Features features_type; ///< An MPL sequence of the features that should be accumulated.
    typedef Weight weight_type;     ///< The type of the weight parameter. Must be a scalar. Defaults to void.

    /// INTERNAL ONLY
    ///
    typedef
        typename detail::make_accumulator_tuple<
            Features
          , Sample
          , Weight
        >::type
    accumulators_mpl_vector;

    // generate a fusion::list of accumulators
    /// INTERNAL ONLY
    ///
    typedef
        typename detail::meta::make_acc_list<
            accumulators_mpl_vector
        >::type
    accumulators_type;

    /// INTERNAL ONLY
    ///
    //BOOST_MPL_ASSERT((mpl::is_sequence<accumulators_type>));

    ///////////////////////////////////////////////////////////////////////////////
    /// default-construct all contained accumulators
    accumulator_set()
      : accumulators(
            detail::make_acc_list(
                accumulators_mpl_vector()
              , detail::accumulator_params()(*this)
            )
        )
    {
        // Add-ref the Features that the user has specified
        this->template visit_if<detail::contains_feature_of_<Features> >(
            detail::make_add_ref_visitor(detail::accumulator_params()(*this))
        );
    }

    /// \overload
    ///
    /// \param a1 Optional named parameter to be passed to all the accumulators
    template<typename A1>
    explicit accumulator_set(A1 const &a1)
      : accumulators(
            detail::make_acc_list(
                accumulators_mpl_vector()
              , detail::accumulator_params()(*this, a1)
            )
        )
    {
        // Add-ref the Features that the user has specified
        this->template visit_if<detail::contains_feature_of_<Features> >(
            detail::make_add_ref_visitor(detail::accumulator_params()(*this))
        );
    }

    // ... other overloads generated by Boost.Preprocessor:

    /// INTERNAL ONLY
    ///
#define BOOST_ACCUMULATORS_ACCUMULATOR_SET_CTOR(z, n, _)                                \
    template<BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)>                                  \
    accumulator_set(BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, A, const &a))                   \
      : accumulators(                                                                   \
            detail::make_acc_list(                                                      \
                accumulators_mpl_vector()                                               \
              , detail::accumulator_params()(                                           \
                    *this BOOST_PP_ENUM_TRAILING_PARAMS_Z(z, n, a)                      \
                )                                                                       \
            )                                                                           \
        )                                                                               \
    {                                                                                   \
        /* Add-ref the Features that the user has specified */                          \
        this->template visit_if<detail::contains_feature_of_<Features> >(               \
            detail::make_add_ref_visitor(detail::accumulator_params()(*this))           \
        );                                                                              \
    }

    /// INTERNAL ONLY
    ///
    BOOST_PP_REPEAT_FROM_TO(
        2
      , BOOST_PP_INC(BOOST_ACCUMULATORS_MAX_ARGS)
      , BOOST_ACCUMULATORS_ACCUMULATOR_SET_CTOR
      , _
    )

    #ifdef BOOST_ACCUMULATORS_DOXYGEN_INVOKED
    /// \overload
    ///
    template<typename A1, typename A2, ...>
    accumulator_set(A1 const &a1, A2 const &a2, ...);
    #endif

    // ... other overloads generated by Boost.Preprocessor below ...

    ///////////////////////////////////////////////////////////////////////////////
    /// Visitation
    /// \param func UnaryFunction which is invoked with each accumulator in turn.
    template<typename UnaryFunction>
    void visit(UnaryFunction const &func)
    {
        fusion::for_each(this->accumulators, func);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// Conditional visitation
    /// \param func UnaryFunction which is invoked with each accumulator in turn,
    ///     provided the accumulator satisfies the MPL predicate FilterPred.
    template<typename FilterPred, typename UnaryFunction>
    void visit_if(UnaryFunction const &func)
    {
        fusion::filter_view<accumulators_type, FilterPred> filtered_accs(this->accumulators);
        fusion::for_each(filtered_accs, func);
    }
    
    ///////////////////////////////////////////////////////////////////////////////
    /// The return type of the operator() overloads is void.
    typedef void result_type;

    ///////////////////////////////////////////////////////////////////////////////
    /// Accumulation
    /// \param a1 Optional named parameter to be passed to all the accumulators
    void operator ()()
    {
        this->visit(
            detail::make_accumulator_visitor(
                detail::accumulator_params()(*this)
            )
        );
    }

    template<typename A1>
    void operator ()(A1 const &a1)
    {
        this->visit(
            detail::make_accumulator_visitor(
                detail::accumulator_params()(*this, a1)
            )
        );
    }

    // ... other overloads generated by Boost.Preprocessor:

    /// INTERNAL ONLY
    ///
#define BOOST_ACCUMULATORS_ACCUMULATOR_SET_FUN_OP(z, n, _)                              \
    template<BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)>                                  \
    void operator ()(BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, A, const &a))                  \
    {                                                                                   \
        this->visit(                                                                    \
            detail::make_accumulator_visitor(                                           \
                detail::accumulator_params()(                                           \
                    *this BOOST_PP_ENUM_TRAILING_PARAMS_Z(z, n, a)                      \
                )                                                                       \
            )                                                                           \
        );                                                                              \
    }

    /// INTERNAL ONLY
    ///
    BOOST_PP_REPEAT_FROM_TO(
        2
      , BOOST_PP_INC(BOOST_ACCUMULATORS_MAX_ARGS)
      , BOOST_ACCUMULATORS_ACCUMULATOR_SET_FUN_OP
      , _
    )

    #ifdef BOOST_ACCUMULATORS_DOXYGEN_INVOKED
    /// \overload
    ///
    template<typename A1, typename A2, ...>
    void operator ()(A1 const &a1, A2 const &a2, ...);
    #endif

    ///////////////////////////////////////////////////////////////////////////////
    /// Extraction
    template<typename Feature>
    struct apply
      : fusion::result_of::value_of<
            typename fusion::result_of::find_if<
                accumulators_type
              , detail::matches_feature<Feature>
            >::type
        >
    {
    };

    ///////////////////////////////////////////////////////////////////////////////
    /// Extraction
    template<typename Feature>
    typename apply<Feature>::type &extract()
    {
        return *fusion::find_if<detail::matches_feature<Feature> >(this->accumulators);
    }

    /// \overload
    template<typename Feature>
    typename apply<Feature>::type const &extract() const
    {
        return *fusion::find_if<detail::matches_feature<Feature> >(this->accumulators);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// Drop
    template<typename Feature>
    void drop()
    {
        // You can only drop the features that you have specified explicitly
        typedef typename apply<Feature>::type the_accumulator;
        BOOST_MPL_ASSERT((detail::contains_feature_of<Features, the_accumulator>));

        typedef
            typename feature_of<typename as_feature<Feature>::type>::type
        the_feature;

        (*fusion::find_if<detail::matches_feature<Feature> >(this->accumulators))
            .drop(detail::accumulator_params()(*this));

        // Also drop accumulators that this feature depends on
        typedef typename the_feature::dependencies dependencies;
        this->template visit_if<detail::contains_feature_of_<dependencies> >(
            detail::make_drop_visitor(detail::accumulator_params()(*this))
        );
    }

private:

    accumulators_type accumulators;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

///////////////////////////////////////////////////////////////////////////////
// find_accumulator
//   find an accumulator in an accumulator_set corresponding to a feature
template<typename Feature, typename AccumulatorSet>
typename mpl::apply<AccumulatorSet, Feature>::type &
find_accumulator(AccumulatorSet &acc BOOST_ACCUMULATORS_PROTO_DISABLE_IF_IS_CONST(AccumulatorSet))
{
    return acc.template extract<Feature>();
}

/// \overload
template<typename Feature, typename AccumulatorSet>
typename mpl::apply<AccumulatorSet, Feature>::type const &
find_accumulator(AccumulatorSet const &acc)
{
    return acc.template extract<Feature>();
}

///////////////////////////////////////////////////////////////////////////////
// extract_result
//   extract a result from an accumulator set
/// INTERNAL ONLY
///
#define BOOST_ACCUMULATORS_EXTRACT_RESULT_FUN(z, n, _)                      \
    template<                                                               \
        typename Feature                                                    \
      , typename AccumulatorSet                                             \
        BOOST_PP_ENUM_TRAILING_PARAMS_Z(z, n, typename A)                   \
    >                                                                       \
    typename mpl::apply<AccumulatorSet, Feature>::type::result_type         \
    extract_result(                                                         \
        AccumulatorSet const &acc                                           \
        BOOST_PP_ENUM_TRAILING_BINARY_PARAMS_Z(z, n, A, const &a)           \
    )                                                                       \
    {                                                                       \
        return find_accumulator<Feature>(acc).result(                       \
            detail::accumulator_params()(                                   \
                acc                                                         \
                BOOST_PP_ENUM_TRAILING_PARAMS_Z(z, n, a)                    \
            )                                                               \
        );                                                                  \
    }

BOOST_PP_REPEAT(
    BOOST_PP_INC(BOOST_ACCUMULATORS_MAX_ARGS)
  , BOOST_ACCUMULATORS_EXTRACT_RESULT_FUN
  , _
)

}} // namespace boost::accumulators

#endif

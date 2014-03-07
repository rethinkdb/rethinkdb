///////////////////////////////////////////////////////////////
//  Copyright Jens Maurer 2006-1011
//  Copyright Steven Watanabe 2011
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#ifndef BOOST_MP_RANDOM_HPP
#define BOOST_MP_RANDOM_HPP

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4127)
#endif

#include <boost/multiprecision/number.hpp>

namespace boost{ namespace random{ namespace detail{
//
// This is a horrible hack: this declaration has to appear before the definition of
// uniform_int_distribution, otherwise it won't be used...
// Need to find a better solution, like make Boost.Random safe to use with
// UDT's and depricate/remove this header altogether.
//
template<class Engine, class Backend, boost::multiprecision::expression_template_option ExpressionTemplates>
boost::multiprecision::number<Backend, ExpressionTemplates> 
   generate_uniform_int(Engine& eng, const boost::multiprecision::number<Backend, ExpressionTemplates>& min_value, const boost::multiprecision::number<Backend, ExpressionTemplates>& max_value);

}}}

#include <boost/random.hpp>
#include <boost/mpl/eval_if.hpp>

namespace boost{
namespace random{
namespace detail{

template<class Backend, boost::multiprecision::expression_template_option ExpressionTemplates>
struct subtract<boost::multiprecision::number<Backend, ExpressionTemplates>, true> 
{ 
  typedef boost::multiprecision::number<Backend, ExpressionTemplates> result_type;
  result_type operator()(result_type const& x, result_type const& y) { return x - y; }
};

}

template<class Engine, std::size_t w, class Backend, boost::multiprecision::expression_template_option ExpressionTemplates>
class independent_bits_engine<Engine, w, boost::multiprecision::number<Backend, ExpressionTemplates> >
{
public:
    typedef Engine base_type;
    typedef boost::multiprecision::number<Backend, ExpressionTemplates> result_type;

    static result_type min BOOST_PREVENT_MACRO_SUBSTITUTION ()
    { return 0; }
    // This is the only function we modify compared to the primary template:
    static result_type max BOOST_PREVENT_MACRO_SUBSTITUTION ()
    {
       // This expression allows for the possibility that w == std::numeric_limits<result_type>::digits:
       return (((result_type(1) << (w - 1)) - 1) << 1) + 1; 
    }

    independent_bits_engine() { }

    BOOST_RANDOM_DETAIL_ARITHMETIC_CONSTRUCTOR(independent_bits_engine,
        result_type, seed_arg)
    {
        _base.seed(seed_arg);
    }

    BOOST_RANDOM_DETAIL_SEED_SEQ_CONSTRUCTOR(independent_bits_engine,
        SeedSeq, seq)
    { _base.seed(seq); }

    independent_bits_engine(const base_type& base_arg) : _base(base_arg) {}

    template<class It>
    independent_bits_engine(It& first, It last) : _base(first, last) { }

    void seed() { _base.seed(); }

    BOOST_RANDOM_DETAIL_ARITHMETIC_SEED(independent_bits_engine,
        result_type, seed_arg)
    { _base.seed(seed_arg); }

    BOOST_RANDOM_DETAIL_SEED_SEQ_SEED(independent_bits_engine,
        SeedSeq, seq)
    { _base.seed(seq); }

    template<class It> void seed(It& first, It last)
    { _base.seed(first, last); }

    result_type operator()()
    {
        // While it may seem wasteful to recalculate this
        // every time, both msvc and gcc can propagate
        // constants, resolving this at compile time.
        base_unsigned range =
            detail::subtract<base_result>()((_base.max)(), (_base.min)());
        std::size_t m =
            (range == (std::numeric_limits<base_unsigned>::max)()) ?
                std::numeric_limits<base_unsigned>::digits :
                detail::integer_log2(range + 1);
        std::size_t n = (w + m - 1) / m;
        std::size_t w0, n0;
        base_unsigned y0, y1;
        base_unsigned y0_mask, y1_mask;
        calc_params(n, range, w0, n0, y0, y1, y0_mask, y1_mask);
        if(base_unsigned(range - y0 + 1) > y0 / n) {
            // increment n and try again.
            ++n;
            calc_params(n, range, w0, n0, y0, y1, y0_mask, y1_mask);
        }

        BOOST_ASSERT(n0*w0 + (n - n0)*(w0 + 1) == w);

        result_type S = 0;
        for(std::size_t k = 0; k < n0; ++k) {
            base_unsigned u;
            do {
                u = detail::subtract<base_result>()(_base(), (_base.min)());
            } while(u > base_unsigned(y0 - 1));
            S = (S << w0) + (u & y0_mask);
        }
        for(std::size_t k = 0; k < (n - n0); ++k) {
            base_unsigned u;
            do {
                u = detail::subtract<base_result>()(_base(), (_base.min)());
            } while(u > base_unsigned(y1 - 1));
            S = (S << (w0 + 1)) + (u & y1_mask);
        }
        return S;
    }
  
    /** Fills a range with random values */
    template<class Iter>
    void generate(Iter first, Iter last)
    { detail::generate_from_int(*this, first, last); }

    /** Advances the state of the generator by @c z. */
    void discard(boost::uintmax_t z)
    {
        for(boost::uintmax_t i = 0; i < z; ++i) {
            (*this)();
        }
    }

    const base_type& base() const { return _base; }

    /**
     * Writes the textual representation if the generator to a @c std::ostream.
     * The textual representation of the engine is the textual representation
     * of the base engine.
     */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, independent_bits_engine, r)
    {
        os << r._base;
        return os;
    }

    /**
     * Reads the state of an @c independent_bits_engine from a
     * @c std::istream.
     */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, independent_bits_engine, r)
    {
        is >> r._base;
        return is;
    }

    /**
     * Returns: true iff the two @c independent_bits_engines will
     * produce the same sequence of values.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(independent_bits_engine, x, y)
    { return x._base == y._base; }
    /**
     * Returns: true iff the two @c independent_bits_engines will
     * produce different sequences of values.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(independent_bits_engine)

private:

    /// \cond show_private
    typedef typename base_type::result_type base_result;
    typedef typename make_unsigned<base_result>::type base_unsigned;

    void calc_params(
        std::size_t n, base_unsigned range,
        std::size_t& w0, std::size_t& n0,
        base_unsigned& y0, base_unsigned& y1,
        base_unsigned& y0_mask, base_unsigned& y1_mask)
    {
        BOOST_ASSERT(w >= n);
        w0 = w/n;
        n0 = n - w % n;
        y0_mask = (base_unsigned(2) << (w0 - 1)) - 1;
        y1_mask = (y0_mask << 1) | 1;
        y0 = (range + 1) & ~y0_mask;
        y1 = (range + 1) & ~y1_mask;
        BOOST_ASSERT(y0 != 0 || base_unsigned(range + 1) == 0);
    }
    /// \endcond

    Engine _base;
};

template<class Backend, boost::multiprecision::expression_template_option ExpressionTemplates>
class uniform_smallint<boost::multiprecision::number<Backend, ExpressionTemplates> >
{
public:
    typedef boost::multiprecision::number<Backend, ExpressionTemplates> input_type;
    typedef boost::multiprecision::number<Backend, ExpressionTemplates> result_type;

    class param_type
    {
    public:

        typedef uniform_smallint distribution_type;

        /** constructs the parameters of a @c uniform_smallint distribution. */
        param_type(result_type const& min_arg = 0, result_type const& max_arg = 9)
          : _min(min_arg), _max(max_arg)
        {
            BOOST_ASSERT(_min <= _max);
        }

        /** Returns the minimum value. */
        result_type a() const { return _min; }
        /** Returns the maximum value. */
        result_type b() const { return _max; }
        

        /** Writes the parameters to a @c std::ostream. */
        BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, param_type, parm)
        {
            os << parm._min << " " << parm._max;
            return os;
        }
    
        /** Reads the parameters from a @c std::istream. */
        BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, param_type, parm)
        {
            is >> parm._min >> std::ws >> parm._max;
            return is;
        }

        /** Returns true if the two sets of parameters are equal. */
        BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(param_type, lhs, rhs)
        { return lhs._min == rhs._min && lhs._max == rhs._max; }

        /** Returns true if the two sets of parameters are different. */
        BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(param_type)

    private:
        result_type _min;
        result_type _max;
    };

    /**
     * Constructs a @c uniform_smallint. @c min and @c max are the
     * lower and upper bounds of the output range, respectively.
     */
    explicit uniform_smallint(result_type const& min_arg = 0, result_type const& max_arg = 9)
      : _min(min_arg), _max(max_arg) {}

    /**
     * Constructs a @c uniform_smallint from its parameters.
     */
    explicit uniform_smallint(const param_type& parm)
      : _min(parm.a()), _max(parm.b()) {}

    /** Returns the minimum value of the distribution. */
    result_type a() const { return _min; }
    /** Returns the maximum value of the distribution. */
    result_type b() const { return _max; }
    /** Returns the minimum value of the distribution. */
    result_type min BOOST_PREVENT_MACRO_SUBSTITUTION () const { return _min; }
    /** Returns the maximum value of the distribution. */
    result_type max BOOST_PREVENT_MACRO_SUBSTITUTION () const { return _max; }

    /** Returns the parameters of the distribution. */
    param_type param() const { return param_type(_min, _max); }
    /** Sets the parameters of the distribution. */
    void param(const param_type& parm)
    {
        _min = parm.a();
        _max = parm.b();
    }

    /**
     * Effects: Subsequent uses of the distribution do not depend
     * on values produced by any engine prior to invoking reset.
     */
    void reset() { }

    /** Returns a value uniformly distributed in the range [min(), max()]. */
    template<class Engine>
    result_type operator()(Engine& eng) const
    {
        typedef typename Engine::result_type base_result;
        return generate(eng, boost::is_integral<base_result>());
    }

    /** Returns a value uniformly distributed in the range [param.a(), param.b()]. */
    template<class Engine>
    result_type operator()(Engine& eng, const param_type& parm) const
    { return uniform_smallint(parm)(eng); }

    /** Writes the distribution to a @c std::ostream. */
    BOOST_RANDOM_DETAIL_OSTREAM_OPERATOR(os, uniform_smallint, ud)
    {
        os << ud._min << " " << ud._max;
        return os;
    }
    
    /** Reads the distribution from a @c std::istream. */
    BOOST_RANDOM_DETAIL_ISTREAM_OPERATOR(is, uniform_smallint, ud)
    {
        is >> ud._min >> std::ws >> ud._max;
        return is;
    }

    /**
     * Returns true if the two distributions will produce identical
     * sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_EQUALITY_OPERATOR(uniform_smallint, lhs, rhs)
    { return lhs._min == rhs._min && lhs._max == rhs._max; }
    
    /**
     * Returns true if the two distributions may produce different
     * sequences of values given equal generators.
     */
    BOOST_RANDOM_DETAIL_INEQUALITY_OPERATOR(uniform_smallint)

private:
    
    // \cond show_private
    template<class Engine>
    result_type generate(Engine& eng, boost::mpl::true_) const
    {
        // equivalent to (eng() - eng.min()) % (_max - _min + 1) + _min,
        // but guarantees no overflow.
        typedef typename Engine::result_type base_result;
        typedef typename boost::make_unsigned<base_result>::type base_unsigned;
        typedef result_type  range_type;
        range_type range = random::detail::subtract<result_type>()(_max, _min);
        base_unsigned base_range =
            random::detail::subtract<result_type>()((eng.max)(), (eng.min)());
        base_unsigned val =
            random::detail::subtract<base_result>()(eng(), (eng.min)());
        if(range >= base_range) {
            return boost::random::detail::add<range_type, result_type>()(
                static_cast<range_type>(val), _min);
        } else {
            base_unsigned modulus = static_cast<base_unsigned>(range) + 1;
            return boost::random::detail::add<range_type, result_type>()(
                static_cast<range_type>(val % modulus), _min);
        }
    }
    
    template<class Engine>
    result_type generate(Engine& eng, boost::mpl::false_) const
    {
        typedef typename Engine::result_type base_result;
        typedef result_type                  range_type;
        range_type range = random::detail::subtract<result_type>()(_max, _min);
        base_result val = boost::uniform_01<base_result>()(eng);
        // what is the worst that can possibly happen here?
        // base_result may not be able to represent all the values in [0, range]
        // exactly.  If this happens, it will cause round off error and we
        // won't be able to produce all the values in the range.  We don't
        // care about this because the user has already told us not to by
        // using uniform_smallint.  However, we do need to be careful
        // to clamp the result, or floating point rounding can produce
        // an out of range result.
        range_type offset = static_cast<range_type>(val * (range + 1));
        if(offset > range) return _max;
        return boost::random::detail::add<range_type, result_type>()(offset , _min);
    }
    // \endcond

    result_type _min;
    result_type _max;
};


namespace detail{

template<class Backend, boost::multiprecision::expression_template_option ExpressionTemplates>
struct select_uniform_01<boost::multiprecision::number<Backend, ExpressionTemplates> >
{
  template<class RealType>
  struct apply
  {
    typedef new_uniform_01<boost::multiprecision::number<Backend, ExpressionTemplates> > type;
  };
};

template<class Engine, class Backend, boost::multiprecision::expression_template_option ExpressionTemplates>
boost::multiprecision::number<Backend, ExpressionTemplates> 
   generate_uniform_int(
    Engine& eng, const boost::multiprecision::number<Backend, ExpressionTemplates>& min_value, const boost::multiprecision::number<Backend, ExpressionTemplates>& max_value,
    boost::mpl::true_ /** is_integral<Engine::result_type> */)
{
    typedef boost::multiprecision::number<Backend, ExpressionTemplates> result_type;
    // Since we're using big-numbers, use the result type for all internal calculations:
    typedef result_type range_type;
    typedef result_type base_result;
    typedef result_type base_unsigned;
    const range_type range = random::detail::subtract<result_type>()(max_value, min_value);
    const base_result bmin = (eng.min)();
    const base_unsigned brange =
      random::detail::subtract<base_result>()((eng.max)(), (eng.min)());

    if(range == 0) {
      return min_value;    
    } else if(brange == range) {
      // this will probably never happen in real life
      // basically nothing to do; just take care we don't overflow / underflow
      base_unsigned v = random::detail::subtract<base_result>()(eng(), bmin);
      return random::detail::add<base_unsigned, result_type>()(v, min_value);
    } else if(brange < range) {
      // use rejection method to handle things like 0..3 --> 0..4
      for(;;) {
        // concatenate several invocations of the base RNG
        // take extra care to avoid overflows

        //  limit == floor((range+1)/(brange+1))
        //  Therefore limit*(brange+1) <= range+1
        range_type limit;
        if(std::numeric_limits<range_type>::is_bounded && (range == (std::numeric_limits<range_type>::max)())) {
          limit = range/(range_type(brange)+1);
          if(range % (range_type(brange)+1) == range_type(brange))
            ++limit;
        } else {
          limit = (range+1)/(range_type(brange)+1);
        }

        // We consider "result" as expressed to base (brange+1):
        // For every power of (brange+1), we determine a random factor
        range_type result = range_type(0);
        range_type mult = range_type(1);

        // loop invariants:
        //  result < mult
        //  mult <= range
        while(mult <= limit) {
          // Postcondition: result <= range, thus no overflow
          //
          // limit*(brange+1)<=range+1                   def. of limit       (1)
          // eng()-bmin<=brange                          eng() post.         (2)
          // and mult<=limit.                            loop condition      (3)
          // Therefore mult*(eng()-bmin+1)<=range+1      by (1),(2),(3)      (4)
          // Therefore mult*(eng()-bmin)+mult<=range+1   rearranging (4)     (5)
          // result<mult                                 loop invariant      (6)
          // Therefore result+mult*(eng()-bmin)<range+1  by (5), (6)         (7)
          //
          // Postcondition: result < mult*(brange+1)
          //
          // result<mult                                 loop invariant      (1)
          // eng()-bmin<=brange                          eng() post.         (2)
          // Therefore result+mult*(eng()-bmin) <
          //           mult+mult*(eng()-bmin)            by (1)              (3)
          // Therefore result+(eng()-bmin)*mult <
          //           mult+mult*brange                  by (2), (3)         (4)
          // Therefore result+(eng()-bmin)*mult <
          //           mult*(brange+1)                   by (4)
          result += static_cast<range_type>(random::detail::subtract<base_result>()(eng(), bmin) * mult);

          // equivalent to (mult * (brange+1)) == range+1, but avoids overflow.
          if(mult * range_type(brange) == range - mult + 1) {
              // The destination range is an integer power of
              // the generator's range.
              return(result);
          }

          // Postcondition: mult <= range
          // 
          // limit*(brange+1)<=range+1                   def. of limit       (1)
          // mult<=limit                                 loop condition      (2)
          // Therefore mult*(brange+1)<=range+1          by (1), (2)         (3)
          // mult*(brange+1)!=range+1                    preceding if        (4)
          // Therefore mult*(brange+1)<range+1           by (3), (4)         (5)
          // 
          // Postcondition: result < mult
          //
          // See the second postcondition on the change to result. 
          mult *= range_type(brange)+range_type(1);
        }
        // loop postcondition: range/mult < brange+1
        //
        // mult > limit                                  loop condition      (1)
        // Suppose range/mult >= brange+1                Assumption          (2)
        // range >= mult*(brange+1)                      by (2)              (3)
        // range+1 > mult*(brange+1)                     by (3)              (4)
        // range+1 > (limit+1)*(brange+1)                by (1), (4)         (5)
        // (range+1)/(brange+1) > limit+1                by (5)              (6)
        // limit < floor((range+1)/(brange+1))           by (6)              (7)
        // limit==floor((range+1)/(brange+1))            def. of limit       (8)
        // not (2)                                       reductio            (9)
        //
        // loop postcondition: (range/mult)*mult+(mult-1) >= range
        //
        // (range/mult)*mult + range%mult == range       identity            (1)
        // range%mult < mult                             def. of %           (2)
        // (range/mult)*mult+mult > range                by (1), (2)         (3)
        // (range/mult)*mult+(mult-1) >= range           by (3)              (4)
        //
        // Note that the maximum value of result at this point is (mult-1),
        // so after this final step, we generate numbers that can be
        // at least as large as range.  We have to really careful to avoid
        // overflow in this final addition and in the rejection.  Anything
        // that overflows is larger than range and can thus be rejected.

        // range/mult < brange+1  -> no endless loop
        range_type result_increment =
            generate_uniform_int(
                eng,
                static_cast<range_type>(0),
                static_cast<range_type>(range/mult),
                boost::mpl::true_());
        if(std::numeric_limits<range_type>::is_bounded && ((std::numeric_limits<range_type>::max)() / mult < result_increment)) {
          // The multiplication would overflow.  Reject immediately.
          continue;
        }
        result_increment *= mult;
        // unsigned integers are guaranteed to wrap on overflow.
        result += result_increment;
        if(result < result_increment) {
          // The addition overflowed.  Reject.
          continue;
        }
        if(result > range) {
          // Too big.  Reject.
          continue;
        }
        return random::detail::add<range_type, result_type>()(result, min_value);
      }
    } else {                   // brange > range
      range_type bucket_size;
      // it's safe to add 1 to range, as long as we cast it first,
      // because we know that it is less than brange.  However,
      // we do need to be careful not to cause overflow by adding 1
      // to brange.
      if(std::numeric_limits<base_unsigned>::is_bounded && (brange == (std::numeric_limits<base_unsigned>::max)())) {
        bucket_size = brange / (range+1);
        if(brange % (range+1) == range) {
          ++bucket_size;
        }
      } else {
        bucket_size = (brange+1) / (range+1);
      }
      for(;;) {
        range_type result =
          random::detail::subtract<base_result>()(eng(), bmin);
        result /= bucket_size;
        // result and range are non-negative, and result is possibly larger
        // than range, so the cast is safe
        if(result <= range)
          return result + min_value;
      }
    }
}

template<class Engine, class Backend, boost::multiprecision::expression_template_option ExpressionTemplates>
inline boost::multiprecision::number<Backend, ExpressionTemplates> 
   generate_uniform_int(Engine& eng, const boost::multiprecision::number<Backend, ExpressionTemplates>& min_value, const boost::multiprecision::number<Backend, ExpressionTemplates>& max_value)
{
    typedef typename Engine::result_type base_result;
    typedef typename mpl::or_<boost::is_integral<base_result>, mpl::bool_<boost::multiprecision::number_category<Backend>::value == boost::multiprecision::number_kind_integer> >::type tag_type;
    return generate_uniform_int(eng, min_value, max_value,
        tag_type());
}

} // detail


}} // namespaces

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif

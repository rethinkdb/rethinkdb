// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_MEASUREMENT_HPP
#define BOOST_UNITS_MEASUREMENT_HPP

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>

#include <boost/io/ios_state.hpp>
#include <boost/units/static_rational.hpp>

namespace boost {

namespace units {

namespace sqr_namespace /**/ {

template<class Y>
Y sqr(Y val)
{ return val*val; }

} // namespace

using sqr_namespace::sqr;

template<class Y>
class measurement
{    
    public:
        typedef measurement<Y>                  this_type;
        typedef Y                               value_type;
        
        measurement(const value_type& val = value_type(),
                    const value_type& err = value_type()) : 
            value_(val),
            uncertainty_(std::abs(err)) 
        { }
        
        measurement(const this_type& source) : 
            value_(source.value_),
            uncertainty_(source.uncertainty_) 
        { }
        
        //~measurement() { }
        
        this_type& operator=(const this_type& source)
        {
            if (this == &source) return *this;
            
            value_ = source.value_;
            uncertainty_ = source.uncertainty_;
            
            return *this;
        }
        
        operator value_type() const    { return value_; }
        
        value_type value() const       { return value_; }
        value_type uncertainty() const { return uncertainty_; }
        value_type lower_bound() const { return value_-uncertainty_; }
        value_type upper_bound() const { return value_+uncertainty_; }
        
        this_type& operator+=(const value_type& val)            
        { 
            value_ += val; 
            return *this; 
        }
        
        this_type& operator-=(const value_type& val)            
        { 
            value_ -= val; 
            return *this; 
        }
        
        this_type& operator*=(const value_type& val)            
        { 
            value_ *= val; 
            uncertainty_ *= val; 
            return *this; 
        }
        
        this_type& operator/=(const value_type& val)            
        { 
            value_ /= val; 
            uncertainty_ /= val; 
            return *this; 
        }
        
        this_type& operator+=(const this_type& /*source*/);
        this_type& operator-=(const this_type& /*source*/);        
        this_type& operator*=(const this_type& /*source*/);        
        this_type& operator/=(const this_type& /*source*/);

    private:
        value_type          value_,
                            uncertainty_;
};

}

}

#if BOOST_UNITS_HAS_BOOST_TYPEOF

BOOST_TYPEOF_REGISTER_TEMPLATE(boost::units::measurement, 1)

#endif

namespace boost {

namespace units {

template<class Y>
inline
measurement<Y>&
measurement<Y>::operator+=(const this_type& source)
{
    uncertainty_ = std::sqrt(sqr(uncertainty_)+sqr(source.uncertainty_));
    value_ += source.value_;
    
    return *this;
}

template<class Y>
inline
measurement<Y>&
measurement<Y>::operator-=(const this_type& source)
{
    uncertainty_ = std::sqrt(sqr(uncertainty_)+sqr(source.uncertainty_));
    value_ -= source.value_;
    
    return *this;
}

template<class Y>
inline
measurement<Y>&
measurement<Y>::operator*=(const this_type& source)
{
    uncertainty_ = (value_*source.value_)*
              std::sqrt(sqr(uncertainty_/value_)+
                        sqr(source.uncertainty_/source.value_));
    value_ *= source.value_;
    
    return *this;
}

template<class Y>
inline
measurement<Y>&
measurement<Y>::operator/=(const this_type& source)
{
    uncertainty_ = (value_/source.value_)*
              std::sqrt(sqr(uncertainty_/value_)+
                        sqr(source.uncertainty_/source.value_));
    value_ /= source.value_;
    
    return *this;
}

// value_type op measurement
template<class Y>
inline
measurement<Y>
operator+(Y lhs,const measurement<Y>& rhs)
{
    return (measurement<Y>(lhs,Y(0))+=rhs);
}

template<class Y>
inline
measurement<Y>
operator-(Y lhs,const measurement<Y>& rhs)
{
    return (measurement<Y>(lhs,Y(0))-=rhs);
}

template<class Y>
inline
measurement<Y>
operator*(Y lhs,const measurement<Y>& rhs)
{
    return (measurement<Y>(lhs,Y(0))*=rhs);
}

template<class Y>
inline
measurement<Y>
operator/(Y lhs,const measurement<Y>& rhs)
{
    return (measurement<Y>(lhs,Y(0))/=rhs);
}

// measurement op value_type
template<class Y>
inline
measurement<Y>
operator+(const measurement<Y>& lhs,Y rhs)
{
    return (measurement<Y>(lhs)+=measurement<Y>(rhs,Y(0)));
}

template<class Y>
inline
measurement<Y>
operator-(const measurement<Y>& lhs,Y rhs)
{
    return (measurement<Y>(lhs)-=measurement<Y>(rhs,Y(0)));
}

template<class Y>
inline
measurement<Y>
operator*(const measurement<Y>& lhs,Y rhs)
{
    return (measurement<Y>(lhs)*=measurement<Y>(rhs,Y(0)));
}

template<class Y>
inline
measurement<Y>
operator/(const measurement<Y>& lhs,Y rhs)
{
    return (measurement<Y>(lhs)/=measurement<Y>(rhs,Y(0)));
}

// measurement op measurement
template<class Y>
inline
measurement<Y>
operator+(const measurement<Y>& lhs,const measurement<Y>& rhs)
{
    return (measurement<Y>(lhs)+=rhs);
}

template<class Y>
inline
measurement<Y>
operator-(const measurement<Y>& lhs,const measurement<Y>& rhs)
{
    return (measurement<Y>(lhs)-=rhs);
}

template<class Y>
inline
measurement<Y>
operator*(const measurement<Y>& lhs,const measurement<Y>& rhs)
{
    return (measurement<Y>(lhs)*=rhs);
}

template<class Y>
inline
measurement<Y>
operator/(const measurement<Y>& lhs,const measurement<Y>& rhs)
{
    return (measurement<Y>(lhs)/=rhs);
}

/// specialize power typeof helper
template<class Y,long N,long D> 
struct power_typeof_helper<measurement<Y>,static_rational<N,D> >
{ 
    typedef measurement<
        typename power_typeof_helper<Y,static_rational<N,D> >::type
    > type; 
    
    static type value(const measurement<Y>& x)  
    { 
        const static_rational<N,D>  rat;

        const Y m = Y(rat.numerator())/Y(rat.denominator()),
                newval = std::pow(x.value(),m),
                err = newval*std::sqrt(std::pow(m*x.uncertainty()/x.value(),2));
        
        return type(newval,err);
    }
};

/// specialize root typeof helper
template<class Y,long N,long D> 
struct root_typeof_helper<measurement<Y>,static_rational<N,D> >                
{ 
    typedef measurement<
        typename root_typeof_helper<Y,static_rational<N,D> >::type
    > type; 
    
    static type value(const measurement<Y>& x)  
    { 
        const static_rational<N,D>  rat;

        const Y m = Y(rat.denominator())/Y(rat.numerator()),
                newval = std::pow(x.value(),m),
                err = newval*std::sqrt(std::pow(m*x.uncertainty()/x.value(),2));
        
        return type(newval,err);
    }
};

// stream output
template<class Y>
inline
std::ostream& operator<<(std::ostream& os,const measurement<Y>& val)
{
    boost::io::ios_precision_saver precision_saver(os);
    boost::io::ios_flags_saver flags_saver(os);
    
    os << val.value() << "(+/-" << val.uncertainty() << ")";
    
    return os;
}

} // namespace units

} // namespace boost

#endif // BOOST_UNITS_MEASUREMENT_HPP

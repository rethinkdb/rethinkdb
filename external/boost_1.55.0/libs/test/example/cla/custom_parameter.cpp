//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Runtime.Param
#include <boost/test/utils/runtime/cla/named_parameter.hpp>
#include <boost/test/utils/runtime/cla/parser.hpp>

namespace rt  = boost::runtime;
namespace cla = boost::runtime::cla;

// STL
#include <iostream>
#include <iterator>

//_____________________________________________________________________//

struct Point : std::pair<int,int> {
    bool parse( rt::cstring& in ) {
        in.trim_left();

        if( first_char( in ) != '(' )
            return false;

        in.trim_left( 1 );
        rt::cstring::size_type pos = in.find( ")" );

        if( pos == rt::cstring::npos )
            return false;
        
        rt::cstring ss( in.begin(), pos );
        pos = ss.find( "," );

        if( pos == rt::cstring::npos )
            return false;

        rt::cstring f( ss.begin(), pos );
        rt::cstring s( ss.begin()+pos+1, ss.end() );

        f.trim();
        s.trim();

        try {
            first  = boost::lexical_cast<int>( f );
            second = boost::lexical_cast<int>( s );
        }
        catch( boost::bad_lexical_cast const& ) {
            return false;
        }

        in.trim_left( ss.end()+1 );
        return true;
    }
};

std::ostream& operator<<( std::ostream& ostr, Point const& p )
{
    ostr << '(' << p.first << ',' << p.second << ')';

    return ostr;
}

struct Segment : std::pair<Point,Point> {
    bool parse( rt::cstring& in ) {
        in.trim_left();

        if( first_char( in ) != '[' )
            return false;

        in.trim_left( 1 );

        if( !first.parse( in ) )
            return false;

        in.trim_left();

        if( first_char( in ) != ',' )
            return false;

        in.trim_left( 1 );

        if( !second.parse( in ) )
            return false;

        in.trim_left();

        if( first_char( in ) != ']' )
            return false;

        in.trim_left( 1 );

        return true;
    }
};

std::ostream& operator<<( std::ostream& ostr, Segment const& p )
{
    ostr << '[' << p.first << ',' << p.second << ']';

    return ostr;
}

struct Circle : std::pair<Point,int> {
    bool parse( rt::cstring& in ) {
        in.trim_left();

        if( first_char( in ) != '[' )
            return false;

        in.trim_left( 1 );

        if( !first.parse( in ) )
            return false;

        in.trim_left();

        if( first_char( in ) != ','  )
            return false;

        in.trim_left( 1 );

        rt::cstring::size_type pos = in.find( "]" );

        if( pos == rt::cstring::npos )
            return false;

        rt::cstring ss( in.begin(), pos );
        ss.trim();

        try {
            second = boost::lexical_cast<int>( ss );
        }
        catch( boost::bad_lexical_cast const& ) {
            return false;
        }

        in.trim_left( pos+1 );

        return true;
    }
};

std::ostream& operator<<( std::ostream& ostr, Circle const& p )
{
    ostr << '[' << p.first << ',' << p.second << ']';

    return ostr;
}

//_____________________________________________________________________//

template<typename T>
class ShapeIdPolicy : public cla::identification_policy {
    rt::cstring m_name;
    rt::cstring m_usage_str;
public:
    explicit ShapeIdPolicy( rt::cstring name )
    : cla::identification_policy( boost::rtti::type_id<ShapeIdPolicy<T> >() )
    , m_name( name ) {}

    virtual bool        responds_to( rt::cstring name ) const       { return m_name == name; }
    virtual bool        conflict_with( cla::identification_policy const& ) const { return false; }
    virtual rt::cstring id_2_report() const                         { return m_name; }
    virtual void        usage_info( rt::format_stream& fs ) const   { fs << m_name; }

    virtual bool        matching( cla::parameter const& p, cla::argv_traverser& tr, bool primary ) const
    {
        T s;

        rt::cstring in = tr.input();
        return s.parse( in );
    }
};

//_____________________________________________________________________//

template<typename T>
class ShapeArgumentFactory : public cla::argument_factory {
    rt::cstring m_usage_str;
public:
    explicit ShapeArgumentFactory( rt::cstring usage ) : m_usage_str( usage ) {}

    // Argument factory interface
    virtual rt::argument_ptr    produce_using( cla::parameter& p, cla::argv_traverser& tr )
    {
        T s;

        rt::cstring in = tr.input();
        s.parse( in );
        tr.trim( in.begin() - tr.input().begin() );

        if( !p.actual_argument() ) {
            rt::argument_ptr   res;

            rt::typed_argument<std::list<T> >* new_arg = new rt::typed_argument<std::list<T> >( p );

            new_arg->p_value.value.push_back( s );
            res.reset( new_arg );

            return res;
        }
        else {
            std::list<T>& arg_values = rt::arg_value<std::list<T> >( *p.actual_argument() );
            arg_values.push_back( s );

            return p.actual_argument();
        }
    }
    virtual rt::argument_ptr    produce_using( cla::parameter& p, cla::parser const& ) { return rt::argument_ptr(); }
    virtual void                argument_usage_info( rt::format_stream& fs ) { fs << m_usage_str; }
};

//_____________________________________________________________________//

struct SegmentParam : cla::parameter {
    SegmentParam()
    : cla::parameter( m_id_policy, m_arg_factory )
    , m_id_policy( "segment" )
    , m_arg_factory( ":((P1x,P1y), (P2x,P2y)) ... ((P1x,P1y), (P2x,P2y))" )
    {}

    ShapeIdPolicy<Segment>          m_id_policy;
    ShapeArgumentFactory<Segment>   m_arg_factory;
};

inline boost::shared_ptr<SegmentParam>
segment_param() { return boost::shared_ptr<SegmentParam>( new SegmentParam ); }

//_____________________________________________________________________//

struct CircleParam : cla::parameter {
    CircleParam()
        : cla::parameter( m_id_policy, m_arg_factory )
        , m_id_policy( "circle" )
        , m_arg_factory( ":((P1x,P1y), R) ... ((P1x,P1y), R)" )
    {}

    ShapeIdPolicy<Circle>          m_id_policy;
    ShapeArgumentFactory<Circle>   m_arg_factory;
};

inline boost::shared_ptr<CircleParam>
circle_param() { return boost::shared_ptr<CircleParam>( new CircleParam ); }

//_____________________________________________________________________//

int main() {
    char* argv[] = { "basic", "[(1,", "1)", ",", "(7,", "-1", ")]", "[(", "1,1)", ",7", "]", "[(3,", "1", ")", ",", "2]",
    "[", "(2,7", "),", "(5", ",1", ")]" };
    int argc = sizeof(argv)/sizeof(char*);

    try {
        cla::parser P;

        P << circle_param()  - cla::optional
          << segment_param() - cla::optional;

        P.parse( argc, argv );

        boost::optional<std::list<Segment> > segments;
        boost::optional<std::list<Circle> > circles;

        P.get( "segment", segments );

        if( segments ) {
            std::cout << "segments : ";
            std::copy( segments->begin(), segments->end(), std::ostream_iterator<Segment>( std::cout, "; " ) );
            std::cout << std::endl;
        }

        P.get( "circle", circles );

        if( circles ) {
            std::cout << "circles : ";
            std::copy( circles->begin(), circles->end(), std::ostream_iterator<Circle>( std::cout, "; " ) );
            std::cout << std::endl;
        }
    }
    catch( rt::logic_error const& ex ) {
        std::cout << "Logic error: " << ex.msg() << std::endl;
        return -1;
    }

    return 0;
}

// EOF

#ifndef JSON_SPIRIT_WRITER_TEMPLATE
#define JSON_SPIRIT_WRITER_TEMPLATE

//          Copyright John W. Wilkinson 2007 - 2011
// Distributed under the MIT License, see accompanying file LICENSE.txt

// json spirit version 4.04

#include "json_spirit_value.h"

#include <cassert>
#include <sstream>
#include <iomanip>
#include <boost/io/ios_state.hpp>

namespace json_spirit
{
    enum Output_options{ pretty_print = 0x01,   // Add whitespace to format the output nicely.

                         raw_utf8 = 0x02,       // This prevents non-printable characters from being escapted using "\uNNNN" notation.
                                                // Note, this is an extension to the JSON standard. It disables the escaping of
                                                // non-printable characters allowing UTF-8 sequences held in 8 bit char strings
                                                // to pass through unaltered.

                         remove_trailing_zeros = 0x04
                                                // outputs e.g. "1.200000000000000" as "1.2"
                       };

    inline char to_hex_char( unsigned int c )
    {
        assert( c <= 0xF );

        const char ch = static_cast< char >( c );

        if( ch < 10 ) return '0' + ch;

        return 'A' - 10 + ch;
    }

    template< class String_type >
    String_type non_printable_to_string( unsigned int c )
    {
        typedef typename String_type::value_type Char_type;

        String_type result( 6, '\\' );

        result[1] = 'u';

        result[ 5 ] = to_hex_char( c & 0x000F ); c >>= 4;
        result[ 4 ] = to_hex_char( c & 0x000F ); c >>= 4;
        result[ 3 ] = to_hex_char( c & 0x000F ); c >>= 4;
        result[ 2 ] = to_hex_char( c & 0x000F );

        return result;
    }

    template< typename Char_type, class String_type >
    bool add_esc_char( Char_type c, String_type& s )
    {
        switch( c )
        {
            case '"':  s += to_str< String_type >( "\\\"" ); return true;
            case '\\': s += to_str< String_type >( "\\\\" ); return true;
            case '\b': s += to_str< String_type >( "\\b"  ); return true;
            case '\f': s += to_str< String_type >( "\\f"  ); return true;
            case '\n': s += to_str< String_type >( "\\n"  ); return true;
            case '\r': s += to_str< String_type >( "\\r"  ); return true;
            case '\t': s += to_str< String_type >( "\\t"  ); return true;
        }

        return false;
    }

    template< class String_type >
    String_type add_esc_chars( const String_type& s, bool raw_utf8 )
    {
        typedef typename String_type::const_iterator Iter_type;
        typedef typename String_type::value_type     Char_type;

        String_type result;

        const Iter_type end( s.end() );

        for( Iter_type i = s.begin(); i != end; ++i )
        {
            const Char_type c( *i );

            if( add_esc_char( c, result ) ) continue;

            if( raw_utf8 )
            {
                result += c;
            }
            else
            {
                const wint_t unsigned_c( ( c >= 0 ) ? c : 256 + c );

                if( iswprint( unsigned_c ) )
                {
                    result += c;
                }
                else
                {
                    result += non_printable_to_string< String_type >( unsigned_c );
                }
            }
        }

        return result;
    }

    template< class Ostream >
    void append_double( Ostream& os, const double d, const int precision )
    {
        os << std::showpoint << std::setprecision( precision ) << d;
    }

    template< class String_type >
    void erase_and_extract_exponent( String_type& str, String_type& exp )
    {
        const typename String_type::size_type exp_start= str.find( 'e' );

        if( exp_start != String_type::npos )
        {
            exp = str.substr( exp_start );
            str.erase( exp_start );
        }
    }

    template< class String_type >
    typename String_type::size_type find_first_non_zero( const String_type& str )
    {
        typename String_type::size_type result = str.size() - 1;

        for( ; result != 0; --result )
        {
            if( str[ result ] != '0' )
            {
                break;
            }
        }

        return result;
    }

    template< class String_type >
    void remove_trailing( String_type& str )
    {
        String_type exp;

        erase_and_extract_exponent( str, exp );

        const typename String_type::size_type first_non_zero = find_first_non_zero( str );

        if( first_non_zero != 0 )
        {
            str.erase( first_non_zero + 1 );
        }

        str += exp;
    }

    // this class generates the JSON text,
    // it keeps track of the indentation level etc.
    //
    template< class Value_type, class Ostream_type >
    class Generator
    {
        typedef typename Value_type::Config_type Config_type;
        typedef typename Config_type::String_type String_type;
        typedef typename Config_type::Object_type Object_type;
        typedef typename Config_type::Array_type Array_type;
        typedef typename String_type::value_type Char_type;
        typedef typename Object_type::value_type Obj_member_type;

    public:

        Generator( const Value_type& value, Ostream_type& os, unsigned int options )
        :   os_( os )
        ,   indentation_level_( 0 )
        ,   pretty_( ( options & pretty_print ) != 0 )
        ,   raw_utf8_( ( options & raw_utf8 ) != 0 )
        ,   remove_trailing_zeros_( ( options & remove_trailing_zeros ) != 0 )
        ,   ios_saver_( os )
        {
            output( value );
        }

    private:

        void output( const Value_type& value )
        {
            switch( value.type() )
            {
                case obj_type:   output( value.get_obj() );   break;
                case array_type: output( value.get_array() ); break;
                case str_type:   output( value.get_str() );   break;
                case bool_type:  output( value.get_bool() );  break;
                case real_type:  output( value.get_real() );  break;
                case int_type:   output_int( value );         break;
                case null_type:  os_ << "null";               break;
                default: assert( false );
            }
        }

        void output( const Object_type& obj )
        {
            output_array_or_obj( obj, '{', '}' );
        }

        void output( const Array_type& arr )
        {
            output_array_or_obj( arr, '[', ']' );
        }

        void output( const Obj_member_type& member )
        {
            output( Config_type::get_name( member ) ); space(); 
            os_ << ':'; space(); 
            output( Config_type::get_value( member ) );
        }

        void output_int( const Value_type& value )
        {
            if( value.is_uint64() )
            {
                os_ << value.get_uint64();
            }
            else
            {
               os_ << value.get_int64();
            }
        }

        void output( const String_type& s )
        {
            os_ << '"' << add_esc_chars( s, raw_utf8_ ) << '"';
        }

        void output( bool b )
        {
            os_ << to_str< String_type >( b ? "true" : "false" );
        }

        void output( double d )
        {
            if( remove_trailing_zeros_ )
            {
                std::basic_ostringstream< Char_type > os;

                append_double( os, d, 16 );  // note precision is 16 so that we get some trailing space that we can remove,
                                             // otherwise, 0.1234 gets converted to "0.12399999..."

                String_type str = os.str();

                remove_trailing( str );

                os_ << str;
            }
            else
            {
                append_double( os_, d, 17 );
            }
        }

        template< class T >
        void output_array_or_obj( const T& t, Char_type start_char, Char_type end_char )
        {
            os_ << start_char; new_line();

            ++indentation_level_;
            
            for( typename T::const_iterator i = t.begin(); i != t.end(); ++i )
            {
                indent(); output( *i );

                typename T::const_iterator next = i;

                if( ++next != t.end())
                {
                    os_ << ',';
                }

                new_line();
            }

            --indentation_level_;

            indent(); os_ << end_char;
        }
        
        void indent()
        {
            if( !pretty_ ) return;

            for( int i = 0; i < indentation_level_; ++i )
            { 
                os_ << "    ";
            }
        }

        void space()
        {
            if( pretty_ ) os_ << ' ';
        }

        void new_line()
        {
            if( pretty_ ) os_ << '\n';
        }

        Generator& operator=( const Generator& ); // to prevent "assignment operator could not be generated" warning

        Ostream_type& os_;
        int indentation_level_;
        bool pretty_;
        bool raw_utf8_;
        bool remove_trailing_zeros_;
        boost::io::basic_ios_all_saver< Char_type > ios_saver_;  // so that ostream state is reset after control is returned to the caller
    };

    // writes JSON Value to a stream, e.g.
    //
    // write_stream( value, os, pretty_print );
    //
    template< class Value_type, class Ostream_type >
    void write_stream( const Value_type& value, Ostream_type& os, unsigned int options = 0 )
    {
        os << std::dec;
        Generator< Value_type, Ostream_type >( value, os, options );
    }

    // writes JSON Value to a stream, e.g.
    //
    // const string json_str = write( value, pretty_print );
    //
    template< class Value_type >
    typename Value_type::String_type write_string( const Value_type& value, unsigned int options = 0 )
    {
        typedef typename Value_type::String_type::value_type Char_type;

        std::basic_ostringstream< Char_type > os;

        write_stream( value, os, options );

        return os.str();
    }
}

#endif

/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    A generic C++ lexer token definition
    
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(SLEX_TOKEN_HPP_53A13BD2_FBAA_444B_9B8B_FCB225C2BBA8_INCLUDED)
#define SLEX_TOKEN_HPP_53A13BD2_FBAA_444B_9B8B_FCB225C2BBA8_INCLUDED

#include <iomanip>
#include <ios>

#include <boost/wave/wave_config.hpp>
#include <boost/wave/token_ids.hpp>  
#include <boost/wave/language_support.hpp>
#include <boost/wave/util/file_position.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace cpplexer {

///////////////////////////////////////////////////////////////////////////////
//  forward declaration of the token type
template <typename PositionT = boost::wave::util::file_position_type>
class slex_token;

///////////////////////////////////////////////////////////////////////////////
//  
//  lex_token
//
///////////////////////////////////////////////////////////////////////////////

template <typename PositionT>
class slex_token 
{
public:
    typedef BOOST_WAVE_STRINGTYPE   string_type;
    typedef PositionT               position_type;

    slex_token()
    :   id(T_EOI)
    {}

    //  construct an invalid token
    explicit slex_token(int)
    :   id(T_UNKNOWN)
    {}

    slex_token(token_id id_, string_type const &value_, PositionT const &pos_)
    :   id(id_), value(value_), pos(pos_)
    {}

// accessors
    operator token_id() const { return id; }
    string_type const &get_value() const { return value; }
    position_type const &get_position() const { return pos; }
    bool is_eoi() const { return id == T_EOI; }
    bool is_valid() const { return id != T_UNKNOWN; }

    void set_token_id (token_id id_) { id = id_; }
    void set_value (string_type const &newval) { value = newval; }
    void set_position (position_type const &pos_) { pos = pos_; }

    friend bool operator== (slex_token const& lhs, slex_token const& rhs)
    {
        //  two tokens are considered equal even if they contain different 
        //  positions
        return (lhs.id == rhs.id && lhs.value == rhs.value) ? true : false;
    }

// debug support    
#if BOOST_WAVE_DUMP_PARSE_TREE != 0
// access functions for the tree_to_xml functionality
    static int get_token_id(slex_token const &t) 
        { return ID_FROM_TOKEN(token_id(t)); }
    static string_type get_token_value(slex_token const &t) 
        { return t.get_value(); }
#endif 
    
// print support
    void print (std::ostream &stream) const
    {
        using namespace std;
        using namespace boost::wave;
        
        stream << std::setw(16) 
            << std::left << boost::wave::get_token_name(id) << " ("
            << "#" << token_id(BASEID_FROM_TOKEN(*this)) 
            << ") at " << get_position().get_file() << " (" 
            << std::setw(3) << std::right << get_position().get_line() << "/" 
            << std::setw(2) << std::right << get_position().get_column() 
            << "): >";
            
        for (std::size_t i = 0; i < value.size(); ++i) {
            switch (value[i]) {
            case '\r':  stream << "\\r"; break;
            case '\n':  stream << "\\n"; break;
            case '\t':  stream << "\\t"; break;
            default:
                stream << value[i]; 
                break;
            }
        }
        stream << "<";
    }

private:
    boost::wave::token_id id;   // the token id
    string_type value;             // the text, which was parsed into this token
    PositionT pos;              // the original file position
};

template <typename PositionT>
inline std::ostream &
operator<< (std::ostream &stream, slex_token<PositionT> const &object)
{
    object.print(stream);
    return stream;
}

///////////////////////////////////////////////////////////////////////////////
//  This overload is needed by the multi_pass/functor_input_policy to 
//  validate a token instance. It has to be defined in the same namespace 
//  as the token class itself to allow ADL to find it.
///////////////////////////////////////////////////////////////////////////////
template <typename Position>
inline bool 
token_is_valid(slex_token<Position> const& t)
{
    return t.is_valid();
}

///////////////////////////////////////////////////////////////////////////////
}   // namespace cpplexer
}   // namespace wave
}   // namespace boost

#endif // !defined(SLEX_TOKEN_HPP_53A13BD2_FBAA_444B_9B8B_FCB225C2BBA8_INCLUDED)

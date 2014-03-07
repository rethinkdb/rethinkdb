/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    A C++ lexer token definition for the real_positions example
    
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(REAL_POSITION_TOKEN_HPP_HK_061109_INCLUDED)
#define REAL_POSITION_TOKEN_HPP_HK_061109_INCLUDED

#include <boost/wave/wave_config.hpp>
#include <boost/wave/util/file_position.hpp>
#include <boost/wave/token_ids.hpp>  
#include <boost/wave/language_support.hpp>
#include <boost/detail/atomic_count.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace impl {

template <typename StringTypeT, typename PositionT>
class token_data
{
public:
    typedef StringTypeT string_type;
    typedef PositionT   position_type;

    token_data()
    :   id(boost::wave::T_EOI), refcnt(1)
    {}

    //  construct an invalid token
    explicit token_data(int)
    :   id(T_UNKNOWN), refcnt(1)
    {}

    token_data(boost::wave::token_id id_, string_type const &value_, 
            position_type const &pos_)
    :   id(id_), value(value_), pos(pos_), corrected_pos(pos_), refcnt(1)
    {}

    token_data(token_data const& rhs)
    :   id(rhs.id), value(rhs.value), pos(rhs.pos), 
        corrected_pos(rhs.corrected_pos), refcnt(1)
    {}

    ~token_data()
    {}

    std::size_t addref() { return ++refcnt; }
    std::size_t release() { return --refcnt; }
    std::size_t get_refcnt() const { return refcnt; }

// accessors
    operator boost::wave::token_id() const { return id; }
    string_type const &get_value() const { return value; }
    position_type const &get_position() const { return pos; }
    position_type const &get_corrected_position() const 
        { return corrected_pos; }
    bool is_eoi() const { id == T_EOI; }

    void set_token_id (boost::wave::token_id id_) { id = id_; }
    void set_value (string_type const &value_) { value = value_; }
    void set_position (position_type const &pos_) { pos = pos_; }
    void set_corrected_position (position_type const &pos_) 
        { corrected_pos = pos_; }

    friend bool operator== (token_data const& lhs, token_data const& rhs)
    {
        //  two tokens are considered equal even if they refer to different 
        //  positions
        return (lhs.id == rhs.id && lhs.value == rhs.value) ? true : false;
    }

private:
    boost::wave::token_id id;     // the token id
    string_type value;            // the text, which was parsed into this token
    position_type pos;            // the original file position
    position_type corrected_pos;  // the original file position
    boost::detail::atomic_count refcnt;
};

///////////////////////////////////////////////////////////////////////////////
} // namespace impl

///////////////////////////////////////////////////////////////////////////////
//  forward declaration of the token type
template <typename PositionT = boost::wave::util::file_position_type>
class lex_token;

///////////////////////////////////////////////////////////////////////////////
//  
//  lex_token
//
///////////////////////////////////////////////////////////////////////////////

template <typename PositionT>
class lex_token 
{
public:
    typedef BOOST_WAVE_STRINGTYPE   string_type;
    typedef PositionT               position_type;

    lex_token()
    :   data(new impl::token_data<string_type, position_type>())
    {}

    //  construct an invalid token
    explicit lex_token(int)
    :   data(new data_type(0))
    {}

    lex_token(lex_token const& rhs)
    :   data(rhs.data)
    {
        data->addref();
    }

    lex_token(boost::wave::token_id id_, string_type const &value_, 
            PositionT const &pos_)
    :   data(new impl::token_data<string_type, position_type>(id_, value_, pos_))
    {}

    ~lex_token()
    {
        if (0 == data->release()) 
            delete data;
        data = 0;
    }

    lex_token& operator=(lex_token const& rhs)
    {
        if (&rhs != this) {
            if (0 == data->release()) 
                delete data;
            
            data = rhs.data;
            data->addref();
        }
        return *this;
    }

// accessors
    operator boost::wave::token_id() const 
        { return boost::wave::token_id(*data); }
    string_type const &get_value() const 
        { return data->get_value(); }
    position_type const &get_position() const 
        { return data->get_position(); }
    position_type const &get_corrected_position() const 
        { return data->get_corrected_position(); }
    bool is_valid() const { return 0 != data && token_id(*data) != T_UNKNOWN; }

    void set_token_id (boost::wave::token_id id_) 
        { make_unique(); data->set_token_id(id_); }
    void set_value (string_type const &value_) 
        { make_unique(); data->set_value(value_); }
    void set_position (position_type const &pos_) 
        { make_unique(); data->set_position(pos_); }
    void set_corrected_position (position_type const &pos_) 
        { make_unique(); data->set_corrected_position(pos_); }

    friend bool operator== (lex_token const& lhs, lex_token const& rhs)
    {
        return *(lhs.data) == *(rhs.data);
    }

// debug support
#if BOOST_WAVE_DUMP_PARSE_TREE != 0
// access functions for the tree_to_xml functionality
    static int get_token_id(lex_token const &t) 
        { return token_id(t); }
    static string_type get_token_value(lex_token const &t) 
        { return t.get_value(); }
#endif 

private:
    // make a unique copy of the current object
    void make_unique()
    {
        if (1 == data->get_refcnt())
            return;

        impl::token_data<string_type, position_type> *newdata = 
            new impl::token_data<string_type, position_type>(*data);

        data->release();          // release this reference, can't get zero 
        data = newdata;
    }

    impl::token_data<string_type, position_type> *data;
};

///////////////////////////////////////////////////////////////////////////////
//  This overload is needed by the multi_pass/functor_input_policy to 
//  validate a token instance. It has to be defined in the same namespace 
//  as the token class itself to allow ADL to find it.
///////////////////////////////////////////////////////////////////////////////
template <typename Position>
inline bool 
token_is_valid(lex_token<Position> const& t)
{
    return t.is_valid();
}

#endif // !defined(REAL_POSITION_TOKEN_HPP_HK_061109_INCLUDED)

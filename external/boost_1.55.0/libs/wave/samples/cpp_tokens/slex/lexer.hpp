/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Spirit based lexer
    
    http://www.boost.org/

    Copyright (c) 2001, Daniel C. Nuffer.
    Copyright (c) 2001-2012 Hartmut Kaiser. 
    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    TODO List:
        X callback objects (called when a match is made.)
        X    callback passed first & last iterator, and
                a reference to a lexercontrol object that supports some
                operations on the lexer.
                    set state
                    terminate
                    state stack (push, pop, top)
                    set new token return value
                    ignore the current token
                    yymore
                    get length of matched token
                    get current lexer state
        X DFA serialization to save recomputing the DFA.

        lexer states.
        organize the file into hpp and ipp. arrange the classes in a logical order.
        use buffering - iterator only needs be an input iterator,
           lexer & callback are not templatized on iterator type, but char type.
           next_token is templatized on iterator type.
        beginning/ending contexts.
        ^ and $
        DFA minimization.
        DFA table compression.

=============================================================================*/
#ifndef BOOST_SPIRIT_LEXER_HPP
#define BOOST_SPIRIT_LEXER_HPP

///////////////////////////////////////////////////////////////////////////////
#include <boost/throw_exception.hpp>

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_symbols.hpp>
#include <boost/spirit/include/classic_chset.hpp>
#include <boost/spirit/include/classic_escape_char.hpp>

#include <set>
#include <map>
#include <vector>
#include <stack>
#include <utility> // for pair
#include <iostream>
#include <fstream>
#include <boost/assert.hpp>
#include <boost/limits.hpp>

#if defined(BOOST_NO_STD_ITERATOR_TRAITS)
#define BOOST_SPIRIT_IT_NS impl
#else
#define BOOST_SPIRIT_IT_NS std
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace spirit {
namespace classic {

typedef unsigned char uchar;
typedef unsigned int node_id_t;
const node_id_t invalid_node = node_id_t(-1);
typedef std::set<node_id_t> node_set;
typedef std::vector<uchar> uchar_vector;
typedef std::map<node_id_t, node_set> followpos_t;
typedef std::vector<uchar_vector> state_match_t;

template <typename TokenT>
class lexer_control;

class bad_regex : public std::exception
{
};

namespace lexerimpl
{

class node
{
public:

    virtual ~node() {}

    virtual node* clone() const = 0;
    virtual bool nullable() const = 0;
    virtual node_set firstpos() const = 0;
    virtual node_set lastpos() const = 0;
    virtual void compute_followpos(followpos_t& followpos) const = 0;
    virtual void compute_state_match(state_match_t& state_match) const = 0;
    virtual void get_eof_ids(node_set& eof_set) const = 0;
    virtual void assign_node_ids(node_id_t& node_count) = 0;
#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
    virtual void dump(std::ostream& out) const = 0;
#endif

};

class char_node : public node
{

public:

    char_node(const uchar c);
    char_node(const char_node& x);
    virtual ~char_node(){}

    virtual node* clone() const;
    virtual bool nullable() const;
    virtual node_set firstpos() const;
    virtual node_set lastpos() const;
    virtual void compute_followpos(followpos_t& followpos) const;
    virtual void compute_state_match(state_match_t& state_match ) const;
    virtual void get_eof_ids(node_set& eof_set) const;
    virtual void assign_node_ids(node_id_t& node_count);
#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
    virtual void dump(std::ostream& out) const;
#endif

private:

    uchar m_char;
    node_id_t m_node_num;
};

inline
char_node::char_node(const uchar c)
    : node()
    , m_char(c)
    , m_node_num(0)
{
}

inline
char_node::char_node(const char_node& x)
    : node(x)
    , m_char(x.m_char)
    , m_node_num(x.m_node_num)
{
}

inline node *
char_node::clone() const
{
    return new char_node(*this);
}

inline bool
char_node::nullable() const
{
    return false;
}

inline node_set
char_node::firstpos() const
{
    node_set rval;
    rval.insert(m_node_num);
    return rval;
}

inline node_set
char_node::lastpos() const
{
    return firstpos();
}

inline void
char_node::compute_followpos(followpos_t&) const
{
    return;
}

inline void
char_node::compute_state_match(state_match_t& state_match) const
{
    if (state_match.size() < m_node_num + 1)
        state_match.resize(m_node_num + 1);
    state_match[m_node_num].resize(256);
    state_match[m_node_num][m_char] = 1;
}

inline void
char_node::get_eof_ids(node_set&) const
{
    return;
}

inline void
char_node::assign_node_ids(node_id_t& node_count)
{
    m_node_num = node_count++;
}

#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
inline void
char_node::dump(std::ostream& out) const
{
    out << "\nchar_node m_char = " << m_char;
    out << " m_node_num = " << m_node_num;
    out << " nullable() = " << (nullable() ? "true" : "false");
    out << " firstpos() = ";
    node_set fp = firstpos();
    std::copy(fp.begin(), fp.end(),
            std::ostream_iterator<node_id_t>(out, ","));

    out << " lastpos() = ";
    node_set lp = lastpos();
    std::copy(lp.begin(), lp.end(),
            std::ostream_iterator<node_id_t>(out, ","));
}
#endif


class epsilon_node : public node
{

public:

    epsilon_node();
    epsilon_node(const epsilon_node& x);
    virtual ~epsilon_node(){}

    virtual node* clone() const;
    virtual bool nullable() const;
    virtual node_set firstpos() const;
    virtual node_set lastpos() const;
    virtual void compute_followpos(followpos_t& followpos) const;
    virtual void compute_state_match(state_match_t& state_match ) const;
    virtual void get_eof_ids(node_set& eof_set) const;
    virtual void assign_node_ids(node_id_t& node_count);
#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
    virtual void dump(std::ostream& out) const;
#endif

private:

    node_id_t m_node_num;
};

inline
epsilon_node::epsilon_node()
    : node()
    , m_node_num(0)
{
}

inline
epsilon_node::epsilon_node(const epsilon_node& x)
    : node(x)
    , m_node_num(x.m_node_num)
{
}

inline node *
epsilon_node::clone() const
{
    return new epsilon_node(*this);
}

inline bool
epsilon_node::nullable() const
{
    return true;
}

inline node_set
epsilon_node::firstpos() const
{
    return node_set();
}

inline node_set
epsilon_node::lastpos() const
{
    return node_set();
}

inline void
epsilon_node::compute_followpos(followpos_t&) const
{
    return;
}

inline void
epsilon_node::compute_state_match(state_match_t& state_match) const
{
    if (state_match.size() < m_node_num + 1)
        state_match.resize(m_node_num + 1);
    state_match[m_node_num].resize(256, 1);
}

inline void
epsilon_node::get_eof_ids(node_set&) const
{
    return;
}

inline void
epsilon_node::assign_node_ids(node_id_t& node_count)
{
    m_node_num = node_count++;
}

#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
inline void
epsilon_node::dump(std::ostream& out) const
{
    out << "\nepsilon_node";
    out << " m_node_num = " << m_node_num;
    out << " nullable() = " << (nullable() ? "true" : "false");
    out << " firstpos() = ";
    node_set fp = firstpos();
    std::copy(fp.begin(), fp.end(),
            std::ostream_iterator<node_id_t>(out, ","));

    out << " lastpos() = ";
    node_set lp = lastpos();
    std::copy(lp.begin(), lp.end(),
            std::ostream_iterator<node_id_t>(out, ","));
}
#endif


class or_node : public node
{

public:

    or_node(node* left, node* right);
    or_node(const or_node& x);
    virtual ~or_node(){}

    virtual node* clone() const;
    virtual bool nullable() const;
    virtual node_set firstpos() const;
    virtual node_set lastpos() const;
    virtual void compute_followpos(followpos_t& followpos) const;
    virtual void compute_state_match(state_match_t& state_match ) const;
    virtual void get_eof_ids(node_set& eof_set) const;
    virtual void assign_node_ids(node_id_t& node_count);
#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
    virtual void dump(std::ostream& out) const;
#endif

private:

    std::auto_ptr<node> m_left;
    std::auto_ptr<node> m_right;
};

inline
or_node::or_node(node* left, node* right)
    : node()
    , m_left(left)
    , m_right(right)
{
}

inline
or_node::or_node(const or_node& x)
    : node(x)
    , m_left(x.m_left->clone())
    , m_right(x.m_right->clone())
{
}

inline node *
or_node::clone() const
{
    return new or_node(m_left->clone(), m_right->clone());
}

inline bool
or_node::nullable() const
{
    return m_left->nullable() || m_right->nullable();
}

inline node_set
or_node::firstpos() const
{
    node_set rval;
    node_set l = m_left->firstpos();
    node_set r = m_right->firstpos();
    std::set_union(l.begin(), l.end(), r.begin(), r.end(),
            std::inserter(rval, rval.begin()));
    return rval;
}

inline node_set
or_node::lastpos() const
{
    node_set rval;
    node_set l = m_left->lastpos();
    node_set r = m_right->lastpos();
    std::set_union(l.begin(), l.end(), r.begin(), r.end(),
            std::inserter(rval, rval.begin()));
    return rval;
}

inline void
or_node::compute_followpos(followpos_t& followpos) const
{
    m_left->compute_followpos(followpos);
    m_right->compute_followpos(followpos);
}

inline void
or_node::compute_state_match(state_match_t& state_match) const
{
    m_left->compute_state_match(state_match);
    m_right->compute_state_match(state_match);
}

inline void
or_node::get_eof_ids(node_set& eof_nodes) const
{
    m_left->get_eof_ids(eof_nodes);
    m_right->get_eof_ids(eof_nodes);
}

inline void
or_node::assign_node_ids(node_id_t& node_count)
{
    m_left->assign_node_ids(node_count);
    m_right->assign_node_ids(node_count);
}

#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
inline void
or_node::dump(std::ostream& out) const
{
    m_left->dump(out);

    out << "\nor_node";
    out << " nullable() = " << (nullable() ? "true" : "false");
    out << " firstpos() = ";
    node_set fp = firstpos();
    std::copy(fp.begin(), fp.end(),
            std::ostream_iterator<node_id_t>(out, ","));

    out << " lastpos() = ";
    node_set lp = lastpos();
    std::copy(lp.begin(), lp.end(),
            std::ostream_iterator<node_id_t>(out, ","));

    m_right->dump(out);
}
#endif


class cat_node : public node
{

public:

    cat_node(node* left, node* right);
    cat_node(const cat_node& x);
    virtual ~cat_node(){}

    virtual node* clone() const;
    virtual bool nullable() const;
    virtual node_set firstpos() const;
    virtual node_set lastpos() const;
    virtual void compute_followpos(followpos_t& followpos) const;
    virtual void compute_state_match(state_match_t& state_match ) const;
    virtual void get_eof_ids(node_set& eof_set) const;
    virtual void assign_node_ids(node_id_t& node_count);
#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
    virtual void dump(std::ostream& out) const;
#endif

private:

    std::auto_ptr<node> m_left;
    std::auto_ptr<node> m_right;
};

inline
cat_node::cat_node(node* left, node* right)
    : node()
    , m_left(left)
    , m_right(right)
{
}

inline
cat_node::cat_node(const cat_node& x)
    : node(x)
    , m_left(x.m_left->clone())
    , m_right(x.m_right->clone())
{
}

inline node *
cat_node::clone() const
{
    return new cat_node(m_left->clone(), m_right->clone());
}

inline bool
cat_node::nullable() const
{
    return m_left->nullable() && m_right->nullable();
}

inline node_set
cat_node::firstpos() const
{
    if (m_left->nullable())
    {
        node_set rval;
        node_set l = m_left->firstpos();
        node_set r = m_right->firstpos();
        std::set_union(l.begin(), l.end(), r.begin(), r.end(),
                std::inserter(rval, rval.begin()));
        return rval;
    }
    else
    {
        return m_left->firstpos();
    }
}

inline node_set
cat_node::lastpos() const
{
    if (m_right->nullable())
    {
        node_set rval;
        node_set l = m_left->lastpos();
        node_set r = m_right->lastpos();
        std::set_union(l.begin(), l.end(), r.begin(), r.end(),
                std::inserter(rval, rval.begin()));
        return rval;
    }
    else
    {
        return m_right->lastpos();
    }
}

inline void
cat_node::compute_followpos(followpos_t& followpos) const
{
    node_set l = m_left->lastpos();
    for (node_set::iterator i = l.begin();
            i != l.end();
            ++i)
    {
        node_set rf = m_right->firstpos();
        followpos[*i].insert(rf.begin(), rf.end());
    }

    m_left->compute_followpos(followpos);
    m_right->compute_followpos(followpos);
}

inline void
cat_node::compute_state_match(state_match_t& state_match) const
{
    m_left->compute_state_match(state_match);
    m_right->compute_state_match(state_match);
}

inline void
cat_node::get_eof_ids(node_set& eof_nodes) const
{
    m_left->get_eof_ids(eof_nodes);
    m_right->get_eof_ids(eof_nodes);
}

inline void
cat_node::assign_node_ids(node_id_t& node_count)
{
    m_left->assign_node_ids(node_count);
    m_right->assign_node_ids(node_count);
}

#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
inline void
cat_node::dump(std::ostream& out) const
{
    m_left->dump(out);

    out << "\ncat_node";
    out << " nullable() = " << (nullable() ? "true" : "false");
    out << " firstpos() = ";
    node_set fp = firstpos();
    std::copy(fp.begin(), fp.end(),
            std::ostream_iterator<node_id_t>(out, ","));

    out << " lastpos() = ";
    node_set lp = lastpos();
    std::copy(lp.begin(), lp.end(),
            std::ostream_iterator<node_id_t>(out, ","));

    m_right->dump(out);
}
#endif


class star_node : public node
{

public:

    star_node(node* left);
    star_node(const star_node& x);
    virtual ~star_node(){}

    virtual node* clone() const;
    virtual bool nullable() const;
    virtual node_set firstpos() const;
    virtual node_set lastpos() const;
    virtual void compute_followpos(followpos_t& followpos) const;
    virtual void compute_state_match(state_match_t& state_match ) const;
    virtual void get_eof_ids(node_set& eof_set) const;
    virtual void assign_node_ids(node_id_t& node_count);
#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
    virtual void dump(std::ostream& out) const;
#endif

private:

    std::auto_ptr<node> m_left;
};

inline
star_node::star_node(node* left)
    : node()
    , m_left(left)
{
}

inline
star_node::star_node(const star_node& x)
    : node(x)
    , m_left(x.m_left->clone())
{
}

inline node *
star_node::clone() const
{
    return new star_node(m_left->clone());
}

inline bool
star_node::nullable() const
{
    return true;
}

inline node_set
star_node::firstpos() const
{
    return m_left->firstpos();
}

inline node_set
star_node::lastpos() const
{
    return m_left->lastpos();
}

inline void
star_node::compute_followpos(followpos_t& followpos) const
{
    node_set lp = this->lastpos();
    for (node_set::iterator i = lp.begin();
            i != lp.end();
            ++i)
    {
        node_set fp = this->firstpos();
        followpos[*i].insert(fp.begin(), fp.end());
    }

    m_left->compute_followpos(followpos);
}

inline void
star_node::compute_state_match(state_match_t& state_match) const
{
    m_left->compute_state_match(state_match);
}

inline void
star_node::get_eof_ids(node_set& eof_nodes) const
{
    m_left->get_eof_ids(eof_nodes);
}

inline void
star_node::assign_node_ids(node_id_t& node_count)
{
    m_left->assign_node_ids(node_count);
}

#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
inline void
star_node::dump(std::ostream& out) const
{
    m_left->dump(out);

    out << "\nstar_node";
    out << " nullable() = " << (nullable() ? "true" : "false");
    out << " firstpos() = ";
    node_set fp = firstpos();
    std::copy(fp.begin(), fp.end(),
            std::ostream_iterator<node_id_t>(out, ","));

    out << " lastpos() = ";
    node_set lp = lastpos();
    std::copy(lp.begin(), lp.end(),
            std::ostream_iterator<node_id_t>(out, ","));

}
#endif


class eof_node : public node
{

public:

    eof_node();
    eof_node(const eof_node& x);
    virtual ~eof_node(){}

    virtual node* clone() const;
    virtual bool nullable() const;
    virtual node_set firstpos() const;
    virtual node_set lastpos() const;
    virtual void compute_followpos(followpos_t& followpos) const;
    virtual void compute_state_match(state_match_t& state_match ) const;
    virtual void get_eof_ids(node_set& eof_set) const;
    virtual void assign_node_ids(node_id_t& node_count);
#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
    virtual void dump(std::ostream& out) const;
#endif

private:

    node_id_t m_node_num;
};

inline
eof_node::eof_node()
    : node()
    , m_node_num(0)
{
}

inline
eof_node::eof_node(const eof_node& x)
    : node(x)
    , m_node_num(x.m_node_num)
{
}

inline node *
eof_node::clone() const
{
    return new eof_node(*this);
}

inline bool
eof_node::nullable() const
{
    return false;
}

inline node_set
eof_node::firstpos() const
{
    node_set rval;
    rval.insert(m_node_num);
    return rval;
}

inline node_set
eof_node::lastpos() const
{
    node_set rval;
    rval.insert(m_node_num);
    return rval;
}

inline void
eof_node::compute_followpos(followpos_t&) const
{
    return;
}

inline void
eof_node::compute_state_match(state_match_t& state_match) const
{
    if (state_match.size() < m_node_num + 1)
        state_match.resize(m_node_num + 1);
    state_match[m_node_num].resize(256, 0);
}

inline void
eof_node::get_eof_ids(node_set& eof_nodes) const
{
    eof_nodes.insert(m_node_num);
}

inline void
eof_node::assign_node_ids(node_id_t& node_count)
{
    m_node_num = node_count++;
}

#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
inline void
eof_node::dump(std::ostream& out) const
{
    out << "\neof_node";
    out << " m_node_num = " << m_node_num;
    out << " nullable() = " << (nullable() ? "true" : "false");
    out << " firstpos() = ";
    node_set fp = firstpos();
    std::copy(fp.begin(), fp.end(),
            std::ostream_iterator<node_id_t>(out, ","));

    out << " lastpos() = ";
    node_set lp = lastpos();
    std::copy(lp.begin(), lp.end(),
            std::ostream_iterator<node_id_t>(out, ","));
}
#endif

class ccl_node : public node
{

public:

    ccl_node(const std::vector<uchar>& v);
    ccl_node(const uchar c1, const uchar c2);
    ccl_node(const ccl_node& x);
    virtual ~ccl_node(){}

    virtual node* clone() const;
    virtual bool nullable() const;
    virtual node_set firstpos() const;
    virtual node_set lastpos() const;
    virtual void compute_followpos(followpos_t& followpos) const;
    virtual void compute_state_match(state_match_t& state_match ) const;
    virtual void get_eof_ids(node_set& eof_set) const;
    virtual void assign_node_ids(node_id_t& node_count);
#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
    virtual void dump(std::ostream& out) const;
#endif

private:

    std::vector<uchar> m_match;
    node_id_t m_node_num;
};

inline
ccl_node::ccl_node(const std::vector<uchar>& v)
    : node()
    , m_match(v)
    , m_node_num(0)
{
    m_match.resize(256); // make sure it's the right size
}

inline
ccl_node::ccl_node(const uchar c1, const uchar c2)
    : node()
    , m_match(256, uchar(0))
    , m_node_num(0)
{
    BOOST_ASSERT(c1 < c2);
    for (std::size_t i = c1; i <= std::size_t(c2); ++i)
    {
        m_match[i] = 1;
    }
}

inline
ccl_node::ccl_node(const ccl_node& x)
    : node(x)
    , m_match(x.m_match)
    , m_node_num(x.m_node_num)
{
}

inline node *
ccl_node::clone() const
{
    return new ccl_node(*this);
}

inline bool
ccl_node::nullable() const
{
    return false;
}

inline node_set
ccl_node::firstpos() const
{
    node_set rval;
    rval.insert(m_node_num);
    return rval;
}

inline node_set
ccl_node::lastpos() const
{
    return firstpos();
}

inline void
ccl_node::compute_followpos(followpos_t&) const
{
    return;
}

inline void
ccl_node::compute_state_match(state_match_t& state_match) const
{
    if (state_match.size() < m_node_num + 1)
        state_match.resize(m_node_num + 1);
    state_match[m_node_num] = m_match;
}

inline void
ccl_node::get_eof_ids(node_set&) const
{
    return;
}

inline void
ccl_node::assign_node_ids(node_id_t& node_count)
{
    m_node_num = node_count++;
}

#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
inline void
ccl_node::dump(std::ostream& out) const
{
    out << "\nccl_node m_match = ";
    for (std::size_t i = 0; i < m_match.size(); ++i)
    {
        if (m_match[i])
            out << i << ", ";
    }
    out << " m_node_num = " << m_node_num;
    out << " nullable() = " << (nullable() ? "true" : "false");
    out << " firstpos() = ";
    node_set fp = firstpos();
    std::copy(fp.begin(), fp.end(),
            std::ostream_iterator<node_id_t>(out, ","));

    out << " lastpos() = ";
    node_set lp = lastpos();
    std::copy(lp.begin(), lp.end(),
            std::ostream_iterator<node_id_t>(out, ","));
}
#endif

template <typename ScannerT>
class make_concat
{
    typedef typename ScannerT::iterator_t iterator_type;

public:

    make_concat(std::stack<node*>& the_stack)
        : m_stack(the_stack)
        {}

    void operator()(iterator_type const &, iterator_type const &) const
    {
        node* right = m_stack.top();
        m_stack.pop();
        node* left = m_stack.top();
        m_stack.pop();
        node* newnode = new cat_node(left, right);
        m_stack.push(newnode);
    }

    std::stack<node*>& m_stack;
};

template <int CharTSize>
struct get_byte_aux;

template<>
struct get_byte_aux<1>
{
    template <typename CharT>
    unsigned char operator()(CharT c, unsigned int byte)
    {
        BOOST_ASSERT(byte == 0);
        return c;
    }
};

template<>
struct get_byte_aux<2>
{
    template <typename CharT>
    unsigned char operator()(CharT c, unsigned int byte)
    {
        static unsigned long mask[] =
        {
            0xFF00,
            0x00FF
        };

        BOOST_ASSERT(byte < 2);
        return (c & mask[byte]) >> ((sizeof(c) - 1 - byte) * 8);
    }
};

template<>
struct get_byte_aux<4>
{
    template <typename CharT>
    unsigned char operator()(CharT c, unsigned int byte)
    {
        static unsigned long mask[] =
        {
            0xFF000000,
            0x00FF0000,
            0x0000FF00,
            0x000000FF
        };

        BOOST_ASSERT(byte < 4);
        return (c & mask[byte]) >> ((sizeof(c) - 1 - byte) * 8);
    }
};

template <typename CharT>
inline unsigned char
get_byte(CharT c, unsigned int byte)
{
    return get_byte_aux<sizeof(c)>()(c, byte);
}

template <typename ScannerT>
class make_star
{
    typedef typename ScannerT::iterator_t iterator_type;

public:
    typedef
        typename BOOST_SPIRIT_IT_NS::iterator_traits<iterator_type>::value_type
        char_t;

    make_star(std::stack<node*>& the_stack)
        : m_stack(the_stack)
        {}

    void operator()(const char_t) const
    {
        node* left = m_stack.top();
        m_stack.pop();
        node* newnode = new star_node(left);
        m_stack.push(newnode);
    }

    std::stack<node*>& m_stack;
};

template <typename ScannerT>
class make_or
{
    typedef typename ScannerT::iterator_t iterator_type;

public:

    make_or(std::stack<node*>& the_stack)
        : m_stack(the_stack)
        {}

    void operator()(iterator_type const&, iterator_type const&) const
    {
        node* right = m_stack.top();
        m_stack.pop();
        node* left = m_stack.top();
        m_stack.pop();
        node* newnode = new or_node(left, right);
        m_stack.push(newnode);
    }

    std::stack<node*>& m_stack;
};

template <typename ScannerT>
class make_plus
{
    typedef typename ScannerT::iterator_t iterator_type;

public:
    typedef
        typename BOOST_SPIRIT_IT_NS::iterator_traits<iterator_type>::value_type
        char_t;

    make_plus(std::stack<node*>& the_stack)
        : m_stack(the_stack)
        {}

    void operator()(const char_t) const
    {
        node* left = m_stack.top();
        m_stack.pop();

        node* copy = left->clone();

        node* new_star = new star_node(copy);
        node* new_cat = new cat_node(left, new_star);
        m_stack.push(new_cat);
    }

    std::stack<node*>& m_stack;
};

template <typename ScannerT>
class make_optional
{
    typedef typename ScannerT::iterator_t iterator_type;

public:
    typedef
        typename BOOST_SPIRIT_IT_NS::iterator_traits<iterator_type>::value_type
        char_t;

    make_optional(std::stack<node*>& the_stack)
        : m_stack(the_stack)
        {}

    void operator()(const char_t) const
    {
        node* left = m_stack.top();
        m_stack.pop();

        node* new_or = new or_node(left, new epsilon_node());
        m_stack.push(new_or);
    }

    std::stack<node*>& m_stack;
};

///////////////////////////////////////////////////////////////////////////////
//  utility function
template <typename CharT>
inline utility::impl::range<CharT> const&
full_range()
{
    static utility::impl::range<CharT> full((std::numeric_limits<CharT>::min)(),
        (std::numeric_limits<CharT>::max)());
    return full;
}

namespace ccl_utils
{
    template <typename char_t>
    inline utility::impl::range_run<char_t>
    negate_range_run(
            const utility::impl::range_run<char_t>& rr)
    {
        utility::impl::range_run<char_t> newrr;
        newrr.set(full_range<char_t>());
        for (typename utility::impl::range_run<char_t>::const_iterator iter = rr.begin();
                iter != rr.end(); ++iter)
            newrr.clear(*iter);
        return newrr;
    }

    template <typename char_t>
    inline node*
    create_mb_node_seq(char_t c)
    {
        node* newnode = new char_node(get_byte(c, 0));
        for (unsigned int i = 1; i < sizeof(c); ++i)
        {
            node* cnode = new char_node(get_byte(c, i));
            node* top_node = new cat_node(newnode, cnode);
            newnode = top_node;
        }
        return newnode;
    }

    template <typename char_t>
    inline void
    handle_mb_char(char_t c, bool first_time,
            std::stack<node*>& stack)
    {
        node* newnode = create_mb_node_seq(c);

        if (first_time)
        {
            stack.push(newnode);
        }
        else
        {
            node* top = stack.top();
            stack.pop();

            node* newtop = new or_node(top, newnode);
            stack.push(newtop);
        }
    }

    // forward decl only
    template <typename char_t>
    inline void
    handle_mb_range(char_t c1, char_t c2, bool first_time,
            std::stack<node*>& stack);

    template <typename char_t>
    inline void
    create_nodes(const utility::impl::range_run<char_t>& rr,
            std::stack<node*>& stack)
    {

        if (sizeof(char_t) == 1)
        {
            std::vector<uchar> ccl;
            ccl.resize(256);
            for (typename utility::impl::range_run<char_t>::const_iterator iter = rr.begin();
                    iter != rr.end(); ++iter)
            {
                for (int i = iter->first; i <= iter->last; ++i)
                {
//                  this is always true because of the limited datatype
//                  BOOST_ASSERT(uchar(i) < 256 && ccl.size() == 256);
                    ccl[uchar(i)] = 1;
                }
            }

            node* new_ccl = new ccl_node(ccl);
            stack.push(new_ccl);
        }
        else
        {
            bool mb_first_time = true;
            for (typename utility::impl::range_run<char_t>::const_iterator iter = rr.begin();
                    iter != rr.end(); ++iter)
            {
                if (iter->first == iter->last)
                {
                    handle_mb_char(iter->first, mb_first_time, stack);
                }
                else
                {
                    handle_mb_range(iter->first, iter->last, mb_first_time, stack);
                }
                mb_first_time = false;
            }
        }
    }

    template <typename char_t>
    inline std::size_t
    compute_differing_byte(char_t c1, char_t c2)
    {
        std::size_t rval = 0;
        while (rval < sizeof(c1) && 
               get_byte(c1, (unsigned int)rval) == get_byte(c2, (unsigned int)rval))
        {
           ++rval;
        }
        return rval;
    }

    template <typename char_t>
    inline node*
    create_mb_node_type1(std::size_t j, char_t c1, char_t c2)
    {
        std::size_t diff = get_byte(c2, (unsigned int)j) - 
            get_byte(c1, (unsigned int)j);
        if (diff == 1) {
            return 0;
        }
        else if (diff == 2) {
            return new char_node(get_byte(c1, (unsigned int)j)+1);
        }
        else {
            return new ccl_node(get_byte(c1, (unsigned int)j)+1, 
                get_byte(c2, (unsigned int)j)-1);
        }
    }

    template <typename char_t>
    inline node *
    create_mb_node_for_byte(std::size_t i, std::size_t j, std::size_t sizem1,
            std::size_t differing_byte, char_t c1, char_t c2, node* newnode)
    {
        node* cnode;
        if (i == sizem1 && j == differing_byte && j != sizem1)
        {
            node* tmp = create_mb_node_type1(j, c1, c2);
            if (tmp == 0)
            {
                delete newnode;
                return 0;
            }
            else
                cnode = tmp;
        }
        else if (i == differing_byte && j == sizem1)
        {
            if (i != sizem1) {
                cnode = new ccl_node(get_byte(c1, (unsigned int)j), 0xFF);
            }
            else {
                cnode = new ccl_node(get_byte(c1, (unsigned int)j), 
                    get_byte(c2, (unsigned int)j));
            }
        }
        else if (i != differing_byte && i != sizem1 &&
                j == (sizem1 - i + differing_byte))
        {
            cnode = new ccl_node(get_byte(c1, (unsigned int)j)+1, 0xFF);
        }
        else if (i + j - differing_byte > sizem1) {
            cnode = new ccl_node(0, 0xFF);
        }
        else {//if (is plain)
            cnode = new char_node(get_byte(c1, (unsigned int)j));
        }

        node* top_node = new cat_node(newnode, cnode);
        return top_node;
    }

// On platforms, where wchar_t is a typedef for unsigned short, the
// comparision for a negative value is pointless
    template <bool is_signed>
    struct correct_char_aux {
    };

    template <>
    struct correct_char_aux<true> {

        template <typename char_t>
        static char_t correct(char_t c) { if (c < 0) c = 0; return c; }
    };

    template <>
    struct correct_char_aux<false> {

        template <typename char_t>
        static char_t correct(char_t c) { return c; }
    };

    template <typename char_t>
    struct correct_char
    {
        static char_t correct(char_t c)
        {
            return correct_char_aux<std::numeric_limits<char_t>::is_signed >::
                correct(c);
        }
    };

    template <typename char_t>
    inline void
    handle_mb_range(char_t c1, char_t c2, bool first_time,
            std::stack<node*>& stack)
    {
        // The algorithm can't handle negative value chars, which don't make
        // much sense anyway. This comparision is pointless for wchar_t's on
        // platforms, where wchar_t is a typedef for unsigned short

        c1 = correct_char<char_t>::correct(c1);
        //if (c1 < 0)
        //    c1 = 0;

        BOOST_ASSERT(c1 < c2);
        node* newnode = 0;
        node* savednode = 0;
        const std::size_t differing_byte = compute_differing_byte(c1, c2);
        const std::size_t sizem1 = sizeof(c1) - 1;
        const std::size_t ndb = sizem1 - differing_byte;
        for (std::size_t i = differing_byte; i < sizeof(c1); ++i)
        {
            // generate node for the first byte
            if (differing_byte == 0 && i == ndb)
            {
                node* tmp = create_mb_node_type1(0, c1, c2);
                if (tmp == 0)
                    continue;
                else
                    newnode = tmp;
            }
            else
            {
                newnode = new char_node(get_byte(c1, 0));
            }
            for (std::size_t j = 1; j < sizeof(c1); ++j)
            {
                newnode = create_mb_node_for_byte(i, j, sizem1, differing_byte,
                        c1, c2, newnode);
                if (newnode == 0)
                    goto end_outer_for;
            }

            // or together the various parts
            if (savednode)
            {
                node* top_node = new or_node(savednode, newnode);
                savednode = top_node;
            }
            else
            {
                savednode = newnode;
            }
end_outer_for:
            continue;
        }

        for (std::size_t k = 0; k < ndb; ++k)
        {
            newnode = new char_node(get_byte(c2, 0));
            for (std::size_t j = 1; j < sizeof(c2); ++j)
            {
                node* cnode;
                if (k == differing_byte && j == sizem1 && k != sizem1)
                    cnode = new ccl_node(0, get_byte(c2, (unsigned int)j));

                else if (k != differing_byte && k != sizem1 &&
                        j == (sizem1 - k + differing_byte))
                    cnode = new ccl_node(0, get_byte(c2, (unsigned int)j)-1);

                else if (k + j - differing_byte > sizem1)
                    cnode = new ccl_node(0, 0xFF);

                else //if (is plain)
                    cnode = new char_node(get_byte(c2, (unsigned int)j));


                node* top_node = new cat_node(newnode, cnode);
                newnode = top_node;
            }

            // or together the various parts
            if (savednode)
            {
                node* top_node = new or_node(savednode, newnode);
                savednode = top_node;
            }
            else
            {
                savednode = newnode;
            }
        }


        if (first_time)
        {
            stack.push(savednode);
        }
        else
        {
            node* top = stack.top();
            stack.pop();

            node* newtop = new or_node(top, savednode);
            stack.push(newtop);
        }
    }
} // namespace ccl_utils

template <typename ScannerT>
class make_char
{
    typedef typename ScannerT::iterator_t iterator_type;

public:
    typedef
        typename BOOST_SPIRIT_IT_NS::iterator_traits<iterator_type>::value_type
        char_t;

    make_char(std::stack<node*>& the_stack)
        : m_stack(the_stack)
        {}

    void operator()(iterator_type const& first, iterator_type const& last) const
    {
        const escape_char_parser<lex_escapes, char_t> lex_escape_ch =
            escape_char_parser<lex_escapes, char_t>();
        char_t the_char;
        iterator_type first_ = first;
        ScannerT scan(first_, last);
        lex_escape_ch[assign(the_char)].parse(scan);
        node* newnode = ccl_utils::create_mb_node_seq(the_char);
        m_stack.push(newnode);
    }

    std::stack<node*>& m_stack;
};


template <typename ScannerT>
class make_ccl
{
    typedef typename ScannerT::iterator_t iterator_type;

public:
    typedef
        typename BOOST_SPIRIT_IT_NS::iterator_traits<iterator_type>::value_type
        char_t;

    make_ccl(std::stack<node*>& the_stack)
        : m_stack(the_stack)
        {}

    static bool is_equal_to_string(iterator_type first,
        iterator_type const & last, const char* str)
    {
        while (first != last &&*str &&*first ==*str)
        {
            ++first;
            ++str;
        }
        return*str == 0;
    }

    template <typename ParserT>
    static void fill_ccl(utility::impl::range_run<char_t>& rr, const ParserT& parser)
    {
        for (int i = 0; i < 256; ++i)
        {
            if (parser.test(static_cast<char_t>(uchar(i))))
                rr.set(utility::impl::range<char_t>(char_t(i), char_t(i)));
        }
    }

    void operator()(iterator_type const& first_, iterator_type const& last) const
    {
        BOOST_ASSERT(*first_ == '[');

        iterator_type first = first_;
        ++first; // skip over '['
        bool negated_ccl = false;
        if (*first == '^')
        {
            negated_ccl = true;
            ++first;
        }

        utility::impl::range_run<char_t> rr;
        while (first != last &&*first != ']')
        {
            if (*first == '[') // it's a ccl_expr like [:space:]
            {
                // check for [:space:], etc.
                if (is_equal_to_string(first, last, "[:alnum:]"))
                {
                    fill_ccl(rr, alnum_p);
                }
                else if (is_equal_to_string(first, last, "[:alpha:]"))
                {
                    fill_ccl(rr, alpha_p);
                }
                else if (is_equal_to_string(first, last, "[:blank:]"))
                {
                    fill_ccl(rr, blank_p);
                }
                else if (is_equal_to_string(first, last, "[:cntrl:]"))
                {
                    fill_ccl(rr, cntrl_p);
                }
                else if (is_equal_to_string(first, last, "[:digit:]"))
                {
                    fill_ccl(rr, digit_p);
                }
                else if (is_equal_to_string(first, last, "[:graph:]"))
                {
                    fill_ccl(rr, graph_p);
                }
                else if (is_equal_to_string(first, last, "[:lower:]"))
                {
                    fill_ccl(rr, lower_p);
                }
                else if (is_equal_to_string(first, last, "[:print:]"))
                {
                    fill_ccl(rr, print_p);
                }
                else if (is_equal_to_string(first, last, "[:punct:]"))
                {
                    fill_ccl(rr, punct_p);
                }
                else if (is_equal_to_string(first, last, "[:space:]"))
                {
                    fill_ccl(rr, space_p);
                }
                else if (is_equal_to_string(first, last, "[:upper:]"))
                {
                    fill_ccl(rr, upper_p);
                }
                else if (is_equal_to_string(first, last, "[:xdigit:]"))
                {
                    fill_ccl(rr, xdigit_p);
                }
                // this can't happen, because it's parsed before we get here.
                //else
                //    throw bad_regex();

                // Advance past the character class expression
                while (first != last &&*first != ']')
                    ++first;
                BOOST_ASSERT(*first == ']');
                ++first;
            }
            else {
                const escape_char_parser<lex_escapes, char_t> lex_escape_ch =
                    escape_char_parser<lex_escapes, char_t>();

                char_t c1;
                ScannerT scan(first, last);
                lex_escape_ch[assign(c1)].parse(scan);
                if (*scan.first == '-') // insert a range
                {
                    ++scan.first;
                    char_t c2;
                    lex_escape_ch[assign(c2)].parse(scan);
                    BOOST_ASSERT(c1 < c2); // Throw exception?
                    rr.set(utility::impl::range<char_t>(c1, c2));
                }
                else // insert 1 char
                {
                    rr.set(utility::impl::range<char_t>(c1, c1));
                }
            }
        }

        if (negated_ccl)
        {
            rr = ccl_utils::negate_range_run(rr);
        }

        ccl_utils::create_nodes(rr, m_stack);
    }

    std::stack<node*>& m_stack;
};

template <typename ScannerT>
class make_any_char
{
    typedef typename ScannerT::iterator_t iterator_type;

public:
    typedef
        typename BOOST_SPIRIT_IT_NS::iterator_traits<iterator_type>::value_type
        char_t;

    std::stack<node*>& m_stack;

    make_any_char(std::stack<node*>& the_stack)
        : m_stack(the_stack)
        {}

    void operator()(const char_t c) const
    {
        BOOST_ASSERT(c == '.');
        do_any_char();
    }

    void do_any_char() const
    {
        static utility::impl::range_run<char_t> rr;
        rr.set(full_range<char_t>());
        char_t newline = '\n';
        rr.clear(utility::impl::range<char_t>(newline, newline));

        ccl_utils::create_nodes(rr, m_stack);
    }
};

template <typename ScannerT>
class make_string
{
    typedef typename ScannerT::iterator_t iterator_type;

public:
    typedef
        typename BOOST_SPIRIT_IT_NS::iterator_traits<iterator_type>::value_type
        char_t;

    std::stack<node*>& m_stack;

    make_string(std::stack<node*>& the_stack)
        : m_stack(the_stack)
        {}

    void operator()(iterator_type const& first, iterator_type const& last) const
    {
        BOOST_ASSERT(*first == '"');

        iterator_type first_ = first;
        ScannerT scan(first_, last);
        ++scan.first; // skip over '"'

        // empty string not allowed
        if (*scan.first == '"')
        {
            boost::throw_exception(bad_regex());
        }

        const escape_char_parser<lex_escapes, char_t> lex_escape_ch =
            escape_char_parser<lex_escapes, char_t>();

        char_t c;
        lex_escape_ch[assign(c)].parse(scan);
        node* top_node = ccl_utils::create_mb_node_seq(c);

        while (*scan.first != '"' && scan.first != scan.last)
        {
            lex_escape_ch[assign(c)].parse(scan);
            node* cur_node = ccl_utils::create_mb_node_seq(c);
            top_node = new cat_node(top_node, cur_node);
        }
        m_stack.push(top_node);
    }
};

inline
node* repeat_node(node* n, int num)
{
    node* list_of_nodes = n;
    for (int i = 1; i < num; ++i)
    {
        list_of_nodes = new cat_node(list_of_nodes, n->clone());
    }
    return list_of_nodes;
}

inline
node* optional_node(node* n)
{
    return new or_node(n, new epsilon_node());
}

template <typename ScannerT>
class make_rep1
{
    typedef typename ScannerT::iterator_t iterator_type;

public:
    typedef
        typename BOOST_SPIRIT_IT_NS::iterator_traits<iterator_type>::value_type
        char_t;

    std::stack<node*>& m_stack;

    make_rep1(std::stack<node*>& the_stack)
        : m_stack(the_stack)
        {}

    void operator()(iterator_type const& first, iterator_type const& last) const
    {
        BOOST_ASSERT(*first == '{');

        iterator_type first_ = first;
        ScannerT scan(first_, last);
        ++scan.first; // skip over '{'

        unsigned int count;
        uint_p[assign(count)].parse(scan);
        if (count == 0)
            boost::throw_exception(bad_regex());

        node* top_node = m_stack.top();
        m_stack.pop();
        top_node = repeat_node(top_node, count);
        m_stack.push(top_node);
    }
};

template <typename ScannerT>
class make_rep2
{
    typedef typename ScannerT::iterator_t iterator_type;

public:
    typedef
        typename BOOST_SPIRIT_IT_NS::iterator_traits<iterator_type>::value_type
        char_t;

    std::stack<node*>& m_stack;

    make_rep2(std::stack<node*>& the_stack)
        : m_stack(the_stack)
        {}

    void operator()(iterator_type const& first, iterator_type const& last) const
    {
        BOOST_ASSERT(*first == '{');

        iterator_type first_ = first;
        ScannerT scan (first_, last);
        ++scan.first; // skip over '{'

        unsigned int count;
        uint_p[assign(count)].parse(scan);
        if (count == 0)
            boost::throw_exception(bad_regex());

        node* top_node = m_stack.top();
        m_stack.pop();
        top_node = new cat_node(repeat_node(top_node, count),
                new star_node(top_node->clone()));
        m_stack.push(top_node);

    }
};

template <typename ScannerT>
class make_rep3
{
    typedef typename ScannerT::iterator_t iterator_type;

public:
    typedef
        typename BOOST_SPIRIT_IT_NS::iterator_traits<iterator_type>::value_type
        char_t;

    std::stack<node*>& m_stack;

    make_rep3(std::stack<node*>& the_stack)
        : m_stack(the_stack)
        {}

    void operator()(iterator_type const& first, iterator_type const& last) const
    {
        BOOST_ASSERT(*first == '{');

        iterator_type first_ = first;
        ScannerT scan(first_, last);
        ++scan.first; // skip over '{'

        unsigned int count1, count2;
        uint_p[assign(count1)].parse(scan);
        if (count1 == 0)
            boost::throw_exception(bad_regex());

        ++scan.first; // skip over ','

        uint_p[assign(count2)].parse(scan);
        if (count2 <= count1)
            boost::throw_exception(bad_regex());

        node* top_node = m_stack.top();
        m_stack.pop();
        node* repeats = repeat_node(top_node, count1);
        top_node = new cat_node(repeats,
                repeat_node(optional_node(top_node->clone()),
                    count2 - count1));

        m_stack.push(top_node);
    }
};

///////////////////////////////////////////////////////////////////////////////
//
//  Lexer grammar
//
//      Defines the grammar, which mandates the syntax of the understood
//      lexeme definitions passed to lexer::register_regex.
//
///////////////////////////////////////////////////////////////////////////////
class lexer_grammar : public boost::spirit::classic::grammar<lexer_grammar>
{
public:
    lexer_grammar(std::stack<node*> &node_stack_)
    : node_stack(node_stack_) {}

    template <typename ScannerT>
    struct definition
    {
        typedef rule<ScannerT> rule_t;
        typedef typename ScannerT::iterator_t iterator_type;
        typedef
            typename BOOST_SPIRIT_IT_NS::iterator_traits<iterator_type>::value_type
            char_t;

        rule_t regex, re, series, singleton, singleton2, fullccl, ccl, string,
            escseq, ccl_char;
        symbols<> ccl_expr;

        definition(lexer_grammar const &self)
        {
            regex =
                    re >> !('/' >> re) >> !ch_p('$')
                ;

            re =
                    series
                >>*( ('|' >> series)[make_or<ScannerT>(self.node_stack)] )
                ;

            series =
                    singleton
                >>*( singleton[make_concat<ScannerT>(self.node_stack)] )
                ;

            singleton =
                    ch_p('.')[make_any_char<ScannerT>(self.node_stack)]
                    >>  singleton2
                |   fullccl
                    >>  singleton2
                |   ('"' >> string >> '"')
                    [
                        make_string<ScannerT>(self.node_stack)
                    ]
                    >>  singleton2
                |   '(' >> re >> ')'
                    >>  singleton2
                |   ((anychar_p - chset<>("/|*+?.(){}\\")) | escseq)
                    [
                        make_char<ScannerT>(self.node_stack)
                    ]
                    >>  singleton2
                ;

            singleton2 =
                    ch_p('*')[make_star<ScannerT>(self.node_stack)]
                    >> singleton2
                |   ch_p('+')[make_plus<ScannerT>(self.node_stack)]
                    >> singleton2
                |   ch_p('?')[make_optional<ScannerT>(self.node_stack)]
                    >> singleton2
                |   ('{' >> uint_p >> '}')
                    [
                        make_rep1<ScannerT>(self.node_stack)
                    ]
                    >>  singleton2
                |   ('{' >> uint_p >> ',' >> '}')
                    [
                        make_rep2<ScannerT>(self.node_stack)
                    ]
                    >>  singleton2
                |   ('{' >> uint_p >> ',' >> uint_p >> '}')
                    [
                        make_rep3<ScannerT>(self.node_stack)
                    ]
                    >> singleton2
                |   epsilon_p
                ;

            fullccl =
                    ('[' >> !ch_p('^') >> ccl >> ']')
                    [
                        make_ccl<ScannerT>(self.node_stack)
                    ]
                ;

            ccl =
                   *(ccl_expr | (ccl_char >> !('-' >> ccl_char)))
                ;

            ccl_char =
                    ( (anychar_p - chset<>("\\\n]")) | escseq )
                ;

            ccl_expr =
                    "[:alnum:]",
                    "[:alpha:]",
                    "[:blank:]",
                    "[:cntrl:]",
                    "[:digit:]",
                    "[:graph:]",
                    "[:lower:]",
                    "[:print:]",
                    "[:punct:]",
                    "[:space:]",
                    "[:upper:]",
                    "[:xdigit:]"
                ;

            string =
                    +( (anychar_p - chset<>("\"\\")) | escseq )
                ;

            typedef
                uint_parser<char_t, 8, 1,
                    std::numeric_limits<char_t>::digits / 3 + 1
                > oct_parser_t;
            typedef
                uint_parser<char_t, 16, 1,
                    std::numeric_limits<char_t>::digits / 4 + 1
                > hex_parser_t;

            escseq =
                    ch_p('\\')
                    >>  (
                            oct_parser_t()
                        |   as_lower_d['x'] >> hex_parser_t()
                        |   (anychar_p - chset<>('\n'))
                        )
                ;

#define BOOST_SLEX_DEBUG (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)

            BOOST_SPIRIT_DEBUG_TRACE_RULE(regex, BOOST_SLEX_DEBUG);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(re, BOOST_SLEX_DEBUG);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(series, BOOST_SLEX_DEBUG);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(singleton, BOOST_SLEX_DEBUG);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(singleton2, BOOST_SLEX_DEBUG);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(fullccl, BOOST_SLEX_DEBUG);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(ccl, BOOST_SLEX_DEBUG);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(string, BOOST_SLEX_DEBUG);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(escseq, BOOST_SLEX_DEBUG);
            BOOST_SPIRIT_DEBUG_TRACE_RULE(ccl_char, BOOST_SLEX_DEBUG);
            
#undef BOOST_SLEX_DEBUG
        }

        rule<ScannerT> const&
        start() const { return regex; }
    };

    std::stack<node*> &node_stack;

}; // class lexer_grammar

template <typename StringT>
inline node *
parse(lexer_grammar& g, StringT const& str)
{
    typedef
        scanner<typename StringT::const_iterator, scanner_policies<> >
        scanner_t;
    typedef typename rule<scanner_t>::template result<scanner_t>::type
        result_t;
        
    typename StringT::const_iterator first = str.begin();
    typename StringT::const_iterator last = str.end();

    scanner_t scan(first, last);
//    typename rule<scanner_t>::result_t hit = g.parse(scan);
    result_t hit = g.parse(scan);
    if (!hit || !scan.at_end())
    {
        while (g.node_stack.size())
        {
            delete g.node_stack.top();
            g.node_stack.pop();
        }
        return 0;
    }

    BOOST_ASSERT(g.node_stack.size() == 1);
    node* rval = g.node_stack.top();
    g.node_stack.pop();
    node* an_eof_node = new eof_node();
    rval = new cat_node(rval, an_eof_node);
    return rval;
}

inline
void make_case_insensitive(state_match_t& state_match)
{
    // TODO: Fix this.
    // This doesn't take into account foreign languages, figure out how to
    // do that. Also this approach is broken for this implementation of
    // wide chars.
    for (state_match_t::iterator iter = state_match.begin();
            iter != state_match.end(); ++iter)
    {
        int i, j;
        for (i = 'A', j = 'a'; i <= 'Z'; ++i, ++j)
        {
            // if either is set, turn them both on
            (*iter)[i] = (*iter)[j] = uchar((*iter)[i] | (*iter)[j]);
        }
    }
}

template<bool wide_char>
struct regex_match_helper;

template<>
struct regex_match_helper<false> // single byte char
{
    template <typename DfaT, typename IteratorT>
    static bool
    do_match(DfaT const& dfa, IteratorT &first, IteratorT const& last,
        int& regex_index,     
        std::basic_string<
            typename BOOST_SPIRIT_IT_NS::iterator_traits<IteratorT>::value_type
        > *token)
    {
        typedef std::basic_string<
            typename BOOST_SPIRIT_IT_NS::iterator_traits<IteratorT>::value_type
        > string_type;
        typedef typename string_type::size_type size_type;
        
        node_id_t s = 0;
        node_id_t last_accepting_index = invalid_node;
        IteratorT p = first;
        IteratorT last_accepting_cpos = first;
        while (p != last)
        {
            s = dfa.transition_table[s][(uchar)*p];
            if (s == invalid_node)
                break;
            if (token) token->append((size_type)1, *p);
            ++p;
            if (dfa.acceptance_index[s] != invalid_node)
            {
                last_accepting_index = s;
                last_accepting_cpos = p;
            }
        }
        if (last_accepting_index != invalid_node)
        {
#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
            std::cout << "dfa.acceptance_index[" << last_accepting_index << "] = " <<
                dfa.acceptance_index[last_accepting_index] << '\n';
#endif

            first = last_accepting_cpos;
            regex_index = dfa.acceptance_index[last_accepting_index];
            return true;
        }
        else
            return false;
    }
};

template<>
struct regex_match_helper<true> // wide char
{
    template <typename DfaT, typename IteratorT>
    static bool
    do_match(DfaT const& dfa, IteratorT &first, IteratorT const& last,
        int& regex_index,
        std::basic_string<
            typename BOOST_SPIRIT_IT_NS::iterator_traits<IteratorT>::value_type
        > *token)
    {
        typedef
            typename BOOST_SPIRIT_IT_NS::iterator_traits<IteratorT>::value_type
            char_t;
        typedef std::basic_string<char_t> string_type;
        typedef typename string_type::size_type size_type;

        node_id_t s = 0;
        node_id_t last_accepting_index = invalid_node;
        IteratorT wp = first;
        IteratorT last_accepting_cpos = first;

        while (wp != last)
        {
            for (unsigned int i = 0;  i < sizeof(char_t); ++i)
            {
                s = dfa.transition_table[s][get_byte(*wp,i)];
                if (s == invalid_node)
                {
                    goto break_while;
                }
            }
            if (token) token->append((size_type)1, *wp);
            ++wp;
            if (dfa.acceptance_index[s] != invalid_node)
            {
                last_accepting_index = s;
                last_accepting_cpos = wp;
            }

        }

    break_while:
        if (last_accepting_index != invalid_node)
        {
#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
            std::cout << "dfa.acceptance_index[" << last_accepting_index << "] = " <<
                dfa.acceptance_index[last_accepting_index] << '\n';
#endif
            first = last_accepting_cpos;
            regex_index = dfa.acceptance_index[last_accepting_index];

            return true;
        }
        else
            return false;
    }
};

template <typename DfaT, typename IteratorT, bool wide_char>
struct regex_match
{
    static bool
    do_match(DfaT const& dfa, IteratorT &first, IteratorT const& last,
        int& regex_index,
        std::basic_string<
            typename BOOST_SPIRIT_IT_NS::iterator_traits<IteratorT>::value_type
        > *token)
    {
        return regex_match_helper<wide_char>::do_match(
            dfa, first, last, regex_index, token);
    }
};

} // namespace lexerimpl

///////////////////////////////////////////////////////////////////////////////
//
template <typename IteratorT = char const*, typename TokenT = int,
          typename CallbackT = void(*)(IteratorT const &,
                                       IteratorT &,
                                       IteratorT const&,
                                       TokenT const&,
                                       lexer_control<TokenT> &)>
class lexer
{
public:
    typedef CallbackT callback_t;
    typedef
        typename BOOST_SPIRIT_IT_NS::iterator_traits<IteratorT>::value_type
        char_t;

    struct regex_info
    {
        std::basic_string<char_t>   str;
        TokenT                      token;
        CallbackT                   callback;

        regex_info(const std::basic_string<char_t>& _str,
                const TokenT& _token,
                const CallbackT& _callback)
            : str(_str)
            , token(_token)
            , callback(_callback)
            {}

    };

    struct dfa_table
    {
        std::vector<std::vector<node_id_t> >    transition_table;
        std::vector<node_id_t>                  acceptance_index;
    };
    typedef std::vector<node_id_t> node_table_t;
    typedef std::vector<node_table_t> transition_table_t;
    typedef std::vector<dfa_table> dfa_t;


    lexer(unsigned int states = 1);

    void register_regex(const std::basic_string<char_t>& regex,
            const TokenT& id, const CallbackT& cb = CallbackT(),
            unsigned int state = 0);

    TokenT next_token(IteratorT &first, IteratorT const &last,
        std::basic_string<char_t> *token = 0);

    void create_dfa();
    bool has_compiled_dfa() { return m_compiled_dfa; }
    
    void set_case_insensitive(bool insensitive);

#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
    void dump(std::ostream& out);
#endif
    typedef std::vector<std::vector<regex_info> > regex_list_t;

    bool load (std::ifstream &in, long unique_id = 0);
    bool save (std::ofstream &out, long unique_id = 0);
    enum {
        SLEX_SIGNATURE = 0x58454C53,    // "SLEX"
        SLEX_VERSION_100 = 0x0100,      // persistance version
        SLEX_LAST_KNOWN_VERSION = SLEX_VERSION_100,
        SLEX_MINOR_VERSION_MASK = 0xFF
    };

private:

    void create_dfa_for_state(int state);

    static bool regex_match(const dfa_t& dfa, IteratorT& first,
        IteratorT& last, int& regex_index);

    mutable std::stack<lexerimpl::node*> node_stack;
    lexerimpl::lexer_grammar g;

    mutable bool m_compiled_dfa;
    mutable dfa_t m_dfa;

    regex_list_t m_regex_list;
    bool m_case_insensitive;

    unsigned int m_state;
    std::stack<unsigned int> m_state_stack;
    unsigned int m_num_states;
};


template <typename IteratorT, typename TokenT, typename CallbackT>
inline
lexer<IteratorT, TokenT, CallbackT>::lexer(unsigned int states)
    : g(node_stack)
    , m_compiled_dfa(false)
    , m_regex_list(states)
    , m_case_insensitive(false)
    , m_state(0)
    , m_state_stack()
    , m_num_states(states)
{
    BOOST_SPIRIT_DEBUG_TRACE_NODE_NAME(g, "slex::lexer", 
        BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX);
}

template <typename IteratorT, typename TokenT, typename CallbackT>
inline void
lexer<IteratorT, TokenT, CallbackT>::register_regex(
        const std::basic_string<char_t>& regex, const TokenT& id,
        const CallbackT& callback, unsigned int state)
{
    if (state > m_num_states) {
        m_regex_list.resize(state);
        m_num_states = state;
    }
    m_regex_list[state].push_back(regex_info(regex, id, callback));
}

template <typename IteratorT, typename TokenT, typename CallbackT>
inline TokenT
lexer<IteratorT, TokenT, CallbackT>::next_token(
    IteratorT &first, IteratorT const& last,
    std::basic_string<
        typename BOOST_SPIRIT_IT_NS::iterator_traits<IteratorT>::value_type
    > *token)
{
    if (!m_compiled_dfa)
    {
        create_dfa();
    }

    IteratorT saved = first;
    int regex_index;
    if (!lexerimpl::regex_match<dfa_table, IteratorT, (sizeof(char_t) > 1)>::
            do_match(m_dfa[m_state], first, last, regex_index, token))
        return -1;  // TODO: can't return -1, need to return some invalid token.
    // how to figure this out?  We can use traits I guess.
    else
    {
        regex_info regex = m_regex_list[m_state][regex_index];
        TokenT rval = regex.token;
        if (regex.callback)
        {
            // execute corresponding callback
            lexer_control<TokenT> controller(rval, m_state, m_state_stack);
            regex.callback(saved, first, last, regex.token, controller);
            if (controller.ignore_current_token_set()) {
                if (token)
                    token->erase();
                return next_token(first, last, token);
            }
        }
        return rval;
    }
}

namespace lexerimpl
{

inline
bool find_acceptance_state(const node_set& eof_node_ids,
        const node_set& current_set,
        node_id_t& acceptance_node_id)
{
    for(node_set::const_iterator nsi = eof_node_ids.begin();
            nsi != eof_node_ids.end(); ++nsi)
    {
        node_id_t eof_node_id =*nsi;
        if (current_set.end() != current_set.find(eof_node_id))
        {
            // store the first one we come to as the
            // matched pattern
            acceptance_node_id = eof_node_id;
            // don't bother searching for more
            return true;
        }
    }
    return false;
}

template <typename RegexListT, typename GrammarT>
inline std::auto_ptr<node>
parse_regexes(const RegexListT& regex_list, GrammarT& g)
{
    // parse the expressions into a tree
    if (regex_list.begin() == regex_list.end())
        boost::throw_exception(bad_regex());

    typename RegexListT::const_iterator ri = regex_list.begin();
    std::auto_ptr<node> tree(lexerimpl::parse(g, (*ri).str));
    if (tree.get() == 0)
        boost::throw_exception(bad_regex());

    ++ri;
    for (/**/; ri != regex_list.end(); ++ri)
    {
        std::auto_ptr<node> next_tree(lexerimpl::parse(g, (*ri).str));
        if (next_tree.get() == 0)
            boost::throw_exception(bad_regex());
        std::auto_ptr<node> newnode(new or_node(tree.release(), next_tree.release()));
        tree = newnode;
    }
    return tree;
}

} //namespace lexerimpl

template <typename IteratorT, typename TokenT, typename CallbackT>
inline void
lexer<IteratorT, TokenT, CallbackT>::create_dfa()
{
    m_dfa.resize(m_num_states);
    for (unsigned int i = 0; i < m_num_states; ++i)
        create_dfa_for_state(i);
}

// Algorithm from Compilers: Principles, Techniques, and Tools p. 141
template <typename IteratorT, typename TokenT, typename CallbackT>
inline void
lexer<IteratorT, TokenT, CallbackT>::create_dfa_for_state(int state)
{
    using lexerimpl::node;
    std::auto_ptr<node> tree = lexerimpl::parse_regexes(m_regex_list[state], g);
    node_id_t dummy = 0;
    tree->assign_node_ids(dummy);

#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
    tree->dump(std::cout);
#endif

    // compute followpos(root)
    followpos_t followpos;
    tree->compute_followpos(followpos);

    // the dfa states <-> nfa state groups
    std::map<node_set, node_id_t> dstates1;
    std::map<node_id_t, node_set> dstates2;

    // the dfa transitions
    m_dfa[state].transition_table.push_back(
            std::vector<node_id_t>(256, invalid_node));
    m_dfa[state].acceptance_index.push_back(invalid_node);

    // whether the dfa state has been processed yet
    std::vector<node_id_t> marked;

    // used to give a unique id to each dfa state
    node_id_t num_states = 0;

    // initially, the only unmarked state in Dstates is firstpos(root).
    marked.push_back(0);
    node_set fpr = tree->firstpos();
    dstates1[fpr] = 0;
    dstates2[0] = fpr;
    state_match_t state_match;
    tree->compute_state_match(state_match);

    if (m_case_insensitive)
        lexerimpl::make_case_insensitive(state_match);

    node_set eof_node_ids;
    tree->get_eof_ids(eof_node_ids);
    // translate the eof_node_ids into a 0-based index
    std::map<node_id_t, node_id_t> eof_node_id_map;
    unsigned int x = 0;
    for (node_set::iterator node_id_it = eof_node_ids.begin();
            node_id_it != eof_node_ids.end();
            ++node_id_it)
    {
        eof_node_id_map[*node_id_it] = x++;
    }

    // figure out if this is an acceptance state
    node_id_t eof_node_id;
    if (lexerimpl::find_acceptance_state(eof_node_ids, fpr, eof_node_id))
    {
        m_dfa[state].acceptance_index[0] = eof_node_id_map[eof_node_id];
    }

    std::vector<node_id_t>::iterator i = std::find(marked.begin(), marked.end(),
            node_id_t(0));
    while (marked.end() != i)
    {
       *i = 1;
        node_id_t T = node_id_t(std::distance(marked.begin(), i));
        BOOST_ASSERT(T < dstates2.size());
        node_set Tstates = dstates2[T];
        for (node_id_t j = 0; j < 256; ++j)
        {
            node_set U;
            for (node_set::iterator k = Tstates.begin();
                    k != Tstates.end(); ++k)
            {
                node_id_t p =*k;
                BOOST_ASSERT(p < state_match.size());
                BOOST_ASSERT(j < state_match[p].size());
                if (state_match[p][j])
                {
                    node_set fpp = followpos[p];
                    U.insert(fpp.begin(), fpp.end());
                }
            }
            if (U.size() > 0)
            {
                std::map<node_set, node_id_t>::iterator l = dstates1.find(U);
                node_id_t target_state;
                if (l == dstates1.end()) // not in the states yet
                {
                    ++num_states;
                    dstates1[U] = target_state = num_states;
                    dstates2[target_state] = U;
                    marked.push_back(0);
                    m_dfa[state].transition_table.push_back(
                            std::vector<node_id_t>(256, invalid_node));
                    m_dfa[state].acceptance_index.push_back(invalid_node);
                    // figure out if this is an acceptance state
                    node_id_t eof_node_id;
                    if (lexerimpl::find_acceptance_state(eof_node_ids, U, eof_node_id))
                    {
                        m_dfa[state].acceptance_index[target_state] =
                            eof_node_id_map[eof_node_id];
                    }
                }
                else
                {
                    target_state = dstates1[U];
                }

                BOOST_ASSERT(T < m_dfa[state].transition_table.size());
                BOOST_ASSERT(j < m_dfa[state].transition_table[T].size());
                m_dfa[state].transition_table[T][j] = target_state;
            }

        }

        i = std::find(marked.begin(), marked.end(), node_id_t(0));
    }
    m_compiled_dfa = true;
}

template <typename IteratorT, typename TokenT, typename CallbackT>
inline void
lexer<IteratorT, TokenT, CallbackT>::set_case_insensitive(bool insensitive)
{
    m_case_insensitive = insensitive;
}


#if defined(BOOST_SPIRIT_DEBUG) && (BOOST_SPIRIT_DEBUG_FLAGS & BOOST_SPIRIT_DEBUG_FLAGS_SLEX)
template <typename IteratorT, typename TokenT, typename CallbackT>
inline void
lexer<IteratorT, TokenT, CallbackT>::dump(std::ostream& out)
{
    for (unsigned x = 0; x < m_dfa.size(); ++x)
    {
        out << "\nm_dfa[" << x << "] has " << m_dfa[x].transition_table.size() << " states\n";
        for (node_id_t i = 0; i < m_dfa[x].transition_table.size(); ++i)
        {
            out << "state " << i << ":";
            for (node_id_t j = 0; j < m_dfa[x].transition_table[i].size(); ++j)
            {
                if (m_dfa[x].transition_table[i][j] != invalid_node)
                    out << j << "->" << m_dfa[x].transition_table[i][j] << " ";
            }
            out << "\n";
        }
        out << "acceptance states: ";
        for(unsigned int k = 0; k < m_dfa[x].acceptance_index.size(); ++k)
        {
            if (m_dfa[x].acceptance_index[k] != invalid_node)
                out << '<' << k << ',' << m_dfa[x].acceptance_index[k] << "> ";
        }
        out << endl;
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////
// load the lexer tables
#define slex_in(strm, val) \
    strm.read((char*)&val, sizeof(val)); \
    if (std::ios::goodbit != strm.rdstate()) return false

template <typename IteratorT, typename TokenT, typename CallbackT>
inline bool
lexer<IteratorT, TokenT, CallbackT>::load (std::ifstream &in, long unique_id)
{
// ensure correct signature and version
long signature = 0;

    slex_in (in, signature);
    if (signature != SLEX_SIGNATURE)
        return false;       // not for us

long version = 0;

    slex_in (in, version);
    if ((version & ~SLEX_MINOR_VERSION_MASK) > SLEX_LAST_KNOWN_VERSION)
        return false;       // to new for us

long uid = 0;

    slex_in (in, uid);
    if (uid != unique_id)
        return false;       // not saved by us

// load auxiliary members
int num_states = 0;

    slex_in (in, num_states);

// load the dfa tables
dfa_t in_dfa;
std::size_t dfa_size = 0;

    slex_in (in, dfa_size);
    in_dfa.resize(dfa_size);
    for (std::size_t dfa = 0; dfa < dfa_size; ++dfa)
    {
    // load the transition tables
    std::size_t tt_size = 0;
    transition_table_t &tt_table = in_dfa[dfa].transition_table;

        slex_in (in, tt_size);
        tt_table.resize(tt_size);
        for (std::size_t tt = 0; tt < tt_size; ++tt)
        {
        std::size_t nt_size = 0;
        node_table_t &nt_table = tt_table[tt];

            slex_in (in, nt_size);
            nt_table.resize(nt_size);
            for (std::size_t nt = 0; nt < nt_size; ++nt)
            {
                slex_in (in, nt_table[nt]);
            }
        }

    // load the acceptance index table
    std::size_t ai_size = 0;
    node_table_t &ai_table = in_dfa[dfa].acceptance_index;

        slex_in (in, ai_size);
        ai_table.resize(ai_size);
        for (std::size_t ai = 0; ai < ai_size; ++ai)
        {
            slex_in (in, ai_table[ai]);
        }
    }

    m_dfa.swap(in_dfa);         // success, swap in the read values
    m_num_states = num_states;

    m_compiled_dfa = true;
    return true;
}

#undef slex_in

///////////////////////////////////////////////////////////////////////////////
// save the lexer tables
#define slex_out(strm, val) \
    strm.write((char*)&val, sizeof(val)); \
    if (std::ios::goodbit != strm.rdstate()) return false

template <typename IteratorT, typename TokenT, typename CallbackT>
inline bool
lexer<IteratorT, TokenT, CallbackT>::save (std::ofstream &out, long unique_id)
{
// save signature and version information
long out_long = SLEX_SIGNATURE;

    slex_out(out, out_long);
    out_long = SLEX_VERSION_100;
    slex_out(out, out_long);
    slex_out(out, unique_id);

// save auxiliary members
    slex_out(out, m_num_states);

// save the dfa tables
    typedef typename dfa_t::const_iterator dfa_iter_t;
    typedef transition_table_t::const_iterator transition_table_iter_t;
    typedef node_table_t::const_iterator node_table_iter_t;

    std::size_t out_size_t = m_dfa.size();
    slex_out(out, out_size_t);

    dfa_iter_t end = m_dfa.end();
    for (dfa_iter_t it = m_dfa.begin(); it != end; ++it)
    {
    // save the transition table
        out_size_t = (*it).transition_table.size();
        slex_out(out, out_size_t);

        transition_table_iter_t tt_end = (*it).transition_table.end();
        for (transition_table_iter_t tt_it = (*it).transition_table.begin();
             tt_it != tt_end;
             ++tt_it)
        {
            out_size_t = (*tt_it).size();
            slex_out(out, out_size_t);

            node_table_iter_t nt_end = (*tt_it).end();
            for (node_table_iter_t nt_it = (*tt_it).begin();
                 nt_it != nt_end;
                 ++nt_it)
            {
                slex_out(out, (*nt_it));
            }
        }

    // save the acceptance index table
        out_size_t = (*it).acceptance_index.size();
        slex_out(out, out_size_t);

        node_table_iter_t nt_end = (*it).acceptance_index.end();
        for (node_table_iter_t nt_it = (*it).acceptance_index.begin();
             nt_it != nt_end;
             ++nt_it)
        {
            slex_out(out, (*nt_it));
        }
    }
    return true;
}

#undef slex_out

/*
a lexer_control object supports some operations on the lexer.
    get current lexer state
    set state
    terminate
    state stack (push, pop, top)
    set new token return value
    ignore the current token
    yymore
    get length of matched token
*/
template <typename TokenT>
class lexer_control
{
public:

    lexer_control(TokenT& token, unsigned int& current_state,
        std::stack<unsigned int>& state_stack);
    // functions dealing with the lexer state

    // set the state to state
    void set_state(unsigned int state);

    // get the current state
    unsigned int get_state();

    // pushes the current state onto the top of the state stack and
    // switches to new_state
    void push_state(unsigned int new_state);

    // pops the top of the state stack and switches to it.
    void pop_state();

    // returns the top of the stack without altering the stack's contents.
    unsigned int top_state();

    // functions dealing with the token returned.

    // set a new TokenT return value, overriding that one that was
    // registered via. register_regex()
    void set_token(const TokenT& token);

    // tell the lexer to return an invalid token, signifying termination.
    void terminate();

    // ignore the current token, and move on to the next one.  The current
    // token will NOT be returned from next_token()
    void ignore_current_token();

    // returns true if ignore_current_token() has been called,
    // false otherwise.
    bool ignore_current_token_set();

private:
    TokenT& m_token;
    bool m_ignore_current_token;
    unsigned int& m_current_state;
    std::stack<unsigned int>& m_state_stack;
};

template <typename TokenT>
inline
lexer_control<TokenT>::lexer_control(TokenT& token, unsigned int& current_state,
        std::stack<unsigned int>& state_stack)
    : m_token(token)
    , m_ignore_current_token(false)
    , m_current_state(current_state)
    , m_state_stack(state_stack)
{
}

template <typename TokenT>
inline void
lexer_control<TokenT>::set_state(unsigned int state)
{
    m_current_state = state;
}

template <typename TokenT>
inline unsigned int
lexer_control<TokenT>::get_state()
{
    return m_current_state;
}

template <typename TokenT>
inline void
lexer_control<TokenT>::push_state(unsigned int new_state)
{
    m_state_stack.push(m_current_state);
    m_current_state = new_state;
}

template <typename TokenT>
inline void
lexer_control<TokenT>::pop_state()
{
    m_current_state = m_state_stack.top();
    m_state_stack.pop();
}

template <typename TokenT>
inline unsigned int
lexer_control<TokenT>::top_state()
{
    return m_state_stack.top();
}

template <typename TokenT>
inline void
lexer_control<TokenT>::set_token(const TokenT& token)
{
    m_token = token;
}

template <typename TokenT>
inline void
lexer_control<TokenT>::terminate()
{
    m_token = -1; // TOOD: fix this, need to be determined by traits
}

template <typename TokenT>
inline void
lexer_control<TokenT>::ignore_current_token()
{
    m_ignore_current_token = true;
}

template <typename TokenT>
inline bool
lexer_control<TokenT>::ignore_current_token_set()
{
    return m_ignore_current_token;
}

}   // namespace classic
}   // namespace spirit
}   // namespace boost

#undef BOOST_SPIRIT_IT_NS

#endif


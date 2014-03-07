// Copyright David Abrahams 2004. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef NODE_DWA2004110_HPP
# define NODE_DWA2004110_HPP

# include <iostream>

// Polymorphic list node base class

struct node_base
{
    node_base() : m_next(0) {}

    virtual ~node_base()
    {
        delete m_next;
    }

    node_base* next() const
    {
        return m_next;
    }

    virtual void print(std::ostream& s) const = 0;
    virtual void double_me() = 0;
        
    void append(node_base* p)
    {
        if (m_next)
            m_next->append(p);
        else
            m_next = p;
    }
    
 private:
    node_base* m_next;
};

inline std::ostream& operator<<(std::ostream& s, node_base const& n)
{
    n.print(s);
    return s;
}

template <class T>
struct node : node_base
{
    node(T x)
      : m_value(x)
    {}

    void print(std::ostream& s) const { s << this->m_value; }
    void double_me() { m_value += m_value; }
    
 private:
    T m_value;
};
    
#endif // NODE_DWA2004110_HPP

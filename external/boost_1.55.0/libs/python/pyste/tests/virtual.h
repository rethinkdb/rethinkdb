/* Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
 distribution is subject to the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or copy at 
 http://www.boost.org/LICENSE_1_0.txt) 
 */
namespace virtual_ {
    
struct C
{
public:
    virtual int f()
    {
        return f_abs();
    }

    virtual void bar(int) {}
    virtual void bar(char*) {}

    const char* get_name()
    {
        return name();
    }
    virtual int dummy() { return 0; }

protected:    
    virtual int f_abs() = 0;

private:    
    virtual const char* name() { return "C"; }
};

struct D
{
    virtual int dummy() { return 0; }
};

inline int call_f(C& c) { return c.f(); }
inline int call_dummy(C* c) { return c->dummy(); }
inline int call_dummy(D* d) { return d->dummy(); }

}

/* Copyright Bruno da Silva de Oliveira 2003. Use, modification and 
 distribution is subject to the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or copy at 
 http://www.boost.org/LICENSE_1_0.txt) 
 */  
namespace inherit {
    
template<typename T>
class A
{
public:
   void set(T v) { mData = v; }

   T get() const { return mData; }

private:
   T mData;
};


class B : public A<int>
{
public:
   int go() { return get(); }
};

struct C : B
{
    enum ab { a = 1, b = 2 };
    int f1() { return 1; }
    int x;
    static int s;
};

struct D : C
{
    int f2() { return 2; }
    int y;
};

struct X {};
struct E: X, D {};
}

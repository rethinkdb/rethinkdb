// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/object.hpp>
#include <boost/python/class.hpp>

using namespace boost::python;

class NotCopyable
{
} not_copyable;

object ref_to_noncopyable()
{
  return object(boost::ref(not_copyable));
}

object call_object_3(object f)
{
    return f(3);
}

object message()
{
    return object("hello, world!");
}

object number()
{
    return object(42);
}

object obj_getattr(object x, char const* name)
{
    return x.attr(name);
}

object obj_objgetattr(object x, object const& name)
{
    return x.attr(name);
}

object obj_const_getattr(object const& x, char const* name)
{
    return x.attr(name);
}

object obj_const_objgetattr(object const& x, object const& name)
{
    return x.attr(name);
}

void obj_setattr(object x, char const* name, object value)
{
    x.attr(name) = value;
}

void obj_objsetattr(object x, object const& name, object value)
{
    x.attr(name) = value;
}

void obj_setattr42(object x, char const* name)
{
    x.attr(name) = 42;
}

void obj_objsetattr42(object x, object const& name)
{
    x.attr(name) = 42;
}

void obj_moveattr(object& x, char const* src, char const* dst)
{
    x.attr(dst) = x.attr(src);
}

void obj_objmoveattr(object& x, object const& src, object const& dst)
{
    x.attr(dst) = x.attr(src);
}

void obj_delattr(object x, char const* name)
{
    x.attr(name).del();
}

void obj_objdelattr(object x, object const& name)
{
    x.attr(name).del();
}

object obj_getitem(object x, object key)
{
    return x[key];
}

object obj_getitem3(object x)
{
    return x[3];
}

object obj_const_getitem(object const& x, object key)
{
    return x[key];
}

void obj_setitem(object x, object key, object value)
{
    x[key] = value;
}

void obj_setitem42(object x, object key)
{
    x[key] = 42;
}

void obj_moveitem(object& x, object src, object dst)
{
    x[dst] = x[src];
}

void obj_moveitem2(object const& x_src, object k_src, object& x_dst, object k_dst)
{
    x_dst[k_dst] = x_src[k_src];
}

bool test(object y)
{
    return y;
}

bool test_not(object y)
{
    return !y;
}

bool test_attr(object y, char* name)
{
    return y.attr(name);
}

bool test_objattr(object y, object& name)
{
    return y.attr(name);
}

bool test_not_attr(object y, char* name)
{
    return !y.attr(name);
}

bool test_not_objattr(object y, object& name)
{
    return !y.attr(name);
}

bool test_item(object y, object key)
{
    return y[key];
}

bool test_not_item(object y, object key)
{
    return !y[key];
}

bool check_string_slice()
{
    object s("hello, world");

    if (s.slice(_,-3) != "hello, wo")
        return false;
    
    if (s.slice(-3,_) != "rld")
        return false;
    
    if (s.slice(_,_) != s)
        return false;
    
    if (", " != s.slice(5,7))
        return false;

    return s.slice(2,-1).slice(1,-1)  == "lo, wor";
}

object test_call(object c, object args, object kwds) 
{ 
    return c(*args, **kwds); 
} 

bool check_binary_operators()
{
    int y;
    
    object x(3);

#define TEST_BINARY(op)                         \
    for (y = 1; y < 6; ++y)                     \
    {                                           \
        if ((x op y) != (3 op y))               \
            return false;                       \
    }                                           \
    for (y = 1; y < 6; ++y)                     \
    {                                           \
        if ((y op x) != (y op 3))               \
            return false;                       \
    }                                           \
    for (y = 1; y < 6; ++y)                     \
    {                                           \
        object oy(y);                           \
        if ((oy op x) != (oy op 3))             \
            return false;                       \
    }
    TEST_BINARY(>)
    TEST_BINARY(>=)
    TEST_BINARY(<)
    TEST_BINARY(<=)
    TEST_BINARY(==)
    TEST_BINARY(!=)

    TEST_BINARY(+)
    TEST_BINARY(-)
    TEST_BINARY(*)
    TEST_BINARY(/)
    TEST_BINARY(%)
    TEST_BINARY(<<)
    TEST_BINARY(>>)
    TEST_BINARY(&)
    TEST_BINARY(^)
    TEST_BINARY(|)
    return true;
}

bool check_inplace(object l, object o)
{
    int y;
#define TEST_INPLACE(op)                        \
    for (y = 1; y < 6; ++y)                     \
    {                                           \
        object x(666);                          \
        x op##= y;                              \
        if (x != (666 op y))                    \
            return false;                       \
    }                                           \
    for (y = 1; y < 6; ++y)                     \
    {                                           \
        object x(666);                          \
        x op##= object(y);                      \
        if (!(x == (666 op y)))                 \
            return false;                       \
    }
    TEST_INPLACE(+)
    TEST_INPLACE(-)
    TEST_INPLACE(*)
    TEST_INPLACE(/)
    TEST_INPLACE(%)
    TEST_INPLACE(<<)
    TEST_INPLACE(>>)
    TEST_INPLACE(&)
    TEST_INPLACE(^)
    TEST_INPLACE(|)
        
    l += l;
    for (y = 0; y < 6; ++y)
    {
        if (l[y] != y % 3)
            return false;
    }

#define TEST_ITEM_INPLACE(index, op, n, r1, r2)         \
    l[index] op##= n;                                   \
    if (l[index] != r1)                                 \
        return false;                                   \
    l[index] op##= object(n);                           \
    if (!(l[index] == r2))                              \
        return false;

    TEST_ITEM_INPLACE(0,+,7,7,14)
    TEST_ITEM_INPLACE(1,-,2,-1,-3)
    TEST_ITEM_INPLACE(2,*,3,6,18)
    TEST_ITEM_INPLACE(2,/,2,9,4)
    TEST_ITEM_INPLACE(0,%,4,2,2)
    l[0] += 1;
    TEST_ITEM_INPLACE(0,<<,2,12,48)
    TEST_ITEM_INPLACE(0,>>,1,24,12)
    l[4] = 15;
    TEST_ITEM_INPLACE(4,&,(16+4+1),5,5)
    TEST_ITEM_INPLACE(0,^,1,13,12)
    TEST_ITEM_INPLACE(0,|,1,13,13)

    o.attr("x0") = 0;
    o.attr("x1") = 1;
    o.attr("x2") = 2;
    o.attr("x3") = 0;
    o.attr("x4") = 1;
    
#define TEST_ATTR_INPLACE(index, op, n, r1, r2) \
    o.attr("x" #index) op##= n;                 \
    if (o.attr("x" #index) != r1)               \
        return false;                           \
    o.attr("x" #index) op##= object(n);         \
    if (o.attr("x" #index) != r2)               \
        return false;
    
    TEST_ATTR_INPLACE(0,+,7,7,14)
    TEST_ATTR_INPLACE(1,-,2,-1,-3)
    TEST_ATTR_INPLACE(2,*,3,6,18)
    TEST_ATTR_INPLACE(2,/,2,9,4)
    TEST_ATTR_INPLACE(0,%,4,2,2)
    o.attr("x0") += 1;
    TEST_ATTR_INPLACE(0,<<,2,12,48)
    TEST_ATTR_INPLACE(0,>>,1,24,12)
    o.attr("x4") = 15;
    TEST_ATTR_INPLACE(4,&,(16+4+1),5,5)
    TEST_ATTR_INPLACE(0,^,1,13,12)
    TEST_ATTR_INPLACE(0,|,1,13,13)

    if (l[0] != o.attr("x0"))
        return false;
    if (l[1] != o.attr("x1"))
        return false;
    if (l[2] != o.attr("x2"))
        return false;
    if (l[3] != o.attr("x3"))
        return false;
    if (l[4] != o.attr("x4"))
        return false;

    // set item 5 to be a list, by calling l.__class__
    l[5] = l.attr("__class__")();
    // append an element
    l[5].attr("append")(2);
    // Check its value
    if (l[5][0] != 2)
        return false;
    
    return true;
}

BOOST_PYTHON_MODULE(object_ext)
{
    class_<NotCopyable, boost::noncopyable>("NotCopyable", no_init);

    def("ref_to_noncopyable", ref_to_noncopyable);
    def("call_object_3", call_object_3);
    def("message", message);
    def("number", number);

    def("obj_getattr", obj_getattr);
    def("obj_objgetattr", obj_objgetattr);
    def("obj_const_getattr", obj_const_getattr);
    def("obj_const_objgetattr", obj_const_objgetattr);
    def("obj_setattr", obj_setattr);
    def("obj_objsetattr", obj_objsetattr);
    def("obj_setattr42", obj_setattr42);
    def("obj_objsetattr42", obj_objsetattr42);
    def("obj_moveattr", obj_moveattr);
    def("obj_objmoveattr", obj_objmoveattr);
    def("obj_delattr", obj_delattr);
    def("obj_objdelattr", obj_objdelattr);

    def("obj_getitem", obj_getitem);
    def("obj_getitem3", obj_getitem);
    def("obj_const_getitem", obj_const_getitem);
    def("obj_setitem", obj_setitem);
    def("obj_setitem42", obj_setitem42);
    def("obj_moveitem", obj_moveitem);
    def("obj_moveitem2", obj_moveitem2);

    def("test", test);
    def("test_not", test_not);

    def("test_attr", test_attr);
    def("test_objattr", test_objattr);
    def("test_not_attr", test_not_attr);
    def("test_not_objattr", test_not_objattr);

    def("test_item", test_item);
    def("test_not_item", test_not_item);

    def("test_call", test_call);
    def("check_binary_operators", check_binary_operators);
    def("check_inplace", check_inplace);
    def("check_string_slice", check_string_slice);
        ;
}

#include "module_tail.cpp"

// Copyright David Abrahams and Gottfried Ganssauge 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
# include <boost/python/return_opaque_pointer.hpp>
# include <boost/python/def.hpp>
# include <boost/python/module.hpp>
# include <boost/python/return_value_policy.hpp>

typedef struct opaque_ *opaque;
typedef struct opaque2_ *opaque2;

opaque the_op   = ((opaque) 0x47110815);
opaque2 the_op2 = ((opaque2) 0x08154711);

opaque get() { return the_op; }

void use(opaque op)
{
    if (op != the_op)
        throw std::runtime_error (std::string ("failed"));
}

int useany(opaque op)
{
    return op ? 1 : 0;
}

opaque getnull()
{
    return 0;
}

void failuse (opaque op)
{
    if (op == the_op)
        throw std::runtime_error (std::string ("success"));
}

opaque2 get2 () { return the_op2; }

void use2 (opaque2 op)
{
    if (op != the_op2)
        throw std::runtime_error (std::string ("failed"));
}

void failuse2 (opaque2 op)
{
    if (op == the_op2)
        throw std::runtime_error (std::string ("success"));
}

BOOST_PYTHON_OPAQUE_SPECIALIZED_TYPE_ID(opaque_)
BOOST_PYTHON_OPAQUE_SPECIALIZED_TYPE_ID(opaque2_)

namespace bpl = boost::python;

BOOST_PYTHON_MODULE(opaque_ext)
{
    bpl::def (
        "get", &::get, bpl::return_value_policy<bpl::return_opaque_pointer>());
    bpl::def ("use", &::use);
    bpl::def ("useany", &::useany);
    bpl::def ("getnull", &::getnull, bpl::return_value_policy<bpl::return_opaque_pointer>());
    bpl::def ("failuse", &::failuse);

    bpl::def (
        "get2",
        &::get2,
        bpl::return_value_policy<bpl::return_opaque_pointer>());
    bpl::def ("use2", &::use2);
    bpl::def ("failuse2", &::failuse2);
}

# include "module_tail.cpp"

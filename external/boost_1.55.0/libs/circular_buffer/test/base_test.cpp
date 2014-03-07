// Test of the base circular buffer container.

// Copyright (c) 2003-2008 Jan Gaspar
// Copyright (c) 2013 Antony Polukhin

// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"

#define CB_CONTAINER circular_buffer

#include "common.ipp"

void iterator_constructor_and_assign_test() {

    circular_buffer<MyInteger> cb(4, 3);
    circular_buffer<MyInteger>::iterator it = cb.begin();
    circular_buffer<MyInteger>::iterator itCopy;
    itCopy = it;
    it = it;
    circular_buffer<MyInteger>::const_iterator cit;
    cit = it;
    circular_buffer<MyInteger>::const_iterator end1 = cb.end();
    circular_buffer<MyInteger>::const_iterator end2 = end1;

    BOOST_CHECK(itCopy == it);
    BOOST_CHECK(cit == it);
    BOOST_CHECK(end1 == end2);
    BOOST_CHECK(it != end1);
    BOOST_CHECK(cit != end2);
}

void iterator_reference_test() {

    circular_buffer<Dummy> cb(3, Dummy());
    circular_buffer<Dummy>::iterator it = cb.begin();
    circular_buffer<Dummy>::const_iterator cit = cb.begin() + 1;

    BOOST_CHECK((*it).m_n == Dummy::eVar);
    BOOST_CHECK((*it).fnc() == Dummy::eFnc);
    BOOST_CHECK((*cit).const_fnc() == Dummy::eConst);
    BOOST_CHECK((*it).virtual_fnc() == Dummy::eVirtual);
    BOOST_CHECK(it->m_n == Dummy::eVar);
    BOOST_CHECK(it->fnc() == Dummy::eFnc);
    BOOST_CHECK(cit->const_fnc() == Dummy::eConst);
    BOOST_CHECK(it->virtual_fnc() == Dummy::eVirtual);
}

void iterator_difference_test() {

    circular_buffer<MyInteger> cb(5, 1);
    cb.push_back(2);
    circular_buffer<MyInteger>::iterator it1 = cb.begin() + 2;
    circular_buffer<MyInteger>::iterator it2 = cb.begin() + 3;
    circular_buffer<MyInteger>::const_iterator begin = cb.begin();
    circular_buffer<MyInteger>::iterator end = cb.end();

    BOOST_CHECK(begin - begin == 0);
    BOOST_CHECK(end - cb.begin() == 5);
    BOOST_CHECK(end - end == 0);
    BOOST_CHECK(begin - cb.end() == -5);
    BOOST_CHECK(it1 - cb.begin() == 2);
    BOOST_CHECK(it1 - begin == 2);
    BOOST_CHECK(end - it1 == 3);
    BOOST_CHECK(it2 - it1 == 1);
    BOOST_CHECK(it1 - it2 == -1);
    BOOST_CHECK(it2 - it2 == 0);
}

void iterator_increment_test() {

    circular_buffer<MyInteger> cb(10, 1);
    cb.push_back(2);
    circular_buffer<MyInteger>::iterator it1 = cb.begin();
    circular_buffer<MyInteger>::iterator it2 = cb.begin() + 5;
    circular_buffer<MyInteger>::iterator it3 = cb.begin() + 9;
    it1++;
    it2++;
    ++it3;

    BOOST_CHECK(it1 == cb.begin() + 1);
    BOOST_CHECK(it2 == cb.begin() + 6);
    BOOST_CHECK(it3 == cb.end());
}

void iterator_decrement_test() {

    circular_buffer<MyInteger> cb(10, 1);
    cb.push_back(2);
    circular_buffer<MyInteger>::iterator it1= cb.end();
    circular_buffer<MyInteger>::iterator it2= cb.end() - 5;
    circular_buffer<MyInteger>::iterator it3= cb.end() - 9;
    --it1;
    it2--;
    --it3;

    BOOST_CHECK(it1 == cb.end() - 1);
    BOOST_CHECK(it2 == cb.end() - 6);
    BOOST_CHECK(it3 == cb.begin());
}

void iterator_addition_test() {

    circular_buffer<MyInteger> cb(10, 1);
    cb.push_back(2);
    cb.push_back(2);
    circular_buffer<MyInteger>::iterator it1 = cb.begin() + 2;
    circular_buffer<MyInteger>::iterator it2 = cb.end();
    circular_buffer<MyInteger>::iterator it3 = cb.begin() + 5;
    circular_buffer<MyInteger>::iterator it4 = cb.begin() + 9;
    it1 += 3;
    it2 += 0;
    it3 += 5;
    it4 += -2;

    BOOST_CHECK(it1 == 5 + cb.begin());
    BOOST_CHECK(it2 == cb.end());
    BOOST_CHECK(it3 == cb.end());
    BOOST_CHECK(it4 + 3 == cb.end());
    BOOST_CHECK((-3) + it4 == cb.begin() + 4);
    BOOST_CHECK(cb.begin() + 0 == cb.begin());
}

void iterator_subtraction_test() {

    circular_buffer<MyInteger> cb(10, 1);
    cb.push_back(2);
    cb.push_back(2);
    cb.push_back(2);
    circular_buffer<MyInteger>::iterator it1 = cb.begin();
    circular_buffer<MyInteger>::iterator it2 = cb.end();
    circular_buffer<MyInteger>::iterator it3 = cb.end() - 5;
    circular_buffer<MyInteger>::iterator it4 = cb.begin() + 7;
    it1 -= -2;
    it2 -= 0;
    it3 -= 5;

    BOOST_CHECK(it1 == cb.begin() + 2);
    BOOST_CHECK(it2 == cb.end());
    BOOST_CHECK(it3 == cb.begin());
    BOOST_CHECK(it4 - 7 == cb.begin());
    BOOST_CHECK(it4 - (-3) == cb.end());
    BOOST_CHECK(cb.begin() - 0 == cb.begin());
}

void iterator_element_access_test() {

    circular_buffer<MyInteger> cb(10);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    cb.push_back(4);
    cb.push_back(5);
    cb.push_back(6);
    circular_buffer<MyInteger>::iterator it = cb.begin() + 1;

    BOOST_CHECK(it[0] == 2);
    BOOST_CHECK(it[-1] == 1);
    BOOST_CHECK(it[2] == 4);
}

void iterator_comparison_test() {

    circular_buffer<MyInteger> cb(5, 1);
    cb.push_back(2);
    circular_buffer<MyInteger>::iterator it = cb.begin() + 2;
    circular_buffer<MyInteger>::const_iterator begin = cb.begin();
    circular_buffer<MyInteger>::iterator end = cb.end();

    BOOST_CHECK(begin == begin);
    BOOST_CHECK(end > cb.begin());
    BOOST_CHECK(begin < end);
    BOOST_CHECK(end > begin);
    BOOST_CHECK(end == end);
    BOOST_CHECK(begin < cb.end());
    BOOST_CHECK(!(begin + 1 > cb.end()));
    BOOST_CHECK(it > cb.begin());
    BOOST_CHECK(end > it);
    BOOST_CHECK(begin >= begin);
    BOOST_CHECK(end >= cb.begin());
    BOOST_CHECK(end <= end);
    BOOST_CHECK(begin <= cb.end());
    BOOST_CHECK(it >= cb.begin());
    BOOST_CHECK(end >= it);
    BOOST_CHECK(!(begin + 4 < begin + 4));
    BOOST_CHECK(begin + 4 < begin + 5);
    BOOST_CHECK(!(begin + 5 < begin + 4));
    BOOST_CHECK(it < end - 1);
    BOOST_CHECK(!(end - 1 < it));
}

void iterator_invalidation_test() {

#if !defined(NDEBUG) && !defined(BOOST_CB_DISABLE_DEBUG)

    circular_buffer<MyInteger>::iterator it1;
    circular_buffer<MyInteger>::const_iterator it2;
    circular_buffer<MyInteger>::iterator it3;
    circular_buffer<MyInteger>::const_iterator it4;
    circular_buffer<MyInteger>::const_iterator it5;
    circular_buffer<MyInteger>::const_iterator it6;

    BOOST_CHECK(it1.is_valid(0));
    BOOST_CHECK(it2.is_valid(0));
    BOOST_CHECK(it3.is_valid(0));
    BOOST_CHECK(it4.is_valid(0));
    BOOST_CHECK(it5.is_valid(0));
    BOOST_CHECK(it6.is_valid(0));

    {
        circular_buffer<MyInteger> cb(5, 0);
        const circular_buffer<MyInteger> ccb(5, 0);

        it1 = cb.begin();
        it2 = ccb.begin();
        it3 = cb.end();
        it4 = it1;
        it5 = it2;
        it6 = it1;

        BOOST_CHECK(it1.is_valid(&cb));
        BOOST_CHECK(it2.is_valid(&ccb));
        BOOST_CHECK(it3.is_valid(&cb));
        BOOST_CHECK(it4.is_valid(&cb));
        BOOST_CHECK(it5.is_valid(&ccb));
        BOOST_CHECK(it6.is_valid(&cb));
    }

    BOOST_CHECK(it1.is_valid(0));
    BOOST_CHECK(it2.is_valid(0));
    BOOST_CHECK(it3.is_valid(0));
    BOOST_CHECK(it4.is_valid(0));
    BOOST_CHECK(it5.is_valid(0));
    BOOST_CHECK(it6.is_valid(0));

    circular_buffer<MyInteger> cb(10, 0);
    it1 = cb.end();
    cb.clear();
    BOOST_CHECK(it1.is_valid(&cb));
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    int i = 0;
    for (it2 = cb.begin(); it2 != it1; it2++, i++);
    BOOST_CHECK(i == 3);

    circular_buffer<MyInteger> cb1(10, 0);
    circular_buffer<MyInteger> cb2(20, 0);
    it1 = cb1.end();
    it2 = cb2.begin();
    BOOST_CHECK(it1.is_valid(&cb1));
    BOOST_CHECK(it2.is_valid(&cb2));

    cb1.swap(cb2);
    BOOST_CHECK(!it1.is_valid(&cb1));
    BOOST_CHECK(!it2.is_valid(&cb2));

    it1 = cb1.begin() + 3;
    it2 = cb1.begin();
    cb1.push_back(1);
    BOOST_CHECK(it1.is_valid(&cb1));
    BOOST_CHECK(!it2.is_valid(&cb1));
    BOOST_CHECK(*it2.m_it == 1);

    circular_buffer<MyInteger> cb3(5);
    cb3.push_back(1);
    cb3.push_back(2);
    cb3.push_back(3);
    cb3.push_back(4);
    cb3.push_back(5);
    it1 = cb3.begin() + 2;
    it2 = cb3.begin();
    cb3.insert(cb3.begin() + 3, 6);
    BOOST_CHECK(it1.is_valid(&cb3));
    BOOST_CHECK(!it2.is_valid(&cb3));
    BOOST_CHECK(*it2.m_it == 5);

    it1 = cb3.begin() + 3;
    it2 = cb3.end() - 1;
    cb3.push_front(7);
    BOOST_CHECK(it1.is_valid(&cb3));
    BOOST_CHECK(!it2.is_valid(&cb3));
    BOOST_CHECK(*it2.m_it == 7);

    circular_buffer<MyInteger> cb4(5);
    cb4.push_back(1);
    cb4.push_back(2);
    cb4.push_back(3);
    cb4.push_back(4);
    cb4.push_back(5);
    it1 = cb4.begin() + 3;
    it2 = cb4.begin();
    cb4.rinsert(cb4.begin() + 2, 6);
    BOOST_CHECK(it1.is_valid(&cb4));
    BOOST_CHECK(!it2.is_valid(&cb4));
    BOOST_CHECK(*it2.m_it == 2);

    it1 = cb1.begin() + 5;
    it2 = cb1.end() - 1;
    cb1.pop_back();
    BOOST_CHECK(it1.is_valid(&cb1));
    BOOST_CHECK(!it2.is_valid(&cb1));

    it1 = cb1.begin() + 5;
    it2 = cb1.begin();
    cb1.pop_front();
    BOOST_CHECK(it1.is_valid(&cb1));
    BOOST_CHECK(!it2.is_valid(&cb1));

    circular_buffer<MyInteger> cb5(20, 0);
    it1 = cb5.begin() + 5;
    it2 = it3 = cb5.begin() + 15;
    cb5.erase(cb5.begin() + 10);
    BOOST_CHECK(it1.is_valid(&cb5));
    BOOST_CHECK(!it2.is_valid(&cb5));
    BOOST_CHECK(!it3.is_valid(&cb5));

    it1 = cb5.begin() + 1;
    it2 = it3 = cb5.begin() + 8;
    cb5.erase(cb5.begin() + 3, cb5.begin() + 7);
    BOOST_CHECK(it1.is_valid(&cb5));
    BOOST_CHECK(!it2.is_valid(&cb5));
    BOOST_CHECK(!it3.is_valid(&cb5));

    circular_buffer<MyInteger> cb6(20, 0);
    it4 = it1 = cb6.begin() + 5;
    it2 = cb6.begin() + 15;
    cb6.rerase(cb6.begin() + 10);
    BOOST_CHECK(!it1.is_valid(&cb6));
    BOOST_CHECK(!it4.is_valid(&cb6));
    BOOST_CHECK(it2.is_valid(&cb6));

    it4 = it1 = cb6.begin() + 1;
    it2 = cb6.begin() + 8;
    cb6.rerase(cb6.begin() + 3, cb6.begin() + 7);
    BOOST_CHECK(!it1.is_valid(&cb6));
    BOOST_CHECK(!it4.is_valid(&cb6));
    BOOST_CHECK(it2.is_valid(&cb6));

    circular_buffer<MyInteger> cb7(10, 1);
    cb7.push_back(2);
    cb7.push_back(3);
    cb7.push_back(4);
    it1 = cb7.end();
    it2 = cb7.begin();
    it3 = cb7.begin() + 6;
    cb7.linearize();
    BOOST_CHECK(it1.is_valid(&cb7));
    BOOST_CHECK(!it2.is_valid(&cb7));
    BOOST_CHECK(!it3.is_valid(&cb7));
    it1 = cb7.end();
    it2 = cb7.begin();
    it3 = cb7.begin() + 6;
    cb7.linearize();
    BOOST_CHECK(it1.is_valid(&cb7));
    BOOST_CHECK(it2.is_valid(&cb7));
    BOOST_CHECK(it3.is_valid(&cb7));

    cb7.push_back(5);
    cb7.push_back(6);
    it1 = cb7.end();
    it2 = cb7.begin();
    it3 = cb7.begin() + 6;
    cb7.set_capacity(10);
    BOOST_CHECK(it1.is_valid(&cb7));
    BOOST_CHECK(it2.is_valid(&cb7));
    BOOST_CHECK(it3.is_valid(&cb7));
    cb7.set_capacity(20);
    BOOST_CHECK(it1.is_valid(&cb7));
    BOOST_CHECK(!it2.is_valid(&cb7));
    BOOST_CHECK(!it3.is_valid(&cb7));
    cb7.push_back(7);
    it1 = cb7.end();
    it2 = cb7.begin();
    it3 = cb7.begin() + 6;
    cb7.set_capacity(10);
    BOOST_CHECK(it1.is_valid(&cb7));
    BOOST_CHECK(!it2.is_valid(&cb7));
    BOOST_CHECK(!it3.is_valid(&cb7));

    cb7.push_back(8);
    cb7.push_back(9);
    it1 = cb7.end();
    it2 = cb7.begin();
    it3 = cb7.begin() + 6;
    cb7.rset_capacity(10);
    BOOST_CHECK(it1.is_valid(&cb7));
    BOOST_CHECK(it2.is_valid(&cb7));
    BOOST_CHECK(it3.is_valid(&cb7));
    cb7.rset_capacity(20);
    BOOST_CHECK(it1.is_valid(&cb7));
    BOOST_CHECK(!it2.is_valid(&cb7));
    BOOST_CHECK(!it3.is_valid(&cb7));
    cb7.push_back(10);
    it1 = cb7.end();
    it2 = cb7.begin();
    it3 = cb7.begin() + 6;
    cb7.rset_capacity(10);
    BOOST_CHECK(it1.is_valid(&cb7));
    BOOST_CHECK(!it2.is_valid(&cb7));
    BOOST_CHECK(!it3.is_valid(&cb7));

    circular_buffer<MyInteger> cb8(10, 1);
    cb8.push_back(2);
    cb8.push_back(3);
    it1 = cb8.end();
    it2 = cb8.begin();
    it3 = cb8.begin() + 6;
    cb8.resize(10);
    BOOST_CHECK(it1.is_valid(&cb8));
    BOOST_CHECK(it2.is_valid(&cb8));
    BOOST_CHECK(it3.is_valid(&cb8));
    cb8.resize(20);
    BOOST_CHECK(it1.is_valid(&cb8));
    BOOST_CHECK(!it2.is_valid(&cb8));
    BOOST_CHECK(!it3.is_valid(&cb8));
    cb8.push_back(4);
    it1 = cb8.end();
    it2 = cb8.begin();
    it3 = cb8.begin() + 6;
    it4 = cb8.begin() + 12;
    cb8.resize(10);
    BOOST_CHECK(it1.is_valid(&cb8));
    BOOST_CHECK(it2.is_valid(&cb8));
    BOOST_CHECK(it3.is_valid(&cb8));
    BOOST_CHECK(!it4.is_valid(&cb8));

    cb8.set_capacity(10);
    cb8.push_back(5);
    cb8.push_back(6);
    it1 = cb8.end();
    it2 = cb8.begin();
    it3 = cb8.begin() + 6;
    cb8.rresize(10);
    BOOST_CHECK(it1.is_valid(&cb8));
    BOOST_CHECK(it2.is_valid(&cb8));
    BOOST_CHECK(it3.is_valid(&cb8));
    cb8.rresize(20);
    BOOST_CHECK(it1.is_valid(&cb8));
    BOOST_CHECK(!it2.is_valid(&cb8));
    BOOST_CHECK(!it3.is_valid(&cb8));
    cb8.push_back(7);
    it1 = cb8.end();
    it2 = cb8.begin();
    it3 = cb8.begin() + 6;
    it4 = cb8.begin() + 12;
    cb8.rresize(10);
    BOOST_CHECK(it1.is_valid(&cb8));
    BOOST_CHECK(!it2.is_valid(&cb8));
    BOOST_CHECK(!it3.is_valid(&cb8));
    BOOST_CHECK(it4.is_valid(&cb8));

    circular_buffer<MyInteger> cb9(15, 1);
    it1 = cb9.end();
    it2 = cb9.begin();
    it3 = cb9.begin() + 6;
    it4 = cb9.begin() + 12;
    cb9 = cb8;
    BOOST_CHECK(it1.is_valid(&cb9));
    BOOST_CHECK(!it2.is_valid(&cb9));
    BOOST_CHECK(!it3.is_valid(&cb9));
    BOOST_CHECK(!it4.is_valid(&cb9));

    circular_buffer<MyInteger> cb10(10, 1);
    it1 = cb10.end();
    it2 = cb10.begin();
    it3 = cb10.begin() + 3;
    it4 = cb10.begin() + 7;
    cb10.assign(5, 2);
    BOOST_CHECK(it1.is_valid(&cb10));
    BOOST_CHECK(!it2.is_valid(&cb10));
    BOOST_CHECK(!it3.is_valid(&cb10));
    BOOST_CHECK(!it4.is_valid(&cb10));

    circular_buffer<MyInteger> cb11(10, 1);
    it1 = cb11.end();
    it2 = cb11.begin();
    it3 = cb11.begin() + 3;
    it4 = cb11.begin() + 7;
    cb11.assign(15, 5, 2);
    BOOST_CHECK(it1.is_valid(&cb11));
    BOOST_CHECK(!it2.is_valid(&cb11));
    BOOST_CHECK(!it3.is_valid(&cb11));
    BOOST_CHECK(!it4.is_valid(&cb11));

    circular_buffer<MyInteger> cb12(10, 1);
    it1 = cb12.end();
    it2 = cb12.begin();
    it3 = cb12.begin() + 3;
    it4 = cb12.begin() + 7;
    cb12.assign(cb11.begin(), cb11.end());
    BOOST_CHECK(it1.is_valid(&cb12));
    BOOST_CHECK(!it2.is_valid(&cb12));
    BOOST_CHECK(!it3.is_valid(&cb12));
    BOOST_CHECK(!it4.is_valid(&cb12));

    circular_buffer<MyInteger> cb13(10, 1);
    it1 = cb13.end();
    it2 = cb13.begin();
    it3 = cb13.begin() + 3;
    it4 = cb13.begin() + 7;
    cb13.assign(15, cb11.begin(), cb11.end());
    BOOST_CHECK(it1.is_valid(&cb13));
    BOOST_CHECK(!it2.is_valid(&cb13));
    BOOST_CHECK(!it3.is_valid(&cb13));
    BOOST_CHECK(!it4.is_valid(&cb13));

    circular_buffer<MyInteger> cb14(10);
    cb14.push_back(1);
    cb14.push_back(2);
    cb14.push_back(3);
    cb14.push_back(4);
    cb14.push_back(5);
    cb14.push_back(6);
    cb14.push_back(7);
    it1 = cb14.end();
    it2 = cb14.begin() + 2;
    it3 = cb14.begin() + 1;
    it4 = cb14.begin() + 5;
    cb14.rotate(it2);
    BOOST_CHECK(it1.is_valid(&cb14));
    BOOST_CHECK(it2.is_valid(&cb14));
    BOOST_CHECK(!it3.is_valid(&cb14));
    BOOST_CHECK(it4.is_valid(&cb14));

    circular_buffer<MyInteger> cb15(7);
    cb15.push_back(1);
    cb15.push_back(2);
    cb15.push_back(3);
    cb15.push_back(4);
    cb15.push_back(5);
    cb15.push_back(6);
    cb15.push_back(7);
    cb15.push_back(8);
    cb15.push_back(9);
    it1 = cb15.end();
    it2 = cb15.begin() + 2;
    it3 = cb15.begin() + 1;
    it4 = cb15.begin() + 5;
    cb15.rotate(it3);
    BOOST_CHECK(it1.is_valid(&cb15));
    BOOST_CHECK(it2.is_valid(&cb15));
    BOOST_CHECK(it3.is_valid(&cb15));
    BOOST_CHECK(it4.is_valid(&cb15));

    circular_buffer<MyInteger> cb16(10);
    cb16.push_back(1);
    cb16.push_back(2);
    cb16.push_back(3);
    cb16.push_back(4);
    cb16.push_back(5);
    cb16.push_back(6);
    cb16.push_back(7);
    it1 = cb16.end();
    it2 = cb16.begin() + 6;
    it3 = cb16.begin();
    it4 = cb16.begin() + 5;
    cb16.rotate(it4);
    BOOST_CHECK(it1.is_valid(&cb16));
    BOOST_CHECK(!it2.is_valid(&cb16));
    BOOST_CHECK(it3.is_valid(&cb16));
    BOOST_CHECK(!it4.is_valid(&cb16));

#endif // #if !defined(NDEBUG) && !defined(BOOST_CB_DISABLE_DEBUG)
}

// basic exception safety test (it is useful to use any memory-leak detection tool)
void exception_safety_test() {

#if !defined(BOOST_NO_EXCEPTIONS)

    circular_buffer<MyInteger> cb1(3, 5);
    MyInteger::set_exception_trigger(3);
    BOOST_CHECK_THROW(cb1.set_capacity(5), std::exception);
    BOOST_CHECK(cb1.capacity() == 3);
    MyInteger::set_exception_trigger(3);
    BOOST_CHECK_THROW(cb1.rset_capacity(5), std::exception);
    BOOST_CHECK(cb1.capacity() == 3);
    generic_test(cb1);

    MyInteger::set_exception_trigger(3);
    BOOST_CHECK_THROW(circular_buffer<MyInteger> cb2(5, 10), std::exception);

    circular_buffer<MyInteger> cb3(5, 10);
    MyInteger::set_exception_trigger(3);
    BOOST_CHECK_THROW(circular_buffer<MyInteger> cb4(cb3), std::exception);

    vector<MyInteger> v(5, MyInteger(10));
    MyInteger::set_exception_trigger(3);
    BOOST_CHECK_THROW(circular_buffer<MyInteger> cb5(8, v.begin(), v.end()), std::exception);

    circular_buffer<MyInteger> cb6(5, 10);
    circular_buffer<MyInteger> cb7(8, 3);
    MyInteger::set_exception_trigger(3);
    BOOST_CHECK_THROW(cb7 = cb6, std::exception);
    BOOST_CHECK(cb7.size() == 8);
    BOOST_CHECK(cb7.capacity() == 8);
    BOOST_CHECK(cb7[0] == 3);
    BOOST_CHECK(cb7[7] == 3);
    generic_test(cb7);

    circular_buffer<MyInteger> cb8(5, 10);
    MyInteger::set_exception_trigger(2);
    BOOST_CHECK_THROW(cb8.push_front(1), std::exception);

    circular_buffer<MyInteger> cb9(5);
    cb9.push_back(1);
    cb9.push_back(2);
    cb9.push_back(3);
    MyInteger::set_exception_trigger(3);
    BOOST_CHECK_THROW(cb9.insert(cb9.begin() + 1, 4), std::exception);

    circular_buffer<MyInteger> cb10(5);
    cb10.push_back(1);
    cb10.push_back(2);
    cb10.push_back(3);
    MyInteger::set_exception_trigger(3);
    BOOST_CHECK_THROW(cb10.rinsert(cb10.begin() + 1, 4), std::exception);

    circular_buffer<MyInteger> cb11(5);
    cb11.push_back(1);
    cb11.push_back(2);
    MyInteger::set_exception_trigger(2);
    BOOST_CHECK_THROW(cb11.rinsert(cb11.begin(), 1), std::exception);

    circular_buffer<MyInteger> cb12(5, 1);
    MyInteger::set_exception_trigger(3);
    BOOST_CHECK_THROW(cb12.assign(4, 2), std::exception);

    circular_buffer<MyInteger> cb13(5, 1);
    MyInteger::set_exception_trigger(3);
    BOOST_CHECK_THROW(cb13.assign(6, 2), std::exception);

    circular_buffer<MyInteger> cb14(5);
    cb14.push_back(1);
    cb14.push_back(2);
    MyInteger::set_exception_trigger(3);
    BOOST_CHECK_THROW(cb14.insert(cb14.begin(), 10, 3), std::exception);

    circular_buffer<MyInteger> cb15(5);
    cb15.push_back(1);
    cb15.push_back(2);
    MyInteger::set_exception_trigger(3);
    BOOST_CHECK_THROW(cb15.insert(cb15.end(), 10, 3), std::exception);

    circular_buffer<MyInteger> cb16(5);
    cb16.push_back(1);
    cb16.push_back(2);
    MyInteger::set_exception_trigger(3);
    BOOST_CHECK_THROW(cb16.rinsert(cb16.begin(), 10, 3), std::exception);

    circular_buffer<MyInteger> cb17(5);
    cb17.push_back(1);
    cb17.push_back(2);
    MyInteger::set_exception_trigger(3);
    BOOST_CHECK_THROW(cb17.rinsert(cb17.end(), 10, 3), std::exception);

    circular_buffer<MyInteger> cb18(5, 0);
    cb18.push_back(1);
    cb18.push_back(2);
    cb18.pop_front();
    MyInteger::set_exception_trigger(4);
    BOOST_CHECK_THROW(cb18.linearize(), std::exception);

    circular_buffer<MyInteger> cb19(5, 0);
    cb19.push_back(1);
    cb19.push_back(2);
    MyInteger::set_exception_trigger(5);
    BOOST_CHECK_THROW(cb19.linearize(), std::exception);

    circular_buffer<MyInteger> cb20(5, 0);
    cb20.push_back(1);
    cb20.push_back(2);
    MyInteger::set_exception_trigger(6);
    BOOST_CHECK_THROW(cb20.linearize(), std::exception);

    circular_buffer<MyInteger> cb21(5);
    cb21.push_back(1);
    cb21.push_back(2);
    cb21.push_back(3);
    MyInteger::set_exception_trigger(2);
    BOOST_CHECK_THROW(cb21.insert(cb21.begin() + 1, 4), std::exception);

    circular_buffer<MyInteger> cb22(5);
    cb22.push_back(1);
    cb22.push_back(2);
    cb22.push_back(3);
    MyInteger::set_exception_trigger(2);
    BOOST_CHECK_THROW(cb22.insert(cb22.end(), 4), std::exception);

    circular_buffer<MyInteger> cb23(5, 0);
    MyInteger::set_exception_trigger(2);
    BOOST_CHECK_THROW(cb23.insert(cb23.begin() + 1, 4), std::exception);

    circular_buffer<MyInteger> cb24(5);
    cb24.push_back(1);
    cb24.push_back(2);
    cb24.push_back(3);
    MyInteger::set_exception_trigger(2);
    BOOST_CHECK_THROW(cb24.rinsert(cb24.begin() + 1, 4), std::exception);

    circular_buffer<MyInteger> cb25(5, 0);
    MyInteger::set_exception_trigger(2);
    BOOST_CHECK_THROW(cb25.rinsert(cb25.begin() + 3, 4), std::exception);

    circular_buffer<MyInteger> cb26(5);
    cb26.push_back(1);
    cb26.push_back(2);
    MyInteger::set_exception_trigger(5);
    BOOST_CHECK_THROW(cb26.insert(cb26.begin(), 10, 3), std::exception);

    circular_buffer<MyInteger> cb27(5);
    cb27.push_back(1);
    cb27.push_back(2);
    MyInteger::set_exception_trigger(5);
    BOOST_CHECK_THROW(cb27.insert(cb27.end(), 10, 3), std::exception);

    circular_buffer<MyInteger> cb28(5);
    cb28.push_back(1);
    cb28.push_back(2);
    MyInteger::set_exception_trigger(5);
    BOOST_CHECK_THROW(cb28.rinsert(cb28.begin(), 10, 3), std::exception);

    circular_buffer<MyInteger> cb29(5);
    cb29.push_back(1);
    cb29.push_back(2);
    MyInteger::set_exception_trigger(5);
    BOOST_CHECK_THROW(cb29.rinsert(cb29.end(), 10, 3), std::exception);

    circular_buffer<MyInteger> cb30(10);
    cb30.push_back(1);
    cb30.push_back(2);
    cb30.push_back(3);
    MyInteger::set_exception_trigger(2);
    BOOST_CHECK_THROW(cb30.rinsert(cb30.begin(), 10, 3), std::exception);

#endif // #if !defined(BOOST_NO_EXCEPTIONS)
}


void move_container_values_except() {
    move_container_values_impl<noncopyable_movable_except_t>();
}

template <class T>
void move_container_values_resetting_impl() {
    typedef T noncopyable_movable_test_t;    
    CB_CONTAINER<noncopyable_movable_test_t> cb1(1);
    noncopyable_movable_test_t var;
    cb1.push_back();

    cb1.push_back(boost::move(var));
    BOOST_CHECK(!cb1.back().is_moved());
    BOOST_CHECK(var.is_moved());
    BOOST_CHECK(cb1.size() == 1);
    var = boost::move(cb1.back());
    BOOST_CHECK(cb1.back().is_moved());

    cb1.push_front(boost::move(var));
    BOOST_CHECK(!cb1.front().is_moved());
    BOOST_CHECK(var.is_moved());
    BOOST_CHECK(cb1.size() == 1);
    var = boost::move(cb1.back());
    BOOST_CHECK(cb1.back().is_moved());

    cb1.push_back();
    BOOST_CHECK(!cb1.back().is_moved());
    BOOST_CHECK(cb1.size() == 1);
    var = boost::move(cb1.back());
    BOOST_CHECK(cb1.back().is_moved());

    cb1.push_front();
    BOOST_CHECK(!cb1.front().is_moved());
    BOOST_CHECK(cb1.size() == 1);
    var = boost::move(cb1.back());
    BOOST_CHECK(cb1.back().is_moved());


    cb1.insert(cb1.begin());
    // If the circular_buffer is full and the pos points to begin(), 
    // then the item will not be inserted.
    BOOST_CHECK(cb1.front().is_moved()); 
    BOOST_CHECK(cb1.size() == 1);
    var = boost::move(cb1.back());
    BOOST_CHECK(cb1.back().is_moved());

    cb1.insert(cb1.begin(), boost::move(var));
    // If the circular_buffer is full and the pos points to begin(), 
    // then the item will not be inserted.
    BOOST_CHECK(cb1.front().is_moved());
    BOOST_CHECK(cb1.size() == 1);
    var = boost::move(cb1.back());
    BOOST_CHECK(cb1.back().is_moved());

    cb1.rinsert(cb1.begin());
    BOOST_CHECK(!cb1.back().is_moved());
    BOOST_CHECK(cb1.size() == 1);
    var = boost::move(cb1.back());
    BOOST_CHECK(cb1.back().is_moved());

    var.reinit();
    cb1.rinsert(cb1.begin(), boost::move(var));
    BOOST_CHECK(!cb1.back().is_moved());
    BOOST_CHECK(cb1.size() == 1);
    var = boost::move(cb1.back());
    BOOST_CHECK(cb1.back().is_moved());
    
    cb1.rinsert(cb1.end());
    BOOST_CHECK(cb1.back().is_moved());
    BOOST_CHECK(cb1.size() == 1);
    var = boost::move(cb1.back());
    BOOST_CHECK(cb1.back().is_moved());

    var.reinit();
    cb1.rinsert(cb1.end(), boost::move(var));
    BOOST_CHECK(cb1.back().is_moved());
    BOOST_CHECK(cb1.size() == 1);
    var = boost::move(cb1.back());
    BOOST_CHECK(cb1.back().is_moved());
    cb1.push_back();
    BOOST_CHECK(!cb1[0].is_moved());

    const int val = cb1[0].value();
    cb1.linearize();
    BOOST_CHECK(!cb1[0].is_moved());
    BOOST_CHECK(cb1[0].value() == val);

    cb1.rotate(cb1.begin());
    BOOST_CHECK(!cb1[0].is_moved());
    BOOST_CHECK(cb1[0].value() == val);
}

void move_container_values_resetting_except() {
    move_container_values_resetting_impl<noncopyable_movable_except_t>();
}

void move_container_values_resetting_noexcept() {
    move_container_values_resetting_impl<noncopyable_movable_noexcept_t>();
}

// test main
test_suite* init_unit_test_suite(int /*argc*/, char* /*argv*/[]) {

    test_suite* tests = BOOST_TEST_SUITE("Unit tests for the circular_buffer.");
    add_common_tests(tests);

    tests->add(BOOST_TEST_CASE(&iterator_constructor_and_assign_test));
    tests->add(BOOST_TEST_CASE(&iterator_reference_test));
    tests->add(BOOST_TEST_CASE(&iterator_difference_test));
    tests->add(BOOST_TEST_CASE(&iterator_increment_test));
    tests->add(BOOST_TEST_CASE(&iterator_decrement_test));
    tests->add(BOOST_TEST_CASE(&iterator_addition_test));
    tests->add(BOOST_TEST_CASE(&iterator_subtraction_test));
    tests->add(BOOST_TEST_CASE(&iterator_element_access_test));
    tests->add(BOOST_TEST_CASE(&iterator_comparison_test));
    tests->add(BOOST_TEST_CASE(&iterator_invalidation_test));
    tests->add(BOOST_TEST_CASE(&exception_safety_test));
    tests->add(BOOST_TEST_CASE(&move_container_values_except));
    tests->add(BOOST_TEST_CASE(&move_container_values_resetting_except));
    tests->add(BOOST_TEST_CASE(&move_container_values_resetting_noexcept));

    return tests;
}

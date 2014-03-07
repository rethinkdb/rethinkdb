/*=============================================================================
    Copyright (c) 2010 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/adapted/adt/adapt_assoc_adt.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <iostream>
#include <string>

namespace fusion=boost::fusion;

template<typename Name, typename Age>
struct employee
{
private:
    Name name;
    Age age;

public:
    template<typename OtherName>
    void
    set_name(OtherName const& n)
    {
        name=n;
    }

    template<typename OtherAge>
    void
    set_age(OtherAge const& a)
    {
        age=a;
    }

    Name& get_name()
    {
        return name;
    }

    Name const& get_name()const
    {
        return name;
    }

    Age& get_age()
    {
        return age;
    }

    Age const& get_age()const
    {
        return age;
    }
};

namespace keys
{
    struct name;
    struct age;
}

BOOST_FUSION_ADAPT_ASSOC_TPL_ADT(
    (Name)(Age),
    (employee) (Name)(Age),
    (Name&, Name const&, obj.get_name(), obj.template set_name<Val>(val), keys::name)
        (Age&, Age const&, obj.get_age(), obj.template set_age<Val>(val), keys::age))

int main()
{
    typedef employee<std::string, int> et;
    typedef et const etc;
    et e;
    etc& ec=e;

    fusion::at_key<keys::name>(e)="marshall mathers";
    fusion::at_key<keys::age>(e)=37;

    BOOST_MPL_ASSERT((
        boost::is_same<
            boost::fusion::result_of::value_at_key<et, keys::name>::type,
            std::string
        >));
    BOOST_MPL_ASSERT((
        boost::is_same<
            boost::fusion::result_of::value_at_key<et, keys::name>::type,
            boost::fusion::result_of::value_at_c<et, 0>::type
        >));
    BOOST_MPL_ASSERT((
        boost::is_same<
            boost::fusion::result_of::value_at_key<et, keys::age>::type,
            int
        >));
    BOOST_MPL_ASSERT((
        boost::is_same<
            boost::fusion::result_of::value_at_key<et, keys::age>::type,
            boost::fusion::result_of::value_at_c<et, 1>::type
        >));

    BOOST_MPL_ASSERT((
        boost::is_same<
            boost::fusion::result_of::at_key<et, keys::name>::type,
            fusion::extension::adt_attribute_proxy<et, 0, false>
        >));
    BOOST_MPL_ASSERT((
        boost::is_same<
            boost::fusion::result_of::at_key<et, keys::age>::type,
            fusion::extension::adt_attribute_proxy<et, 1, false>
        >));
    BOOST_MPL_ASSERT((
        boost::is_same<
            boost::fusion::result_of::at_key<et, keys::name>::type,
            boost::fusion::result_of::front<et>::type
        >));
    BOOST_MPL_ASSERT((
        boost::is_same<
            boost::fusion::result_of::at_key<et, keys::age>::type,
            boost::fusion::result_of::back<et>::type
        >));

    BOOST_MPL_ASSERT((
        boost::is_same<
            boost::fusion::result_of::at_key<etc, keys::name>::type,
            fusion::extension::adt_attribute_proxy<et, 0, true>
        >));
    BOOST_MPL_ASSERT((
        boost::is_same<
            boost::fusion::result_of::at_key<etc, keys::age>::type,
            fusion::extension::adt_attribute_proxy<et, 1, true>
        >));
    BOOST_MPL_ASSERT((
        boost::is_same<
            boost::fusion::result_of::at_key<etc, keys::name>::type,
            boost::fusion::result_of::front<etc>::type
        >));
    BOOST_MPL_ASSERT((
        boost::is_same<
            boost::fusion::result_of::at_key<etc, keys::age>::type,
            boost::fusion::result_of::back<etc>::type
        >));

    BOOST_MPL_ASSERT((
        boost::is_same<
            fusion::extension::adt_attribute_proxy<et, 0, false>::type,
            std::string&
        >));
    BOOST_MPL_ASSERT((
        boost::is_same<
            fusion::extension::adt_attribute_proxy<et, 0, true>::type,
            std::string const&
        >));
    BOOST_MPL_ASSERT((
        boost::is_same<
            fusion::extension::adt_attribute_proxy<et, 1, false>::type,
            int&
        >));
    BOOST_MPL_ASSERT((
        boost::is_same<
            fusion::extension::adt_attribute_proxy<et, 1, true>::type,
            int const&
        >));

    {
        std::string& name=fusion::at_key<keys::name>(e);
        int& age=fusion::at_key<keys::age>(e);
        BOOST_TEST(name=="marshall mathers");
        BOOST_TEST(age==37);
        BOOST_TEST(fusion::at_key<keys::name>(e).get()=="marshall mathers");
        BOOST_TEST(fusion::at_key<keys::age>(e).get()==37);
        BOOST_TEST(fusion::front(e).get()=="marshall mathers");
        BOOST_TEST(fusion::back(e).get()==37);
    }

    {
        std::string const& name=fusion::at_key<keys::name>(ec);
        int const& age=fusion::at_key<keys::age>(ec);
        BOOST_TEST(name=="marshall mathers");
        BOOST_TEST(age==37);
        BOOST_TEST(fusion::at_key<keys::name>(ec).get()=="marshall mathers");
        BOOST_TEST(fusion::at_key<keys::age>(ec).get()==37);
        BOOST_TEST(fusion::front(ec).get()=="marshall mathers");
        BOOST_TEST(fusion::back(ec).get()==37);
    }
}

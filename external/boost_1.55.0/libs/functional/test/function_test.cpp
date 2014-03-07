// ------------------------------------------------------------------------------
// Copyright (c) 2000 Cadenza New Zealand Ltd
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
// ------------------------------------------------------------------------------
// Tests for the Boost functional.hpp header file
//
// Note that functional.hpp relies on partial specialisation to be
// effective.  If your compiler lacks this feature, very few of the
// tests would compile, and so have been excluded from the test.
// ------------------------------------------------------------------------------
// $Id: function_test.cpp 50456 2009-01-04 05:17:02Z bgubenko $
// ------------------------------------------------------------------------------
// $Log$
// Revision 1.3  2006/12/02 13:57:32  andreas_huber69
// Fixed license & copyright issues.
//
// From Mark Rodgers Fri Dec 1 12:59:14 2006
// X-Apparently-To: ahd6974-boostorg -at- yahoo.com via 68.142.206.160; Fri, 01 Dec 2006 12:59:41 -0800
// X-Originating-IP: [195.112.4.54]
// Return-Path: <mark.rodgers -at- cadenza.co.nz>
// Authentication-Results: mta550.mail.mud.yahoo.com from=cadenza.co.nz; domainkeys=neutral (no sig)
// Received: from 195.112.4.54 (EHLO smtp.nildram.co.uk) (195.112.4.54) by mta550.mail.mud.yahoo.com with SMTP; Fri, 01 Dec 2006 12:59:40 -0800
// Received: from snagglepuss.cadenza.co.nz (81-6-246-87.dyn.gotadsl.co.uk [81.6.246.87]) by smtp.nildram.co.uk (Postfix) with ESMTP id D32EA2B6D8C for <ahd6974-boostorg -at- yahoo.com>; Fri, 1 Dec 2006 20:59:35 +0000 (GMT)
// Received: from penfold.cadenza.co.nz ([192.168.55.56]) by snagglepuss.cadenza.co.nz with esmtp (Exim 4.63) (envelope-from <mark.rodgers -at- cadenza.co.nz>) id J9M4Y9-0009TO-9K for ahd6974-boostorg -at- yahoo.com; Fri, 01 Dec 2006 20:58:57 +0000
// Message-ID: <457097A2.1090305@cadenza.co.nz>
// Date: Fri, 01 Dec 2006 20:59:14 +0000
// From: "Mark Rodgers" <mark.rodgers -at- cadenza.co.nz>
// User-Agent: Thunderbird 1.5.0.8 (Macintosh/20061025)
// MIME-Version: 1.0
// To: ahd6974-boostorg -at- yahoo.com [Edit - Delete]
// Subject: Re: [boost] Reminder: Need your permission to correct license & copyright issues
// References: <379990.36007.qm@web33507.mail.mud.yahoo.com>
// In-Reply-To: <379990.36007.qm@web33507.mail.mud.yahoo.com>
// Content-Type: text/plain; charset=ISO-8859-1; format=flowed
// Content-Transfer-Encoding: 7bit
// Content-Length: 812
// Gidday Andreas
//
// Sure that's fine.  I'm happy for you to do 1, 2 and 3.
//
// Regards
// Mark
//
// Andreas Huber wrote:
// > Hello Mark
// >
// > Quite a while ago it was decided that every file that goes into the
// > 1.34 release of the Boost distribution (www.boost.org) needs uniform
// > license and copyright information. For more information please see:
// >
// > <http://www.boost.org/more/license_info.html>
// >
// > You are receiving this email because several files you contributed
// > lack such information or have an old license:
// >
// > boost/functional/functional.hpp
// > boost/libs/functional/binders.html
// > boost/libs/functional/function_test.cpp
// > boost/libs/functional/function_traits.html
// > boost/libs/functional/index.html
// > boost/libs/functional/mem_fun.html
// > boost/libs/functional/negators.html
// > boost/libs/functional/ptr_fun.html
// > boost/people/mark_rodgers.htm
// >
// > I therefore kindly ask you to grant the permission to do the
// > following:
// >
// > 1. For the files above that already have a license text (all except
// > mark_rodgers.htm), replace the license text with:
// >
// > "Distributed under the Boost Software License, Version 1.0. (See
// > accompanying file LICENSE_1_0.txt or copy at
// > http://www.boost.org/LICENSE_1_0.txt)"
// >
// > 2. For the file that does not yet have a license and copyright
// > (mark_rodgers.htm) add the same license text as under 1. and add the
// > following copyright:
// >
// > "(c) Copyright Mark Rodgers 2000"
// >
// > 3. (Optional) I would also want to convert all HTML files to conform
// > the HTML 4.01 Standard by running them through HTML Tidy, see
// > <http://tidy.sf.net>
// >
// > It would be great if you could grant me permission to do 1 & 2 and
// > optionally also 3.
// >
// > Thank you!
// >
// > Regards,
// >
// > Andreas Huber
// >
//
// Revision 1.2  2001/09/22 11:52:24  johnmaddock
// Intel C++ fixes: no void return types supported.
//
// Revision 1.1.1.1  2000/07/07 16:04:18  beman
// 1.16.1 initial CVS checkin
//
// Revision 1.3  2000/06/26 09:44:01  mark
// Updated following feedback from Jens Maurer.
//
// Revision 1.2  2000/05/17 08:31:45  mark
// Added extra tests now that function traits work correctly.
// For compilers with no support for partial specialisation,
// excluded tests that won't work.
//
// Revision 1.1  2000/05/07 09:14:41  mark
// Initial revision
// ------------------------------------------------------------------------------

// To demonstrate what the boosted function object adapters do for
// you, try compiling with USE_STD defined.  This will endeavour to
// use the standard function object adapters, but is likely to result
// in numerous errors due to the fact that you cannot have references
// to references.
#ifdef USE_STD
#include <functional>
#define boost std
#else
#include <boost/functional.hpp>
#endif

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

class Person
{
  public:
    Person() {}
    Person(const char *n) : name(n) {}

    const std::string &get_name() const { return name; }
    void print(std::ostream &os) const { os << name << " "; }
    void set_name(const std::string &n) { name = n; std::cout << name << " "; }
    std::string clear_name() { std::string ret = name; name = ""; return ret; }
    void do_something(int) const {}

    bool is_fred() const { return name == "Fred"; }
    
  private:
    std::string name;
};

namespace
{
    bool is_equal(const std::string &s1, const std::string &s2)
    {
        return s1 == s2;
    }

    bool is_betty(const std::string &s)
    {
        return s == "Betty";
    }

    void do_set_name(Person *p, const std::string &name)
    {
        p->set_name(name);
    }
    
    void do_set_name_ref(Person &p, const std::string &name)
    {
        p.set_name(name);
    }
}

int main()
{
    std::vector<Person> v1;
    v1.push_back("Fred");
    v1.push_back("Wilma");
    v1.push_back("Barney");
    v1.push_back("Betty");

    const std::vector<Person> cv1(v1.begin(), v1.end());

    std::vector<std::string> v2;
    v2.push_back("Fred");
    v2.push_back("Wilma");
    v2.push_back("Barney");
    v2.push_back("Betty");

    Person person;
    Person &r = person;

    Person fred("Fred");
    Person wilma("Wilma");
    Person barney("Barney");
    Person betty("Betty");
    std::vector<Person*> v3;
    v3.push_back(&fred);
    v3.push_back(&wilma);
    v3.push_back(&barney);
    v3.push_back(&betty);

    const std::vector<Person*> cv3(v3.begin(), v3.end());
    std::vector<const Person*> v3c(v3.begin(), v3.end());

    std::ostream &os = std::cout;

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) && !defined(__ICL)
    // unary_traits, unary_negate
    std::transform(v2.begin(), v2.end(),
                   std::ostream_iterator<bool>(std::cout, " "),
                   boost::not1(is_betty));

    std::cout << '\n';
    std::transform(v1.begin(), v1.end(),
                   std::ostream_iterator<bool>(std::cout, " "),
                   boost::not1(boost::mem_fun_ref(&Person::is_fred)));

    // binary_traits, binary_negate
    std::cout << '\n';
    std::transform(v2.begin(), v2.end(),
                   std::ostream_iterator<bool>(std::cout, " "),
                   boost::bind1st(boost::not2(is_equal), "Betty"));

    std::cout << '\n';
    std::transform(v2.begin(), v2.end(),
                   std::ostream_iterator<bool>(std::cout, " "),
                   boost::bind2nd(boost::not2(is_equal), "Betty"));

    // pointer_to_unary_function
    std::cout << '\n';
    std::transform(v2.begin(), v2.end(),
                   std::ostream_iterator<bool>(std::cout, " "),
                   boost::not1(boost::ptr_fun(is_betty)));

    // binary_traits, bind1st, bind2nd
    std::cout << '\n';
    std::transform(v2.begin(), v2.end(),
                   std::ostream_iterator<bool>(std::cout, " "),
                   boost::bind1st(is_equal, "Betty"));

    std::cout << '\n';
    std::transform(v2.begin(), v2.end(),
                   std::ostream_iterator<bool>(std::cout, " "),
                   boost::bind2nd(is_equal, "Betty"));

    // pointer_to_binary_function, bind1st
    std::cout << '\n';
    std::for_each(v2.begin(), v2.end(), boost::bind1st(boost::ptr_fun(do_set_name), &person));

    std::cout << '\n';
    std::for_each(v2.begin(), v2.end(), boost::bind1st(boost::ptr_fun(do_set_name_ref), person));

    std::cout << '\n';
    std::for_each(v2.begin(), v2.end(), boost::bind1st(boost::ptr_fun(do_set_name_ref), r));

    // binary_traits
    std::cout << '\n';
    std::for_each(v2.begin(), v2.end(), boost::bind1st(do_set_name, &person));

    std::cout << '\n';
    std::for_each(v2.begin(), v2.end(), boost::bind1st(do_set_name_ref, person));

    std::cout << '\n';
    std::for_each(v2.begin(), v2.end(), boost::bind1st(do_set_name_ref, r));
#endif

    // const_mem_fun_t
    std::cout << '\n';
    std::transform(v3.begin(), v3.end(),
                   std::ostream_iterator<std::string>(std::cout, " "),
                   boost::mem_fun(&Person::get_name));

    std::cout << '\n';
    std::transform(cv3.begin(), cv3.end(),
                   std::ostream_iterator<std::string>(std::cout, " "),
                   boost::mem_fun(&Person::get_name));

    std::cout << '\n';
    std::transform(v3c.begin(), v3c.end(),
                   std::ostream_iterator<std::string>(std::cout, " "),
                   boost::mem_fun(&Person::get_name));

    // const_mem_fun_ref_t
    std::cout << '\n';
    std::transform(v1.begin(), v1.end(),
                   std::ostream_iterator<std::string>(std::cout, " "),
                   boost::mem_fun_ref(&Person::get_name));

    std::cout << '\n';
    std::transform(cv1.begin(), cv1.end(),
                   std::ostream_iterator<std::string>(std::cout, " "),
                   boost::mem_fun_ref(&Person::get_name));

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
    // const_mem_fun1_t, bind2nd
    std::cout << '\n';
    std::for_each(v3.begin(), v3.end(), boost::bind2nd(boost::mem_fun(&Person::print), std::cout));

    std::cout << '\n';
    std::for_each(v3.begin(), v3.end(), boost::bind2nd(boost::mem_fun(&Person::print), os));

    // const_mem_fun1_ref_t, bind2nd
    std::cout << '\n';
    std::for_each(v1.begin(), v1.end(), boost::bind2nd(boost::mem_fun_ref(&Person::print), std::cout));

    std::cout << '\n';
    std::for_each(v1.begin(), v1.end(), boost::bind2nd(boost::mem_fun_ref(&Person::print), os));

    // mem_fun1_t, bind1st
    std::cout << '\n';
    std::for_each(v2.begin(), v2.end(), boost::bind1st(boost::mem_fun(&Person::set_name), &person));

    // mem_fun1_ref_t, bind1st
    std::cout << '\n';
    std::for_each(v2.begin(), v2.end(), boost::bind1st(boost::mem_fun_ref(&Person::set_name), person));

    std::cout << '\n';
    std::for_each(v2.begin(), v2.end(), boost::bind1st(boost::mem_fun_ref(&Person::set_name), r));
#endif

    // mem_fun_t
    std::cout << '\n';
    std::transform(v3.begin(), v3.end(), std::ostream_iterator<std::string>(std::cout, " "),
                   boost::mem_fun(&Person::clear_name));
    
    // mem_fun_ref_t
    std::cout << '\n';
    std::transform(v1.begin(), v1.end(), std::ostream_iterator<std::string>(std::cout, " "),
                   boost::mem_fun_ref(&Person::clear_name));    

    std::cout << '\n';
    return 0;
}

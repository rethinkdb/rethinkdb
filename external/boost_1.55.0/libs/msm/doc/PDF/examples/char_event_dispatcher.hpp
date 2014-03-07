// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_CHAR_EVENT_DISPATCHER_HPP
#define BOOST_MSM_CHAR_EVENT_DISPATCHER_HPP

#include <boost/msm/back/common_types.hpp>

struct digit {};
struct char_0 : public digit {};
struct char_1 : public digit {};
struct char_2 : public digit {};
struct char_3 : public digit {};
struct char_4 : public digit {};
struct char_5 : public digit {};
struct char_6 : public digit {};
struct char_7 : public digit {};
struct char_8 : public digit {};
struct char_9 : public digit {};
struct minus_char {};
template <char c>
struct event_char{};
template <>
struct event_char<'0'> : public digit{};
template <>
struct event_char<'1'> : public digit{};
template <>
struct event_char<'2'> : public digit{};
template <>
struct event_char<'3'> : public digit{};
template <>
struct event_char<'4'> : public digit{};
template <>
struct event_char<'5'> : public digit{};
template <>
struct event_char<'6'> : public digit{};
template <>
struct event_char<'7'> : public digit{};
template <>
struct event_char<'8'> : public digit{};
template <>
struct event_char<'9'> : public digit{};

namespace boost { namespace msm { namespace back {


template <class Fsm>
struct char_event_dispatcher
{
    template <char c>
    struct dispatch_event_helper
    {
        static execute_return apply(Fsm& fsm)
        {
            return fsm.process_event(event_char<c>());
        }
    };
    char_event_dispatcher()
    {
        entries[0x30]=&dispatch_event_helper<'0'>::apply;
        entries[0x31]=&dispatch_event_helper<'1'>::apply;
        entries[0x32]=&dispatch_event_helper<'2'>::apply;
        entries[0x33]=&dispatch_event_helper<'3'>::apply;
        entries[0x34]=&dispatch_event_helper<'4'>::apply;
        entries[0x35]=&dispatch_event_helper<'5'>::apply;
        entries[0x36]=&dispatch_event_helper<'6'>::apply;
        entries[0x37]=&dispatch_event_helper<'7'>::apply;
        entries[0x38]=&dispatch_event_helper<'8'>::apply;
        entries[0x39]=&dispatch_event_helper<'9'>::apply;
        entries[0x2D]=&dispatch_event_helper<'-'>::apply;
        entries[0x2B]=&dispatch_event_helper<'+'>::apply;
    }
    execute_return process_event(Fsm& fsm,char c) const
    {
        return entries[c](fsm);
    }

    // The singleton instance.
    static const char_event_dispatcher instance;
    typedef execute_return (*cell)(Fsm&);
    cell entries[255];
};

}}} // boost::msm::back
#endif //BOOST_MSM_CHAR_EVENT_DISPATCHER_HPP

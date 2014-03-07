// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef EMPTY_HPP
#define EMPTY_HPP

// back-end
#include <boost/msm/back/state_machine.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/row2.hpp>

#include "Events.hpp"

struct Open;

namespace msm = boost::msm;
namespace mpl = boost::mpl;

struct Empty : public msm::front::state<> 
{
    template <class Event,class FSM>
    void on_entry(Event const&,FSM& ) {std::cout << "entering: Empty" << std::endl;}
    template <class Event,class FSM>
    void on_exit(Event const&,FSM& ) {std::cout << "leaving: Empty" << std::endl;}
    void open_drawer(open_close const&);

    struct internal_transition_table : mpl::vector<
        //              Start     Event         Next      Action                       Guard
        //+-------------+---------+-------------+---------+---------------------------+----------------------+
    msm::front::a_row2  < Empty   , open_close  , Open    , Empty,&Empty::open_drawer                        >
    //+-------------+---------+-------------+---------+---------------------------+----------------------+
    > {};
};

#endif

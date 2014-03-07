// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef OPEN_HPP
#define OPEN_HPP

// back-end
#include <boost/msm/back/state_machine.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/row2.hpp>

#include "Events.hpp"

struct Empty;

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;

struct Open : public msm::front::state<> 
{ 
    template <class Event,class FSM>
    void on_entry(Event const& ,FSM&) {std::cout << "entering: Open" << std::endl;}
    template <class Event,class FSM>
    void on_exit(Event const&,FSM& ) {std::cout << "leaving: Open" << std::endl;}
    void close_drawer(open_close const&);

    struct internal_transition_table : mpl::vector<
        //               Start     Event         Next      Action                      Guard
        //+-------------+---------+-------------+---------+---------------------------+----------------------+
    msm::front::a_row2  < Open    , open_close  , Empty   , Open,&Open::close_drawer                         >
        //+-------------+---------+-------------+---------+---------------------------+----------------------+
    > {};
};

#endif

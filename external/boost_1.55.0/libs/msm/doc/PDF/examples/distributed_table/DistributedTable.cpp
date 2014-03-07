// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <vector>
#include <iostream>
// back-end
#include <boost/msm/back/state_machine.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>
// functors
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>

#include "Empty.hpp"
#include "Open.hpp"

using namespace std;
namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;


// front-end: define the FSM structure 
struct player_ : public msm::front::state_machine_def<player_>
{
    // the initial state of the player SM. Must be defined
    typedef Empty initial_state;
    typedef mpl::vector<Empty,Open> explicit_creation;

    typedef player_ p; // makes transition table cleaner

    // Replaces the default no-transition response.
    template <class FSM,class Event>
    void no_transition(Event const& e, FSM&,int state)
    {
        std::cout << "no transition from state " << state
            << " on event " << typeid(e).name() << std::endl;
    }
};
// Pick a back-end
typedef msm::back::state_machine<player_> player;

//
// Testing utilities.
//
static char const* const state_names[] = { "Empty", "Open" };
void pstate(player const& p)
{
    std::cout << " -> " << state_names[p.current_state()[0]] << std::endl;
}

void test()
{        
    player p;
    // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
    p.start(); 
    // go to Open, call on_exit on Empty, then action, then on_entry on Open
    p.process_event(open_close()); pstate(p);
    p.process_event(open_close()); pstate(p);
}

int main()
{
    test();
    return 0;
}


// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

// back-end
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/back/mpl_graph_fsm_check.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::back;
namespace
{
    // events
    struct play {};
    struct end_pause {};
    struct stop {};
    struct pause {};
    struct open_close {};
    struct cd_detected{};
    struct error_found {};

    // front-end: define the FSM structure 
    struct player_ : public msm::front::state_machine_def<player_>
    {
        // The list of FSM states
        struct Empty : public msm::front::state<> 
        {
        };
        struct Open : public msm::front::state<> 
        {	 
        };

        // sm_ptr still supported but deprecated as functors are a much better way to do the same thing
        struct Stopped : public msm::front::state<> 
        {	 
        };

        struct Playing : public msm::front::state<>
        {
        };

        // state not defining any entry or exit
        struct Paused : public msm::front::state<>
        {
        };
        struct AllOk : public msm::front::state<>
        {
        };
        struct ErrorMode : public msm::front::state<>
        {
        };
        struct State1 : public msm::front::state<>
        {
        };
        struct State2 : public msm::front::state<>
        {
        };
        // the initial state of the player SM. Must be defined
        typedef mpl::vector<Empty,AllOk> initial_state;

        // Transition table for player
        struct transition_table : mpl::vector<
            //    Start     Event         Next      Action				 Guard
            //  +---------+-------------+---------+---------------------+----------------------+
           // adding this line makes non-reachable states and should cause a static assert
           //_row < State1  , open_close  , State2                                                >,
           // adding this line makes non-orthogonal regions and should cause a static assert
           //_row < Paused  , error_found , ErrorMode                                             >,
           _row < Stopped , play        , Playing                                               >,
           _row < Stopped , open_close  , Open                                                  >,
           _row < Stopped , stop        , Stopped                                               >,
            //  +---------+-------------+---------+---------------------+----------------------+
           _row < Open    , open_close  , Empty                                                 >,
            //  +---------+-------------+---------+---------------------+----------------------+
           _row < Empty   , open_close  , Open                                                  >,
           _row < Empty   , cd_detected , Stopped                                               >,
           _row < Empty   , cd_detected , Playing                                               >,
            //  +---------+-------------+---------+---------------------+----------------------+
           _row < Playing , stop        , Stopped                                               >,
           _row < Playing , pause       , Paused                                                >,
           _row < Playing , open_close  , Open                                                  >,
            //  +---------+-------------+---------+---------------------+----------------------+
           _row < Paused  , end_pause   , Playing                                               >,
           _row < Paused  , stop        , Stopped                                               >,
           _row < Paused  , open_close  , Open                                                  >,
           _row < AllOk   , error_found , ErrorMode                                             >
            //  +---------+-------------+---------+---------------------+----------------------+
        > {};
        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
        }
    };
    // Pick a back-end
    typedef msm::back::state_machine<player_,msm::back::mpl_graph_fsm_check> player;

    void test()
    {        
		player p;
    }
}

int main()
{
    test();
    return 0;
}

// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MENU_MODE_HPP
#define MENU_MODE_HPP

#include <iostream>
#include <boost/any.hpp>

#include "Events.hpp"
#include <boost/msm/back/favor_compile_time.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>

using namespace std;
namespace msm = boost::msm;

struct MenuMode_ : public msm::front::state_machine_def<MenuMode_>
{
    typedef mpl::vector1<MenuActive>        flag_list;
    struct WaitingForSongChoice : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {std::cout << "starting: MenuMode::WaitingForSongChoice" << std::endl;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "finishing: MenuMode::WaitingForSongChoice" << std::endl;}
    };
    struct StartCurrentSong : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {std::cout << "starting: MenuMode::StartCurrentSong" << std::endl;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "finishing: MenuMode::StartCurrentSong" << std::endl;}
    };
    struct MenuExit : public msm::front::exit_pseudo_state<CloseMenu>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {std::cout << "starting: MenuMode::WaitingForSongChoice" << std::endl;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "finishing: MenuMode::WaitingForSongChoice" << std::endl;}
    };
    typedef WaitingForSongChoice initial_state;
    typedef MenuMode_ fsm; // makes transition table cleaner
    // Transition table for player
    struct transition_table : mpl::vector2<
        //     Start                 Event           Next                Action                   Guard
        //    +---------------------+------------------+-------------------+---------------------+----------------------+
        _row < WaitingForSongChoice , MenuMiddleButton , StartCurrentSong                                               >,
        _row < StartCurrentSong     , SelectSong       , MenuExit                                                       >
        //    +---------------------+------------------+-------------------+---------------------+----------------------+
    > {};
};
typedef msm::back::state_machine<MenuMode_> MenuMode;

#endif

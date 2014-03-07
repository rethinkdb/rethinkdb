// Copyright 2010 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/statechart/event.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/transition.hpp>
#include "boost/mpl/list.hpp"

#include <vector>

#include <iostream>
#ifdef WIN32
#include "windows.h"
#else
#include <sys/time.h>
#endif

namespace sc = boost::statechart;
namespace mpl = boost::mpl;

namespace test_sc
{

    //events
    struct play : sc::event< play > {};
    struct end_pause : sc::event< end_pause > {};
    struct stop : sc::event< stop > {};
    struct pause : sc::event< pause > {};
    struct open_close : sc::event< open_close > {};
    struct cd_detected : sc::event< cd_detected > {};


    struct Empty;
    struct Open;
    struct Stopped;
    struct Playing;
    struct Paused;
    // SM
    struct player : sc::state_machine< player, Empty > 
    {
        void open_drawer(open_close const&)         { /*std::cout << "player::open_drawer\n";*/ }
        void store_cd_info(cd_detected const& cd)   {/*std::cout << "player::store_cd_info\n";*/ }
        void close_drawer(open_close const&)        { /*std::cout << "player::close_drawer\n";*/ }
        void start_playback(play const&)            { /*std::cout << "player::start_playback\n";*/ }
        void stopped_again(stop const&)             {/*std::cout << "player::stopped_again\n";*/}
        void stop_playback(stop const&)             { /*std::cout << "player::stop_playback\n";*/ }
        void pause_playback(pause const&)           { /*std::cout << "player::pause_playback\n"; */}
        void stop_and_open(open_close const&)       { /*std::cout << "player::stop_and_open\n";*/ }
        void resume_playback(end_pause const&)      { /*std::cout << "player::resume_playback\n";*/ }
    };

    struct Empty : sc::simple_state< Empty, player >
    {
        Empty() { /*std::cout << "entering Empty" << std::endl;*/ } // entry
        ~Empty() { /*std::cout << "leaving Empty" << std::endl;*/ } // exit
        typedef mpl::list<
            sc::transition< open_close, Open,
            player, &player::open_drawer >,
            sc::transition< cd_detected, Stopped,
            player, &player::store_cd_info > > reactions;

    };
    struct Open : sc::simple_state< Open, player >
    {
        Open() { /*std::cout << "entering Open" << std::endl;*/ } // entry
        ~Open() { /*std::cout << "leaving Open" << std::endl;*/ } // exit
        typedef sc::transition< open_close, Empty,
            player, &player::close_drawer > reactions;

    };
    struct Stopped : sc::simple_state< Stopped, player >
    {
        Stopped() { /*std::cout << "entering Stopped" << std::endl;*/ } // entry
        ~Stopped() { /*std::cout << "leaving Stopped" << std::endl;*/ } // exit
        typedef mpl::list<
            sc::transition< play, Playing,
            player, &player::start_playback >,
            sc::transition< open_close, Open,
            player, &player::open_drawer >, 
            sc::transition< stop, Stopped,
            player, &player::stopped_again > > reactions;

    };
    struct Playing : sc::simple_state< Playing, player >
    {
        Playing() { /*std::cout << "entering Playing" << std::endl;*/ } // entry
        ~Playing() { /*std::cout << "leaving Playing" << std::endl;*/ } // exit
        typedef mpl::list<
            sc::transition< stop, Stopped,
            player, &player::stop_playback >,
            sc::transition< pause, Paused,
            player, &player::pause_playback >, 
            sc::transition< open_close, Open,
            player, &player::stop_and_open > > reactions;
    };
    struct Paused : sc::simple_state< Paused, player >
    {
        Paused() { /*std::cout << "entering Paused" << std::endl;*/ } // entry
        ~Paused() { /*std::cout << "leaving Paused" << std::endl;*/ } // exit
        typedef mpl::list<
            sc::transition< end_pause, Playing,
            player, &player::resume_playback >,
            sc::transition< stop, Stopped,
            player, &player::stop_playback >, 
            sc::transition< open_close, Open,
            player, &player::stop_and_open > > reactions;
    };
}


#ifndef WIN32
long mtime(struct timeval& tv1,struct timeval& tv2)
{
    return (tv2.tv_sec-tv1.tv_sec) *1000000 + ((tv2.tv_usec-tv1.tv_usec));
}
#endif


int main()
{
    test_sc::player p;
    p.initiate();
    // for timing
#ifdef WIN32
    LARGE_INTEGER res;
    ::QueryPerformanceFrequency(&res);
    LARGE_INTEGER li,li2;
    ::QueryPerformanceCounter(&li);
#else
    struct timeval tv1,tv2;
    gettimeofday(&tv1,NULL);
#endif
    for (int i=0;i<100;++i)
    {
        p.process_event(test_sc::open_close());
        p.process_event(test_sc::open_close()); 
        p.process_event(test_sc::cd_detected());
        p.process_event(test_sc::play());      
        p.process_event(test_sc::pause()); 
        // go back to Playing
        p.process_event(test_sc::end_pause()); 
        p.process_event(test_sc::pause()); 
        p.process_event(test_sc::stop());  
        // event leading to the same state
        p.process_event(test_sc::stop());
        p.process_event(test_sc::open_close());
        p.process_event(test_sc::open_close());
    }
#ifdef WIN32
    ::QueryPerformanceCounter(&li2);
#else
    gettimeofday(&tv2,NULL);
#endif
#ifdef WIN32
    std::cout << "sc took in s:" << (double)(li2.QuadPart-li.QuadPart)/res.QuadPart <<"\n" <<std::endl;
#else
    std::cout << "sc took in us:" << mtime(tv1,tv2) <<"\n" <<std::endl;
#endif
    return 0;
}


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
//front-end
#include <boost/msm/front/state_machine_def.hpp>
#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>

namespace msm = boost::msm;
namespace mpl = boost::mpl;


namespace
{
    struct SomeExternalContext
    {
        SomeExternalContext(int b):bla(b){}
        int bla;
    };
    // events
    struct play {};
    struct end_pause {};
    struct stop {};
    struct pause {};
    struct open_close {};
    struct NextSong {};
    struct PreviousSong {};

    // A "complicated" event type that carries some data.
    enum DiskTypeEnum
    {
        DISK_CD=0,
        DISK_DVD=1
    };
    struct cd_detected
    {
        cd_detected(std::string name, DiskTypeEnum diskType)
            : name(name),
            disc_type(diskType)
        {}

        std::string name;
        DiskTypeEnum disc_type;
    };

    // front-end: define the FSM structure 
    struct player_ : public msm::front::state_machine_def<player_>
    {
        player_(SomeExternalContext& context,int someint)
            :context_(context)
        {
            BOOST_CHECK_MESSAGE(context_.bla == 3,"Wrong context value");
            BOOST_CHECK_MESSAGE(someint == 5,"Wrong int value");
            context.bla = 10;
        }

        SomeExternalContext& context_;

        // The list of FSM states
        struct Empty : public msm::front::state<> 
        {
            int data_;
            Empty():data_(0){}
            Empty(int i):data_(i){}
            // every (optional) entry/exit methods get the event passed.
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Empty" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Empty" << std::endl;}
        };
        struct Open : public msm::front::state<> 
        {	 
            int data_;
            Open():data_(0){}
            Open(int i):data_(i){}

            template <class Event,class FSM>
            void on_entry(Event const& ,FSM&) {std::cout << "entering: Open" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Open" << std::endl;}
        };

        // sm_ptr still supported but deprecated as functors are a much better way to do the same thing
        struct Stopped : public msm::front::state<msm::front::default_base_state,msm::front::sm_ptr> 
        {	 
            template <class Event,class FSM>
            void on_entry(Event const& ,FSM&) {std::cout << "entering: Stopped" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Stopped" << std::endl;}
            void set_sm_ptr(player_* pl)
            {
                m_player=pl;
            }
            player_* m_player;
        };

        struct Playing_ : public msm::front::state_machine_def<Playing_>
        {
            // when playing, the CD is loaded and we are in either pause or playing (duh)
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Playing" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Playing" << std::endl;}

            // The list of FSM states
            struct Song1 : public msm::front::state<>
            {
                int data_;
                Song1():data_(0){}
                Song1(int i):data_(i){}

                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "starting: First song" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "finishing: First Song" << std::endl;}

            };
            struct Song2 : public msm::front::state<>
            { 
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "starting: Second song" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "finishing: Second Song" << std::endl;}
            };
            struct Song3 : public msm::front::state<>
            { 
                template <class Event,class FSM>
                void on_entry(Event const&,FSM& ) {std::cout << "starting: Third song" << std::endl;}
                template <class Event,class FSM>
                void on_exit(Event const&,FSM& ) {std::cout << "finishing: Third Song" << std::endl;}
            };
            // the initial state. Must be defined
            typedef Song1 initial_state;
            // transition actions
            void start_next_song(NextSong const&)       { std::cout << "Playing::start_next_song\n"; }
            void start_prev_song(PreviousSong const&)       { std::cout << "Playing::start_prev_song\n"; }
            // guard conditions

            typedef Playing_ pl; // makes transition table cleaner
            // Transition table for Playing
            struct transition_table : mpl::vector4<
                //      Start     Event         Next      Action               Guard
                //    +---------+-------------+---------+---------------------+----------------------+
                a_row < Song1   , NextSong    , Song2   , &pl::start_next_song                       >,
                a_row < Song2   , PreviousSong, Song1   , &pl::start_prev_song                       >,
                a_row < Song2   , NextSong    , Song3   , &pl::start_next_song                       >,
                a_row < Song3   , PreviousSong, Song2   , &pl::start_prev_song                       >
                //    +---------+-------------+---------+---------------------+----------------------+
            > {};
            // Replaces the default no-transition response.
            template <class FSM,class Event>
            void no_transition(Event const& e, FSM&,int state)
            {
                std::cout << "no transition from state " << state
                    << " on event " << typeid(e).name() << std::endl;
            }
        };
        // back-end
        typedef msm::back::state_machine<Playing_> Playing;

        // state not defining any entry or exit
        struct Paused : public msm::front::state<>
        {
        };

        // the initial state of the player SM. Must be defined
        typedef Empty initial_state;

        // transition actions
        void start_playback(play const&)       { std::cout << "player::start_playback\n"; }
        void open_drawer(open_close const&)    { std::cout << "player::open_drawer\n"; }
        void close_drawer(open_close const&)   { std::cout << "player::close_drawer\n"; }
        void store_cd_info(cd_detected const&) { std::cout << "player::store_cd_info\n"; }
        void stop_playback(stop const&)        { std::cout << "player::stop_playback\n"; }
        void pause_playback(pause const&)      { std::cout << "player::pause_playback\n"; }
        void resume_playback(end_pause const&)      { std::cout << "player::resume_playback\n"; }
        void stop_and_open(open_close const&)  { std::cout << "player::stop_and_open\n"; }
        void stopped_again(stop const&)	{std::cout << "player::stopped_again\n";}
        // guard conditions
        bool good_disk_format(cd_detected const& evt)
        {
            // to test a guard condition, let's say we understand only CDs, not DVD
            if (evt.disc_type != DISK_CD)
            {
                std::cout << "wrong disk, sorry" << std::endl;
                return false;
            }
            return true;
        }
        // used to show a transition conflict. This guard will simply deactivate one transition and thus
        // solve the conflict
        bool auto_start(cd_detected const&)
        {
            return false;
        }

        typedef player_ p; // makes transition table cleaner

        // Transition table for player
        struct transition_table : mpl::vector<
            //    Start     Event         Next      Action				 Guard
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < Stopped , play        , Playing , &p::start_playback                         >,
          a_row < Stopped , open_close  , Open    , &p::open_drawer                            >,
           _row < Stopped , stop        , Stopped                                              >,
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < Open    , open_close  , Empty   , &p::close_drawer                           >,
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < Empty   , open_close  , Open    , &p::open_drawer                            >,
            row < Empty   , cd_detected , Stopped , &p::store_cd_info   ,&p::good_disk_format  >,
            row < Empty   , cd_detected , Playing , &p::store_cd_info   ,&p::auto_start        >,
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < Playing , stop        , Stopped , &p::stop_playback                          >,
          a_row < Playing , pause       , Paused  , &p::pause_playback                         >,
          a_row < Playing , open_close  , Open    , &p::stop_and_open                          >,
            //  +---------+-------------+---------+---------------------+----------------------+
          a_row < Paused  , end_pause   , Playing , &p::resume_playback                        >,
          a_row < Paused  , stop        , Stopped , &p::stop_playback                          >,
          a_row < Paused  , open_close  , Open    , &p::stop_and_open                          >
            //  +---------+-------------+---------+---------------------+----------------------+
        > {};
        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& e, FSM&,int state)
        {
            BOOST_FAIL("no_transition called!");
        }
    };
    // Pick a back-end
    typedef msm::back::state_machine<player_> player;


    BOOST_AUTO_TEST_CASE( my_test )
    {     
        SomeExternalContext ctx(3);
        player p1(boost::ref(ctx),5);
        BOOST_CHECK_MESSAGE(p1.context_.bla == 10,"Wrong returned context value");

        ctx.bla = 3;
        player p2(msm::back::states_ << player_::Empty(1),boost::ref(ctx),5);
        BOOST_CHECK_MESSAGE(p2.get_state<player_::Empty&>().data_ == 1,"Wrong Empty value");

        p2.set_states(msm::back::states_ << player_::Empty(5));
        BOOST_CHECK_MESSAGE(p2.get_state<player_::Empty&>().data_ == 5,"Wrong Empty value");

        p2.set_states(msm::back::states_ << player_::Empty(7) << player_::Open(2));
        BOOST_CHECK_MESSAGE(p2.get_state<player_::Empty&>().data_ == 7,"Wrong Empty value");
        BOOST_CHECK_MESSAGE(p2.get_state<player_::Open&>().data_ == 2,"Wrong Open value");

        ctx.bla = 3;
        player p(msm::back::states_ << player_::Empty(1) 
                                    << player_::Playing(msm::back::states_ << player_::Playing_::Song1(8)),
                 boost::ref(ctx),5);
        BOOST_CHECK_MESSAGE(p.get_state<player_::Playing&>().get_state<player_::Playing_::Song1&>().data_ == 8,"Wrong Open value");
    }
}


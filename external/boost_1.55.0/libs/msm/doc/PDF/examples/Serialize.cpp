#include <iostream>
// back-end
#include <boost/msm/back/state_machine.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>

// include headers that implement a archive in simple text format
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/tracking.hpp>

#include <fstream>


namespace msm = boost::msm;
namespace mpl = boost::mpl;


namespace
{
    // events
    struct play {};
    struct end_pause {};
    struct stop {};
    struct pause {};
    struct open_close {};

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
        //we might want to serialize some data contained by the front-end
        int front_end_data;
        player_():front_end_data(0){}
        // to achieve this, ask for it
        typedef int do_serialize;
        // and provide a serialize
        template<class Archive>
        void serialize(Archive & ar, const unsigned int )
        {
            ar & front_end_data;
        }
        // The list of FSM states
        struct Empty : public msm::front::state<> 
        {
            // we want Empty to be serialized
            typedef int do_serialize;
            template<class Archive>
            void serialize(Archive & ar, const unsigned int )
            {
                ar & some_dummy_data;
            }
            Empty():some_dummy_data(0){}
            // every (optional) entry/exit methods get the event passed.
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Empty" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Empty" << std::endl;}
            int some_dummy_data;
        };
        struct Open : public msm::front::state<> 
        {	 
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

        struct Playing : public msm::front::state<>
        {
            template <class Event,class FSM>
            void on_entry(Event const&,FSM& ) {std::cout << "entering: Playing" << std::endl;}
            template <class Event,class FSM>
            void on_exit(Event const&,FSM& ) {std::cout << "leaving: Playing" << std::endl;}
        };

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
            std::cout << "no transition from state " << state
                << " on event " << typeid(e).name() << std::endl;
        }
    };
    // Pick a back-end
    typedef msm::back::state_machine<player_> player;

    //
    // Testing utilities.
    //
    static char const* const state_names[] = { "Stopped", "Open", "Empty", "Playing", "Paused" };
    void pstate(player const& p)
    {
        std::cout << " -> " << state_names[p.current_state()[0]] << std::endl;
    }

    void test()
    {        
		player p;
        // needed to start the highest-level SM. This will call on_entry and mark the start of the SM
        p.start(); 
        p.get_state<player_::Empty&>().some_dummy_data=3;
        p.front_end_data=4;

        // go to Open, call on_exit on Empty, then action, then on_entry on Open
        p.process_event(open_close()); pstate(p);

        std::ofstream ofs("fsm.txt");
        // save fsm to archive (current state is Open)
        {
            boost::archive::text_oarchive oa(ofs);
            // write class instance to archive
            oa << p;
        }
        // reload fsm in state Open
        player p2;
        {
            // create and open an archive for input
            std::ifstream ifs("fsm.txt");
            boost::archive::text_iarchive ia(ifs);
            // read class state from archive
            ia >> p2;
        }
        // we now use p2 as it was loaded
        // check that we kept Empty's data value
        std::cout << "Empty's data should be 3:" << p2.get_state<player_::Empty&>().some_dummy_data << std::endl;
        std::cout << "front-end data should be 4:" << p2.front_end_data << std::endl;

        p2.process_event(open_close()); pstate(p2);
        // will be rejected, wrong disk type
        p2.process_event(
            cd_detected("louie, louie",DISK_DVD)); pstate(p2);
        p2.process_event(
            cd_detected("louie, louie",DISK_CD)); pstate(p2);
		p2.process_event(play());

        // at this point, Play is active      
        p2.process_event(pause()); pstate(p2);
        // go back to Playing
        p2.process_event(end_pause());  pstate(p2);
        p2.process_event(pause()); pstate(p2);
        p2.process_event(stop());  pstate(p2);
        // event leading to the same state
        // no action method called as it is not present in the transition table
        p2.process_event(stop());  pstate(p2);
    }
}
// eliminate object tracking (even if serialized through a pointer)
// at the risk of a programming error creating duplicate objects.
// this is to get rid of warning because p is not const
BOOST_CLASS_TRACKING(player, boost::serialization::track_never)

int main()
{
    test();
    return 0;
}

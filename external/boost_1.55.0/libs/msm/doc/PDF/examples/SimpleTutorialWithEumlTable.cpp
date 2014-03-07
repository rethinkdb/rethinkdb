#include <iostream>
// back-end
#include <boost/msm/back/state_machine.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/euml/euml.hpp>

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace std;

// entry/exit/action/guard logging functors
#include "logging_functors.h"

namespace
{
    // events
    struct play_impl : msm::front::euml::euml_event<play_impl> {};
    struct end_pause_impl : msm::front::euml::euml_event<end_pause_impl>{};
    struct stop_impl : msm::front::euml::euml_event<stop_impl>{};
    struct pause_impl : msm::front::euml::euml_event<pause_impl>{};
    struct open_close_impl : msm::front::euml::euml_event<open_close_impl>{};
    struct cd_detected_impl : msm::front::euml::euml_event<cd_detected_impl>{};

    // define some dummy instances for use in the transition table
    // it is also possible to default-construct them instead:
    // struct play {};
    // inside the table: play()
    play_impl play;
    end_pause_impl end_pause;
    stop_impl stop;
    pause_impl pause;
    open_close_impl open_close;
    cd_detected_impl cd_detected;

    // The list of FSM states
    // they have to be declared outside of the front-end only to make VC happy :(
    // note: gcc would have no problem
    struct Empty_impl : public msm::front::state<> , public msm::front::euml::euml_state<Empty_impl>
    {
        // every (optional) entry/exit methods get the event passed.
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {std::cout << "entering: Empty" << std::endl;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "leaving: Empty" << std::endl;}
    };
    struct Open_impl : public msm::front::state<> , public msm::front::euml::euml_state<Open_impl> 
    {	 
        template <class Event,class FSM>
        void on_entry(Event const& ,FSM&) {std::cout << "entering: Open" << std::endl;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "leaving: Open" << std::endl;}
    };

    struct Stopped_impl : public msm::front::state<> , public msm::front::euml::euml_state<Stopped_impl> 
    {	 
        template <class Event,class FSM>
        void on_entry(Event const& ,FSM&) {std::cout << "entering: Stopped" << std::endl;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "leaving: Stopped" << std::endl;}
    };

    struct Playing_impl : public msm::front::state<> , public msm::front::euml::euml_state<Playing_impl>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& ) {std::cout << "entering: Playing" << std::endl;}
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& ) {std::cout << "leaving: Playing" << std::endl;}
    };

    // state not defining any entry or exit
    struct Paused_impl : public msm::front::state<> , public msm::front::euml::euml_state<Paused_impl>
    {
    };
    //to make the transition table more readable
    Empty_impl const Empty;
    Open_impl const Open;
    Stopped_impl const Stopped;
    Playing_impl const Playing;
    Paused_impl const Paused;

    // front-end: define the FSM structure 
    struct player_ : public msm::front::state_machine_def<player_>
    {

        // the initial state of the player SM. Must be defined
        typedef Empty_impl initial_state;

        // Transition table for player
        // replaces the old transition table
        BOOST_MSM_EUML_DECLARE_TRANSITION_TABLE((
          Stopped + play        / start_playback                    == Playing               ,
          Stopped + open_close  / open_drawer                       == Open                  ,
          Stopped + stop                                            == Stopped               ,
          //  +------------------------------------------------------------------------------+
          Open    + open_close  / close_drawer                      == Empty                 ,
          //  +------------------------------------------------------------------------------+
          Empty   + open_close  / open_drawer                       == Open                  ,
          Empty   + cd_detected /(store_cd_info,
                                  msm::front::euml::process_(play)) == Stopped               ,
          //  +------------------------------------------------------------------------------+
          Playing + stop        / stop_playback                     == Stopped               ,
          Playing + pause       / pause_playback                    == Paused                ,
          Playing + open_close  / stop_and_open                     == Open                  ,
          //  +------------------------------------------------------------------------------+
          Paused  + end_pause   / resume_playback                   == Playing               ,
          Paused  + stop        / stop_playback                     == Stopped               ,
          Paused  + open_close  / stop_and_open                     == Open     
          //  +------------------------------------------------------------------------------+
          ),transition_table)

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
        // go to Open, call on_exit on Empty, then action, then on_entry on Open
        p.process_event(open_close); pstate(p);
        p.process_event(open_close); pstate(p);
        p.process_event(cd_detected); pstate(p);

        // at this point, Play is active      
        p.process_event(pause); pstate(p);
        // go back to Playing
        p.process_event(end_pause);  pstate(p);
        p.process_event(pause); pstate(p);
        p.process_event(stop);  pstate(p);
        // event leading to the same state
        // no action method called as it is not present in the transition table
        p.process_event(stop);  pstate(p);
    }
}

int main()
{
    test();
    return 0;
}

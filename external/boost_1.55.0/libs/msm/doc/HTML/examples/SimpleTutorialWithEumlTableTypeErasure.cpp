#include <iostream>
// back-end
#include <boost/msm/back/state_machine.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/euml/euml.hpp>
#include <boost/msm/event_traits.hpp>

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/member.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/constructible.hpp>
#include <boost/type_erasure/relaxed_match.hpp>
#include <boost/type_erasure/any_cast.hpp>

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace std;
using namespace msm::front::euml;

// entry/exit/action/guard logging functors
#include "logging_functors.h"

BOOST_TYPE_ERASURE_MEMBER((has_getNumber), getNumber, 0);
//type erasure event
typedef ::boost::mpl::vector<
    has_getNumber<int(), const boost::type_erasure::_self>,
    boost::type_erasure::relaxed_match,
    boost::type_erasure::copy_constructible<>,
    boost::type_erasure::typeid_<>
> any_number_event_concept;
struct any_number_event :   boost::type_erasure::any<any_number_event_concept>,
                            msm::front::euml::euml_event<any_number_event>
{
    template <class U>
    any_number_event(U const& u): boost::type_erasure::any<any_number_event_concept> (u){}
    any_number_event(): boost::type_erasure::any<any_number_event_concept> (){}
};


namespace boost { namespace msm{
    template<> 
    struct is_kleene_event< any_number_event >
    { 
        typedef boost::mpl::true_ type;
    };
}}

namespace
{
    // events
    struct play_impl : msm::front::euml::euml_event<play_impl> {int getNumber()const {return 0;}};
    struct end_pause_impl : msm::front::euml::euml_event<end_pause_impl>{int getNumber()const {return 1;}};
    struct stop_impl : msm::front::euml::euml_event<stop_impl>{int getNumber()const {return 2;}};
    struct pause_impl : msm::front::euml::euml_event<pause_impl>{int getNumber()const {return 3;}};
    struct open_close_impl : msm::front::euml::euml_event<open_close_impl>{int getNumber()const {return 4;}};
    struct cd_detected_impl : msm::front::euml::euml_event<cd_detected_impl>{int getNumber()const {return 5;}};

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
    any_number_event number_event;

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

    BOOST_MSM_EUML_ACTION(pause_playback2)
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& evt,FSM&,SourceState& ,TargetState& )
        {
            cout << "player::pause_playback2" << endl;
            std::cout << "event type: " << typeid(EVT).name() << std::endl;
            std::cout << "event's number: " << evt.getNumber() << std::endl;
        }
    };
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
          Playing + number_event / pause_playback                   == Paused                ,
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

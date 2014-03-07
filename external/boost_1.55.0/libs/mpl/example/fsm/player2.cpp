//
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#include "boost/mpl/int.hpp"
#include "boost/mpl/fold.hpp"
#include "boost/mpl/prior.hpp"
#include "boost/mpl/count.hpp"
#include "boost/mpl/insert.hpp"
#include <boost/mpl/greater.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/filter_view.hpp>
#include "boost/mpl/vector/vector20.hpp"
#include "boost/assert.hpp"
#include <boost/type_traits/is_same.hpp>

#include <vector>
#include <ctime>
#include <iostream>

#if defined(BOOST_DINKUMWARE_STDLIB) && BOOST_DINKUMWARE_STDLIB < 310
namespace std { using ::clock_t; }
#endif

namespace mpl = boost::mpl;
using namespace mpl::placeholders;

// A metafunction that returns the Event associated with a transition.
template <class Transition>
struct transition_event
{
    typedef typename Transition::event type;
};

// A metafunction computing the maximum of a transition's source and
// end states.
template <class Transition>
struct transition_max_state
{
    typedef typename mpl::int_<
        (Transition::current_state > Transition::next_state)
            ? Transition::current_state
            : Transition::next_state
    > type;
};

template<class Derived>
class state_machine;

// Generates a singleton runtime lookup table that maps current state
// to a function that makes the FSM take its transition on the given
// Event type.
template <class Fsm, int initial_state, class Stt, class Event>
struct dispatch_table
{
 private:
    // This is a table of these function pointers.
    typedef int (*cell)(Fsm&, int, Event const&);

    // Compute the maximum state value in the Fsm so we know how big
    // to make the table
    BOOST_STATIC_CONSTANT(
        int, max_state = (
            mpl::fold<Stt
              , mpl::int_<initial_state>
              , mpl::if_<
                    mpl::greater<transition_max_state<_2>,_1>
                  , transition_max_state<_2>
                  , _1
                >
            >::type::value
        )
    );

    // A function object for use with mpl::for_each that stuffs
    // transitions into cells.
    struct init_cell
    {
        init_cell(dispatch_table* self_)
          : self(self_)
        {}
        
        // Cell initializer function object, used with mpl::for_each
        template <class Transition>
        void operator()(Transition const&) const
        {
            self->entries[Transition::current_state] = &Transition::execute;
        }
    
        dispatch_table* self;
    };
    
 public:
    // initialize the dispatch table for a given Event and Fsm
    dispatch_table()
    {
        // Initialize cells for no transition
        for (int i = 0; i <= max_state; ++i)
        {
            // VC7.1 seems to need the two-phase assignment.
            cell call_no_transition = &state_machine<Fsm>::call_no_transition;
            entries[i] = call_no_transition;
        }

        // Go back and fill in cells for matching transitions.
        mpl::for_each<
            mpl::filter_view<
                Stt
              , boost::is_same<transition_event<_>, Event>
            >
        >(init_cell(this));
    }

    // The singleton instance.
    static const dispatch_table instance;

 public: // data members
    cell entries[max_state + 1];
};

// This declares the statically-initialized dispatch_table instance.
template <class Fsm, int initial_state, class Stt, class Event>
const dispatch_table<Fsm, initial_state, Stt, Event>
dispatch_table<Fsm, initial_state, Stt, Event>::instance;

// CRTP base class for state machines.  Pass the actual FSM class as
// the Derived parameter.
template<class Derived>
class state_machine
{
 public: // Member functions
    
    // Main function used by clients of the derived FSM to make
    // transitions.
    template<class Event>
    int process_event(Event const& evt)
    {
        typedef typename Derived::transition_table stt;
        typedef dispatch_table<Derived, Derived::initial_state,stt,Event> table;
        
        // Call the action
        return this->m_state
            = table::instance.entries[this->m_state](
                *static_cast<Derived*>(this), this->m_state, evt);
    }

    // Getter that returns the current state of the FSM
    int current_state() const
    {
        return this->m_state;
    }

 private:
    template <class Fsm, int initial_state, class Stt, class Event>
    friend class dispatch_table;
    
    template <class Event>
    static int call_no_transition(Derived& fsm, int state, Event const& e)
    {
        return fsm.no_transition(state, e);
    }

    // Default no-transition handler.  Can be replaced in the Derived
    // FSM class.
    template <class Event>
    int no_transition(int state, Event const& e)
    {
        BOOST_ASSERT(false);
        return state;
    }
    
 protected:    // interface for the derived class
    
    template<class State>
    state_machine(State state)                  // Construct with an initial state
        : m_state(state)
    {
    }

    state_machine()
      : m_state(Derived::initial_state)         // Construct with the default initial_state
    {
    }

    // Template used to form rows in the transition table
    template<
        int CurrentState
      , class Event
      , int NextState
      , void (Derived::*action)(Event const&)
    >
    struct row
    {
        BOOST_STATIC_CONSTANT(int, current_state = CurrentState);
        BOOST_STATIC_CONSTANT(int, next_state = NextState);
        typedef Event event;

        // Take the transition action and return the next state.
        static int execute(Derived& fsm, int state, Event const& evt)
        {
            BOOST_ASSERT(state == current_state);
            (fsm.*action)(evt);
            return next_state;
        }
    };

 private: // data members
    int m_state;
};

namespace  // Concrete FSM implementation
{
  // events
  struct play {};
  struct stop {};
  struct pause {};
  struct open_close {};

  // A "complicated" event type that carries some data.
  struct cd_detected
  {
      cd_detected(std::string name, std::vector<std::clock_t> durations)
        : name(name)
        , track_durations(durations)
      {}
        
      std::string name;
      std::vector<std::clock_t> track_durations;
  };

  // Concrete FSM implementation 
  class player : public state_machine<player>
  {
      // The list of FSM states
      enum states {
          Empty, Open, Stopped, Playing, Paused
        , initial_state = Empty
      };

#ifdef __MWERKS__
   public: // Codewarrior bug workaround.  Tested at 0x3202
#endif 
      // transition actions
      void start_playback(play const&)       { std::cout << "player::start_playback\n"; }
      void open_drawer(open_close const&)    { std::cout << "player::open_drawer\n"; }
      void close_drawer(open_close const&)   { std::cout << "player::close_drawer\n"; }
      void store_cd_info(cd_detected const&) { std::cout << "player::store_cd_info\n"; }
      void stop_playback(stop const&)        { std::cout << "player::stop_playback\n"; }
      void pause_playback(pause const&)      { std::cout << "player::pause_playback\n"; }
      void resume_playback(play const&)      { std::cout << "player::resume_playback\n"; }
      void stop_and_open(open_close const&)  { std::cout << "player::stop_and_open\n"; }
#ifdef __MWERKS__
   private:
#endif 
      friend class state_machine<player>;
      typedef player p; // makes transition table cleaner

      // Transition table
      struct transition_table : mpl::vector11<
          //    Start     Event         Next      Action
          //  +---------+-------------+---------+---------------------+
          row < Stopped , play        , Playing , &p::start_playback  >,
          row < Stopped , open_close  , Open    , &p::open_drawer     >,
          //  +---------+-------------+---------+---------------------+
          row < Open    , open_close  , Empty   , &p::close_drawer    >,
          //  +---------+-------------+---------+---------------------+
          row < Empty   , open_close  , Open    , &p::open_drawer     >,
          row < Empty   , cd_detected , Stopped , &p::store_cd_info   >,
          //  +---------+-------------+---------+---------------------+
          row < Playing , stop        , Stopped , &p::stop_playback   >,
          row < Playing , pause       , Paused  , &p::pause_playback  >,
          row < Playing , open_close  , Open    , &p::stop_and_open   >,
          //  +---------+-------------+---------+---------------------+
          row < Paused  , play        , Playing , &p::resume_playback >,
          row < Paused  , stop        , Stopped , &p::stop_playback   >,
          row < Paused  , open_close  , Open    , &p::stop_and_open   >
          //  +---------+-------------+---------+---------------------+
      > {};

      // Replaces the default no-transition response.
      template <class Event>
      int no_transition(int state, Event const& e)
      {
          std::cout << "no transition from state " << state
                    << " on event " << typeid(e).name() << std::endl;
          return state;
      }
  };

  //
  // Testing utilities.
  //
  static char const* const state_names[] = { "Empty", "Open", "Stopped", "Playing", "Paused" };

  void pstate(player const& p)
  {
      std::cout << " -> " << state_names[p.current_state()] << std::endl;
  }
  
  void test()
  {
      player p;
      p.process_event(open_close()); pstate(p);
      p.process_event(open_close()); pstate(p);
      p.process_event(
          cd_detected(
              "louie, louie"
            , std::vector<std::clock_t>( /* track lengths */ )
          )
      );
      pstate(p);
      
      p.process_event(play());  pstate(p);
      p.process_event(pause()); pstate(p);
      p.process_event(play());  pstate(p);
      p.process_event(stop());  pstate(p);
  }
}

int main()
{
    test();
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// The following code implements the state-machine (this version details an
// alternative way of retrieving the elapsed time from the main program):
//
//  --------------------------------
// |                                |
// |           O     Active         |
// |           |                    |<----
// |           v                    |     | EvReset
// |  ----------------------------  |     |
// | |                            | |-----
// | |         Stopped            | |
// |  ----------------------------  |
// |  |              ^              |
// |  | EvStartStop  | EvStartStop  |<-----O
// |  v              |              |
// |  ----------------------------  |
// | |                            | |
// | |         Running            | |
// |  ----------------------------  |
//  --------------------------------



#include <boost/statechart/event.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include <boost/mpl/list.hpp>

#include <boost/config.hpp>

#include <ctime>
#include <iostream>

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std
{
  using ::time;
  using ::difftime;
  using ::time_t;
}
#endif

#ifdef BOOST_INTEL
#  pragma warning( disable: 304 ) // access control not specified
#  pragma warning( disable: 444 ) // destructor for base is not virtual
#  pragma warning( disable: 981 ) // operands are evaluated in unspecified order
#endif



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



struct EvStartStop : sc::event< EvStartStop > {};
struct EvReset : sc::event< EvReset > {};
struct EvGetElapsedTime : sc::event< EvGetElapsedTime >
{
  public:
    EvGetElapsedTime( double & time ) : time_( time ) {}

    void Assign( double time ) const
    {
      time_ = time;
    }

  private:
    double & time_;
};


struct Active;
struct StopWatch : sc::state_machine< StopWatch, Active > {};


struct Stopped;
struct Active : sc::simple_state< Active, StopWatch, Stopped >
{
  public:
    typedef sc::transition< EvReset, Active > reactions;

    Active() : elapsedTime_( 0.0 ) {}

    double & ElapsedTime()
    {
      return elapsedTime_;
    }

    double ElapsedTime() const
    {
      return elapsedTime_;
    }

  private:
    double elapsedTime_;
};

struct Running : sc::simple_state< Running, Active >
{
  public:
    typedef mpl::list<
      sc::custom_reaction< EvGetElapsedTime >,
      sc::transition< EvStartStop, Stopped >
    > reactions;

    Running() : startTime_( std::time( 0 ) ) {}

    ~Running()
    {
      context< Active >().ElapsedTime() = ElapsedTime();
    }

    sc::result react( const EvGetElapsedTime & evt )
    {
      evt.Assign( ElapsedTime() );
      return discard_event();
    }

  private:
    double ElapsedTime() const
    {
      return context< Active >().ElapsedTime() +
        std::difftime( std::time( 0 ), startTime_ );
    }

    std::time_t startTime_;
};

struct Stopped : sc::simple_state< Stopped, Active >
{
  typedef mpl::list<
    sc::custom_reaction< EvGetElapsedTime >,
    sc::transition< EvStartStop, Running >
  > reactions;

  sc::result react( const EvGetElapsedTime & evt )
  {
    evt.Assign( context< Active >().ElapsedTime() );
    return discard_event();
  }
};


namespace
{
  char GetKey()
  {
    char key;
    std::cin >> key;
    return key;
  }
}

int main()
{
  std::cout << "Boost.Statechart StopWatch example\n\n";
  std::cout << "s<CR>: Starts/Stops stop watch\n";
  std::cout << "r<CR>: Resets stop watch\n";
  std::cout << "d<CR>: Displays the elapsed time in seconds\n";
  std::cout << "e<CR>: Exits the program\n\n";
  std::cout << "You may chain commands, e.g. rs<CR> resets and starts stop watch\n\n";
  
  StopWatch stopWatch;
  stopWatch.initiate();

  char key = GetKey();

  while ( key != 'e' )
  {
    switch( key )
    {
      case 'r':
      {
        stopWatch.process_event( EvReset() );
      }
      break;

      case 's':
      {
        stopWatch.process_event( EvStartStop() );
      }
      break;

      case 'd':
      {
        double elapsedTime = 0.0;
        stopWatch.process_event( EvGetElapsedTime( elapsedTime ) );
        std::cout << "Elapsed time: " << elapsedTime << "\n";
      }
      break;

      default:
      {
        std::cout << "Invalid key!\n";
      }
      break;
    }

    key = GetKey();
  }

  return 0;
}

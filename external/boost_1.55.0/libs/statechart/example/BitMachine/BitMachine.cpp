//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
#define NO_OF_BITS 3
//////////////////////////////////////////////////////////////////////////////
// This program demonstrates the fact that measures must be taken to hide some
// of the complexity (e.g. in separate .cpp file) of a Boost.Statechart state
// machine once a certain size is reached.
// For this purpose, a state machine with exactly 2^NO_OF_BITS states (i.e.
// BitState< 0 > .. BitState< 2^NO_OF_BITS - 1 >) is generated. For the events
// EvFlipBit< 0 > .. EvFlipBit< NO_OF_BITS - 1 > there is a transition from
// each state to the state with the corresponding bit toggled. That is, there
// is a total of 2^NO_OF_BITS * NO_OF_BITS transitions.
// E.g. if the state machine is currently in state BitState< 5 > and receives
// EvFlipBit< 2 >, it transitions to state BitState< 1 >. If it is in
// BitState< 15 > and receives EvFlipBit< 4 > it transitions to BitState< 31 >
// etc.
// The maximum size of such a state machine depends on your compiler. The
// following table gives upper limits for NO_OF_BITS. From this, rough
// estimates for the maximum size of any "naively" implemented Boost.Statechart
// machine (i.e. no attempt is made to hide inner state implementation in a
// .cpp file) can be deduced.
//
// NOTE: Due to the fact that the amount of generated code more than
// *doubles* each time NO_OF_BITS is *incremented*, build times on most
// compilers soar when NO_OF_BITS > 6.
//
// Compiler      | max. NO_OF_BITS b | max. states s  |
// --------------|-------------------|----------------|
// MSVC 7.1      |      b < 6        |  32 < s <  64  |
// GCC 3.4.2 (1) |      b < 8        | 128 < s < 256  |
//
// (1) This is a practical rather than a hard limit, caused by a compiler
//     memory footprint that was significantly larger than the 1GB physical
//     memory installed in the test machine. The resulting frequent swapping
//     led to compilation times of hours rather than minutes.
//////////////////////////////////////////////////////////////////////////////



#include "UniqueObject.hpp"

#include <boost/statechart/event.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/transition.hpp>

#include <boost/mpl/list.hpp>
#include <boost/mpl/front_inserter.hpp>
#include <boost/mpl/transform_view.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/integral_c.hpp>
#include <boost/mpl/shift_left.hpp>
#include <boost/mpl/bitxor.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/aux_/lambda_support.hpp>

#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>

#include <iostream>
#include <iomanip>
#include <cstddef> // size_t

#ifdef BOOST_INTEL
#  pragma warning( disable: 304 ) // access control not specified
#  pragma warning( disable: 444 ) // destructor for base is not virtual
#  pragma warning( disable: 981 ) // operands are evaluated in unspecified order
#endif



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



//////////////////////////////////////////////////////////////////////////////
struct IDisplay
{
  virtual void Display() const = 0;
};

//////////////////////////////////////////////////////////////////////////////
template< class BitNo >
struct EvFlipBit : sc::event< EvFlipBit< BitNo > > {};

template< class StateNo >
struct BitState;

struct BitMachine : sc::state_machine<
  BitMachine, BitState< mpl::integral_c< unsigned int, 0 > > > {};

template< class BitNo, class StateNo >
struct FlipTransition
{
  private:
    typedef typename mpl::bitxor_< 
      StateNo, 
      mpl::shift_left< mpl::integral_c< unsigned int, 1 >, BitNo >
    >::type NextStateNo;

  public:
    typedef typename sc::transition<
      EvFlipBit< BitNo >, BitState< NextStateNo > > type;

    BOOST_MPL_AUX_LAMBDA_SUPPORT( 2, FlipTransition, (BitNo, StateNo) );
};

//////////////////////////////////////////////////////////////////////////////
void DisplayBits( unsigned int number )
{
  char buffer[ NO_OF_BITS + 1 ];
  buffer[ NO_OF_BITS ] = 0;

  for ( unsigned int bit = 0; bit < NO_OF_BITS; ++bit )
  {
    buffer[ bit ] = number & ( 1 << ( NO_OF_BITS - bit - 1 ) ) ? '1' : '0';
  }

  std::cout << "Current state: " << std::setw( 4 ) <<
    number << " (" << buffer << ")" << std::endl;
}

template< class StateNo >
struct BitState : sc::simple_state< BitState< StateNo >, BitMachine >,
  UniqueObject< BitState< StateNo > >, IDisplay
{
  void * operator new( std::size_t size )
  {
    return UniqueObject< BitState< StateNo > >::operator new( size );
  }

  void operator delete( void * p, std::size_t size )
  {
    UniqueObject< BitState< StateNo > >::operator delete( p, size );
  }

  typedef typename mpl::copy<
    typename mpl::transform_view<
      mpl::range_c< unsigned int, 0, NO_OF_BITS >,
      FlipTransition< mpl::placeholders::_, StateNo > >::type,
    mpl::front_inserter< mpl::list<> >
  >::type reactions;

  virtual void Display() const
  {
    DisplayBits( StateNo::value );
  }
};


void DisplayMachineState( const BitMachine & bitMachine )
{
  bitMachine.state_cast< const IDisplay & >().Display();
}

//////////////////////////////////////////////////////////////////////////////
boost::intrusive_ptr< const sc::event_base > pFlipBitEvents[ NO_OF_BITS ];

struct EventInserter
{
  template< class BitNo >
  void operator()( const BitNo & )
  {
    pFlipBitEvents[ BitNo::value ] = new EvFlipBit< BitNo >();
  }
};

void FillEventArray()
{
  mpl::for_each< mpl::range_c< unsigned int, 0, NO_OF_BITS > >(
    EventInserter() );
}

//////////////////////////////////////////////////////////////////////////////
void VisitAllStates( BitMachine & bitMachine, unsigned int msb )
{
  if ( msb > 0 )
  {
    VisitAllStates( bitMachine, msb - 1 );
  }

  bitMachine.process_event( *pFlipBitEvents[ msb ] );
  DisplayMachineState( bitMachine );

  if ( msb > 0 )
  {
    VisitAllStates( bitMachine, msb - 1 );
  }
}

//////////////////////////////////////////////////////////////////////////////
char GetKey()
{
  char key;
  std::cin >> key;
  return key;
}


//////////////////////////////////////////////////////////////////////////////
int main()
{
  FillEventArray();

  const unsigned int noOfStates = 1 << NO_OF_BITS;
  std::cout << "Boost.Statechart BitMachine example\n";
  std::cout << "Machine configuration: " << noOfStates <<
    " states interconnected with " << noOfStates * NO_OF_BITS <<
    " transitions.\n\n";

  for ( unsigned int bit = 0; bit < NO_OF_BITS; ++bit )
  {
    std::cout << bit - 0 << "<CR>: Flips bit " << bit - 0 << "\n";
  }

  std::cout << "a<CR>: Goes through all states automatically\n";
  std::cout << "e<CR>: Exits the program\n\n";
  std::cout << "You may chain commands, e.g. 31<CR> flips bits 3 and 1\n\n";


  BitMachine bitMachine;
  bitMachine.initiate();

  char key = GetKey();

  while ( key != 'e' )
  {
    if ( ( key >= '0' ) && ( key < static_cast< char >( '0' + NO_OF_BITS ) ) )
    {
      bitMachine.process_event( *pFlipBitEvents[ key - '0' ] );
      DisplayMachineState( bitMachine );
    }
    else
    {
      switch( key )
      {
        case 'a':
        {
          VisitAllStates( bitMachine, NO_OF_BITS - 1 );
        }
        break;

        default:
        {
          std::cout << "Invalid key!\n";
        }
      }
    }

    key = GetKey();
  }

  return 0;
}

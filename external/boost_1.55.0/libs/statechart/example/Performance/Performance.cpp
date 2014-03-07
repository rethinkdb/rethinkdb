//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2008 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// #define CUSTOMIZE_MEMORY_MANAGEMENT
// #define BOOST_STATECHART_USE_NATIVE_RTTI
//////////////////////////////////////////////////////////////////////////////
// This program measures event processing performance of the BitMachine
// (see BitMachine example for more information) with a varying number of
// states. Also, a varying number of transitions are replaced with in-state
// reactions. This allows us to calculate how much time is spent for state-
// entry and state-exit during a transition. All measurements are written to
// comma-separated-values files, one file for each individual BitMachine.
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/event.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/in_state_reaction.hpp>

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
#include <boost/mpl/if.hpp>
#include <boost/mpl/less.hpp>
#include <boost/mpl/aux_/lambda_support.hpp>

#include <boost/intrusive_ptr.hpp>
#include <boost/config.hpp>
#include <boost/assert.hpp>

#ifdef CUSTOMIZE_MEMORY_MANAGEMENT
#  ifdef BOOST_MSVC
#    pragma warning( push )
#    pragma warning( disable: 4127 ) // conditional expression is constant
#    pragma warning( disable: 4800 ) // forcing value to bool 'true' or 'false'
#  endif
#  define BOOST_NO_MT
#  include <boost/pool/pool_alloc.hpp>
#  ifdef BOOST_MSVC
#    pragma warning( pop )
#  endif
#endif

#include <vector>
#include <ctime>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ios>
#include <string>
#include <algorithm>

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std
{
  using ::clock_t;
  using ::clock;
}
#endif

#ifdef BOOST_INTEL
#  pragma warning( disable: 304 ) // access control not specified
#  pragma warning( disable: 444 ) // destructor for base is not virtual
#  pragma warning( disable: 981 ) // operands are evaluated in unspecified order
#endif



namespace sc = boost::statechart;
namespace mpl = boost::mpl;



//////////////////////////////////////////////////////////////////////////////
typedef mpl::integral_c< unsigned int, 0 > uint0;
typedef mpl::integral_c< unsigned int, 1 > uint1;
typedef mpl::integral_c< unsigned int, 2 > uint2;
typedef mpl::integral_c< unsigned int, 3 > uint3;
typedef mpl::integral_c< unsigned int, 4 > uint4;
typedef mpl::integral_c< unsigned int, 5 > uint5;
typedef mpl::integral_c< unsigned int, 6 > uint6;
typedef mpl::integral_c< unsigned int, 7 > uint7;
typedef mpl::integral_c< unsigned int, 8 > uint8;
typedef mpl::integral_c< unsigned int, 9 > uint9;

//////////////////////////////////////////////////////////////////////////////
template< class BitNo >
struct EvFlipBit : sc::event< EvFlipBit< BitNo > > {};

boost::intrusive_ptr< const sc::event_base > pFlipBitEvents[] =
{
  new EvFlipBit< uint0 >,
  new EvFlipBit< uint1 >,
  new EvFlipBit< uint2 >,
  new EvFlipBit< uint3 >,
  new EvFlipBit< uint4 >,
  new EvFlipBit< uint5 >,
  new EvFlipBit< uint6 >,
  new EvFlipBit< uint7 >,
  new EvFlipBit< uint8 >,
  new EvFlipBit< uint9 >
};


//////////////////////////////////////////////////////////////////////////////
template<
  class StateNo,
  class NoOfBits,
  class FirstTransitionBit >
struct BitState;

template< class NoOfBits, class FirstTransitionBit >
struct BitMachine : sc::state_machine<
  BitMachine< NoOfBits, FirstTransitionBit >,
  BitState< uint0, NoOfBits, FirstTransitionBit >
  #ifdef CUSTOMIZE_MEMORY_MANAGEMENT
  , boost::fast_pool_allocator< int >
  #endif
>
{
  public:
    BitMachine() : inStateReactions_( 0 ), transitions_( 0 ) {}

    // GCC 3.4.2 doesn't seem to instantiate a function template despite the
    // fact that an address of the instantiation is passed as a non-type
    // template argument. This leads to linker errors when a function template
    // is defined instead of the overloads below.
    void InStateReaction( const EvFlipBit< uint0 > & )
    {
      ++inStateReactions_;
    }

    void InStateReaction( const EvFlipBit< uint1 > & )
    {
      ++inStateReactions_;
    }

    void InStateReaction( const EvFlipBit< uint2 > & )
    {
      ++inStateReactions_;
    }

    void InStateReaction( const EvFlipBit< uint3 > & )
    {
      ++inStateReactions_;
    }

    void InStateReaction( const EvFlipBit< uint4 > & )
    {
      ++inStateReactions_;
    }

    void InStateReaction( const EvFlipBit< uint5 > & )
    {
      ++inStateReactions_;
    }

    void InStateReaction( const EvFlipBit< uint6 > & )
    {
      ++inStateReactions_;
    }

    void InStateReaction( const EvFlipBit< uint7 > & )
    {
      ++inStateReactions_;
    }

    void InStateReaction( const EvFlipBit< uint8 > & )
    {
      ++inStateReactions_;
    }

    void InStateReaction( const EvFlipBit< uint9 > & )
    {
      ++inStateReactions_;
    }

    void Transition( const EvFlipBit< uint0 > & )
    {
      ++transitions_;
    }

    void Transition( const EvFlipBit< uint1 > & )
    {
      ++transitions_;
    }

    void Transition( const EvFlipBit< uint2 > & )
    {
      ++transitions_;
    }

    void Transition( const EvFlipBit< uint3 > & )
    {
      ++transitions_;
    }

    void Transition( const EvFlipBit< uint4 > & )
    {
      ++transitions_;
    }

    void Transition( const EvFlipBit< uint5 > & )
    {
      ++transitions_;
    }

    void Transition( const EvFlipBit< uint6 > & )
    {
      ++transitions_;
    }

    void Transition( const EvFlipBit< uint7 > & )
    {
      ++transitions_;
    }

    void Transition( const EvFlipBit< uint8 > & )
    {
      ++transitions_;
    }

    void Transition( const EvFlipBit< uint9 > & )
    {
      ++transitions_;
    }

    unsigned int GetNoOfInStateReactions() const
    {
      return inStateReactions_;
    }

    unsigned int GetNoOfTransitions() const
    {
      return transitions_;
    }

  private:
    unsigned int inStateReactions_;
    unsigned int transitions_;
};

//////////////////////////////////////////////////////////////////////////////
template<
  class BitNo, class StateNo, class NoOfBits, class FirstTransitionBit >
struct FlipTransition
{
  private:
    typedef typename mpl::bitxor_<
      StateNo,
      mpl::shift_left< uint1, BitNo >
    >::type NextStateNo;

  public:
    typedef typename mpl::if_<
      mpl::less< BitNo, FirstTransitionBit >,
      sc::in_state_reaction< 
        EvFlipBit< BitNo >,
        BitMachine< NoOfBits, FirstTransitionBit >,
        &BitMachine< NoOfBits, FirstTransitionBit >::InStateReaction >,
      sc::transition<
        EvFlipBit< BitNo >,
        BitState< NextStateNo, NoOfBits, FirstTransitionBit >,
        BitMachine< NoOfBits, FirstTransitionBit >,
        &BitMachine< NoOfBits, FirstTransitionBit >::Transition >
    >::type type;

    BOOST_MPL_AUX_LAMBDA_SUPPORT(
      3, FlipTransition, (BitNo, StateNo, FirstTransitionBit) );
};

//////////////////////////////////////////////////////////////////////////////
template<
  class StateNo,
  class NoOfBits,
  class FirstTransitionBit >
struct BitState : sc::simple_state<
  BitState< StateNo, NoOfBits, FirstTransitionBit >,
  BitMachine< NoOfBits, FirstTransitionBit > >
{
  typedef typename mpl::copy< 
    typename mpl::transform_view<
      mpl::range_c< unsigned int, 0, NoOfBits::value >,
      FlipTransition<
        mpl::placeholders::_, StateNo, NoOfBits, FirstTransitionBit >
    >::type,
    mpl::front_inserter< mpl::list<> >
  >::type reactions;
};

// GCC 3.4.2 doesn't seem to instantiate a class template member function
// despite the fact that an address of the function is passed as a non-type
// template argument. This leads to linker errors when the class template
// defining the functions is not explicitly instantiated.
template struct BitMachine< uint1, uint0 >;
template struct BitMachine< uint1, uint1 >;

template struct BitMachine< uint2, uint0 >;
template struct BitMachine< uint2, uint1 >;
template struct BitMachine< uint2, uint2 >;

template struct BitMachine< uint3, uint0 >;
template struct BitMachine< uint3, uint1 >;
template struct BitMachine< uint3, uint2 >;
template struct BitMachine< uint3, uint3 >;

template struct BitMachine< uint4, uint0 >;
template struct BitMachine< uint4, uint1 >;
template struct BitMachine< uint4, uint2 >;
template struct BitMachine< uint4, uint3 >;
template struct BitMachine< uint4, uint4 >;

template struct BitMachine< uint5, uint0 >;
template struct BitMachine< uint5, uint1 >;
template struct BitMachine< uint5, uint2 >;
template struct BitMachine< uint5, uint3 >;
template struct BitMachine< uint5, uint4 >;
template struct BitMachine< uint5, uint5 >;

template struct BitMachine< uint6, uint0 >;
template struct BitMachine< uint6, uint1 >;
template struct BitMachine< uint6, uint2 >;
template struct BitMachine< uint6, uint3 >;
template struct BitMachine< uint6, uint4 >;
template struct BitMachine< uint6, uint5 >;
template struct BitMachine< uint6, uint6 >;

template struct BitMachine< uint7, uint0 >;
template struct BitMachine< uint7, uint1 >;
template struct BitMachine< uint7, uint2 >;
template struct BitMachine< uint7, uint3 >;
template struct BitMachine< uint7, uint4 >;
template struct BitMachine< uint7, uint5 >;
template struct BitMachine< uint7, uint6 >;
template struct BitMachine< uint7, uint7 >;


////////////////////////////////////////////////////////////////////////////
struct PerfResult
{
  PerfResult( double inStateRatio, double nanoSecondsPerReaction ) :
    inStateRatio_( inStateRatio ),
    nanoSecondsPerReaction_( nanoSecondsPerReaction )
  {
  }

  double inStateRatio_;
  double nanoSecondsPerReaction_;
};

template< class NoOfBits, class FirstTransitionBit >
class PerformanceTester
{
  public:
    ////////////////////////////////////////////////////////////////////////
    static PerfResult Test()
    {
      eventsSent_ = 0;
      BitMachine< NoOfBits, FirstTransitionBit > machine;
      machine.initiate();
      const std::clock_t startTime = std::clock();

      const unsigned int laps = eventsToSend_ / ( GetNoOfStates() - 1 );

      for ( unsigned int lap = 0; lap < laps; ++lap )
      {
        VisitAllStatesImpl( machine, NoOfBits::value - 1 );
      }

      const std::clock_t elapsedTime = std::clock() - startTime;

      BOOST_ASSERT( eventsSent_ == eventsToSend_ );
      BOOST_ASSERT( 
        machine.GetNoOfInStateReactions() +
        machine.GetNoOfTransitions() == eventsSent_ );

      return PerfResult(
        static_cast< double >( machine.GetNoOfInStateReactions() ) /
          eventsSent_,
        static_cast< double >( elapsedTime ) /
          CLOCKS_PER_SEC * 1000.0 * 1000.0 * 1000.0 / eventsSent_ );
    }

    static unsigned int GetNoOfStates()
    {
      return 1 << NoOfBits::value;
    }

    static unsigned int GetNoOfReactions()
    {
      return GetNoOfStates() * NoOfBits::value;
    }

  private:
    ////////////////////////////////////////////////////////////////////////
    static void VisitAllStatesImpl(
      BitMachine< NoOfBits, FirstTransitionBit > & machine,
      unsigned int bit )
    {
      if ( bit > 0 )
      {
        PerformanceTester< NoOfBits, FirstTransitionBit >::
          VisitAllStatesImpl( machine, bit - 1 );
      }

      machine.process_event( *pFlipBitEvents[ bit ] );
      ++PerformanceTester< NoOfBits, FirstTransitionBit >::eventsSent_;

      if ( bit > 0 )
      {
        PerformanceTester< NoOfBits, FirstTransitionBit >::
          VisitAllStatesImpl( machine, bit - 1 );
      }
    }

    // common prime factors of 2^n-1 for n in [1,8]
    static const unsigned int eventsToSend_ = 3 * 3 * 5 * 7 * 17 * 31 * 127;
    static unsigned int eventsSent_;
};

template< class NoOfBits, class FirstTransitionBit >
unsigned int PerformanceTester< NoOfBits, FirstTransitionBit >::eventsSent_;


//////////////////////////////////////////////////////////////////////////////
typedef std::vector< PerfResult > PerfResultList;

template< class NoOfBits >
struct PerfResultBackInserter
{
  public:
    PerfResultBackInserter( PerfResultList & perfResultList ) :
      perfResultList_( perfResultList )
    {
    }

    template< class FirstTransitionBit >
    void operator()( const FirstTransitionBit & )
    {
      perfResultList_.push_back(
        PerformanceTester< NoOfBits, FirstTransitionBit >::Test() );
    }

  private:
    // avoids C4512 (assignment operator could not be generated)
    PerfResultBackInserter & operator=( const PerfResultBackInserter & );

    PerfResultList & perfResultList_;
};

template< class NoOfBits >
std::vector< PerfResult > TestMachine()
{
  PerfResultList result;

  mpl::for_each< mpl::range_c< unsigned int, 0, NoOfBits::value + 1 > >(
    PerfResultBackInserter< NoOfBits >( result ) );

  return result;
}

template< class NoOfBits >
void TestAndWriteResults()
{
  PerfResultList results = TestMachine< NoOfBits >();

  std::fstream output;
  output.exceptions(
    std::ios_base::badbit | std::ios_base::eofbit | std::ios_base::failbit );

  std::string prefix = std::string( BOOST_COMPILER ) + "__";
  std::replace( prefix.begin(), prefix.end(), ' ', '_' );

  output.open( 
    ( prefix + std::string( 1, '0' + static_cast< char >( NoOfBits::value ) )
      + ".txt" ).c_str(),
    std::ios_base::out );

  for ( PerfResultList::const_iterator pResult = results.begin();
    pResult != results.end(); ++pResult )
  {
    output << std::fixed << std::setprecision( 0 ) <<
      std::setw( 8 ) << pResult->inStateRatio_ * 100 << ',' <<
      std::setw( 8 ) << pResult->nanoSecondsPerReaction_ << "\n";
  }
}


//////////////////////////////////////////////////////////////////////////////
int main()
{
  std::cout <<
    "Boost.Statechart in-state reaction vs. transition performance test\n\n";
  std::cout << "Press <CR> to start the test: ";

  {
    std::string input;
    std::getline( std::cin, input );
  }

  TestAndWriteResults< uint1 >();
  TestAndWriteResults< uint2 >();
  TestAndWriteResults< uint3 >();

  return 0;
}

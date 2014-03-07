//  timer, job_timer, and progress_display sample program  -------------------//

//  Copyright Beman Dawes 1998.  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/timer for documentation.

//  Revision History
//  12 Jan 01  Cut time to 1.0 secs to speed regression tests (Beman Dawes)
//  25 Sep 99  added elapsed_min() and elapsed_max() reporting
//  16 Jul 99  Second beta
//   6 Jul 99  Initial boost version

#include <boost/progress.hpp>
#include <iostream>
#include <climits>

using boost::timer;
using boost::progress_timer;
using boost::progress_display;
using std::cout;
using std::endl;

int main() {

  timer t0;  // used only for elapsed_max() and elapsed_min()

  cout << "timer::elapsed_min() reports " << t0.elapsed_min() << " seconds\n";
  cout << "timer::elapsed_max() reports " << t0.elapsed_max()
       << " seconds, which is " << t0.elapsed_max()/3600.0 << " hours\n";

  cout << "\nverify progress_display(0) doesn't divide by zero" << endl;
  progress_display zero( 0 );  // verify 0 doesn't divide by zero
  ++zero;

  long loops;
  timer loop_timer;
  const double time = 1.0;

  cout << "\ndetermine " << time << " second iteration count" << endl;
  for ( loops = 0; loops < LONG_MAX
     && loop_timer.elapsed() < time; ++loops ) {}
  cout << loops << " iterations"<< endl;
    
  long i;
  bool time_waster; // defeat [some] optimizers by storing result here

  progress_timer pt;
  timer t1;
  timer t4;
  timer t5;

  cout << "\nburn about " << time << " seconds" << endl;
  progress_display pd( loops );
  for ( i = loops; i--; )
    { time_waster = loop_timer.elapsed() < time; ++pd; }

  timer t2( t1 );
  timer t3;
  t4 = t3;
  t5.restart();

  cout << "\nburn about " << time << " seconds again" << endl;
  pd.restart( loops );
  for ( i = loops; i--; )
    { time_waster = loop_timer.elapsed() < time; ++pd; }

  if ( time_waster ) cout << ' ';  // using time_waster quiets compiler warnings
  progress_display pd2( 50, cout, "\nLead string 1 ", "Lead string 2 ", "Lead string 3 " );
  for ( ; pd2.count() < 50; ++pd2 ) {}

  cout << "\nt1 elapsed: " << t1.elapsed() << '\n';
  cout << "t2 elapsed: " << t2.elapsed() << '\n';
  cout << "t3 elapsed: " << t3.elapsed() << '\n';
  cout << "t4 elapsed: " << t4.elapsed() << '\n';
  cout << "t5 elapsed: " << t5.elapsed() << '\n';
  cout << "t1 and t2 should report the same times (very approximately "
       << 2*time << " seconds).\n";
  cout << "t3, t4 and t5 should report about the same times,\n";
  cout << "and these should be about half the t1 and t2 times.\n";
  cout << "The following elapsed time should be slightly greater than t1."
       << endl; 
  return 0;
  } // main





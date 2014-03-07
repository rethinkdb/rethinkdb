// ----------------------------------------------------------------------------
// sample_advanced.cc :  examples of adanced usage of format 
// ----------------------------------------------------------------------------

//  Copyright Samuel Krempp 2003. Use, modification, and distribution are
//  subject to the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/format for library home page

// ----------------------------------------------------------------------------

#include <iostream>
#include <iomanip>

#include "boost/format.hpp"


namespace MyNS_ForOutput {
  using std::cout; using std::cerr;
  using std::string;
  using std::endl; using std::flush;

  using boost::format;
  using boost::io::group;
}

namespace MyNS_Manips {
  using std::setfill;
  using std::setw;
  using std::hex ;
  using std::dec ;
  using std::showbase ;
  using std::left ;
  using std::right ;
  using std::internal ;
}

int main(){
    using namespace MyNS_ForOutput;
    using namespace MyNS_Manips;

    std::string s;

    //------------------------------------------------------------------------
    // storing the parsed format-string in a 'formatter' : 
    // format objects are regular objects that can be copied, assigned, 
    // fed arguments, dumped to a stream, re-fed arguments, etc... 
    // So users can use them the way they like.

    format fmter("%1% %2% %3% %1% \n");
    fmter % 10 % 20 % 30; 
    cout  << fmter;
    //          prints  "10 20 30 10 \n"
    
    // note that once the fmter got all its arguments, 
    // the formatted string stays available  (until next call to '%')
    //    The result is  available via function str() or stream's << :
    cout << fmter; 
    //          prints the same string again.


    // once you call operator% again, arguments are cleared inside the object
    // and it is an error to ask for the conversion string before feeding all arguments :
    fmter % 1001;
    try  { cout << fmter;   }
    catch (boost::io::too_few_args& exc) { 
      cout <<  exc.what() << "***Dont worry, that was planned\n";
    }

    // we just need to feed the last two arguments, and it will be ready for output again :
    cout << fmter % 1002 % 1003;
    //          prints  "1001 1002 1003 1001 \n"

    cout  << fmter % 10 % 1 % 2;
    //          prints  "10 1 2 10 \n"



    //---------------------------------------------------------------
    // using format objects 

    // modify the formatting options for a given directive :
    fmter = format("%1% %2% %3% %2% %1% \n");
    fmter.modify_item(4, group(setfill('_'), hex, showbase, setw(5)) );
    cout << fmter % 1 % 2 % 3;
    //          prints  "1 2 3 __0x2 1 \n"
    
    // bind one of the argumets :
    fmter.bind_arg(1, 18);
    cout << fmter % group(hex, showbase, 20) % 30;  // %2 is 20, and 20 == 0x14
    //          prints  "18 0x14 30  _0x14 18 \n"
    
    
    fmter.modify_item(4, setw(0)); // cancels previous width-5
    fmter.bind_arg(1, 77); // replace 18 with 77 for first argument.
    cout << fmter % 10 % 20;
    //          prints  "77 10 20 0xa 77 \n"

    try  
    { 
      cout << fmter % 6 % 7 % 8;   // Aye ! too many args, because arg1 is bound already
    }
    catch (boost::io::too_many_args& exc) 
    { 
      cout <<  exc.what() << "***Dont worry, that was planned\n";
    }

    // clear regular arguments, but not bound arguments :
    fmter.clear();
    cout << fmter % 2 % 3;
    //          prints "77 2 3 0x2 77 \n"

    // clear_binds() clears both regular AND bound arguments :
    fmter.clear_binds(); 
    cout << fmter % 1 % 2 % 3;
    //          prints  "1 2 3 0x2 1 \n"
    
 
    // setting desired exceptions :
    fmter.exceptions( boost::io::all_error_bits ^( boost::io::too_many_args_bit ) );
    cout << fmter % 1 % 2 % 3 % 4 % 5 % 6 ;


   // -----------------------------------------------------------
    // misc:

    // unsupported printf directives %n and asterisk-fields are purely ignored.
    // do *NOT* provide an argument for them, it is an error.
    cout << format("|%5d| %n") % 7 << endl;
    //          prints  "|    7| "
    cout << format("|%*.*d|")  % 7 << endl;
    //          prints "|7|"


    // truncations of strings :
    cout << format("%|.2s| %|8c|.\n") % "root" % "user";
    //          prints  "ro        u.\n"


    // manipulators conflicting with format-string : manipulators win.
    cout << format("%2s")  % group(setfill('0'), setw(6), 1) << endl;
    //          prints  "000001"
    cout << format("%2$5s %1% %2$3s\n")  % 1    % group(setfill('X'), setw(4), 2) ;
    //          prints  "XXX2 1 XXX2\n"  
    //          width is 4, as set by manip, not the format-string.
    
    // nesting :
    cout << format("%2$014x [%1%] %2$05s\n") % (format("%05s / %s") % -18 % 7)
                                             % group(showbase, -100);
    //          prints   "0x0000ffffff9c [-0018 / 7] -0100\n"


    cout << "\n\nEverything went OK, exiting. \n";
    return 0;
}

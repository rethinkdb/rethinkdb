// ----------------------------------------------------------------------------
// sample_userType.cc :  example usage of format with a user-defined type
// ----------------------------------------------------------------------------

//  Copyright Samuel Krempp 2003. Use, modification, and distribution are
//  subject to the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/format for library home page

// ----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include "boost/format.hpp"


#if !(BOOST_WORKAROUND(__GNUC__, < 3) && defined(__STL_CONFIG_H) ) 
  // not for broken gcc stdlib
#include <boost/io/ios_state.hpp>

#else
// not as complete, but compatible with gcc-2.95 :

void copyfmt(ios& left, const ios& right) {
    left.fill(right.fill());
    left.flags(right.flags() );
    left.exceptions(right.exceptions());
    left.width(right.width());
    left.precision(right.precision());
}

namespace boost { namespace io {
class ios_all_saver {
    std::basic_ios<char>  ios_;
    std::ios & target_r;
public:
    ios_all_saver(std::ios& right) : ios_(0), target_r(right) {
        copyfmt(ios_, right);
    }
    ~ios_all_saver() {
        copyfmt(target_r, ios_);
    }
};

} } // N.S. boost::io


// define showpos and noshowpos : 
class ShowPos {
public:
    bool showpos_;
    ShowPos(bool v) : showpos_(v) {}
};
std::ostream& operator<<(std::ostream& os, const ShowPos& x) { 
    if(x.showpos_) 
        os.setf(ios_base:: showpos);
    else
        os.unsetf(ios_base:: showpos);
    return os; 
}
ShowPos noshowpos(false);
ShowPos showpos(true);

#endif // -end gcc-2.95 workarounds



//---------------------------------------------------------------------------//
//  *** an exemple of UDT : a Rational class ****
class Rational {
public:
  Rational(int n, unsigned int d) : n_(n), d_(d) {}
  Rational(int n, int d);    // convert denominator to unsigned
  friend std::ostream& operator<<(std::ostream&, const Rational&);
private:
  int n_;               // numerator
  unsigned int d_;      // denominator
};

Rational::Rational(int n, int d) : n_(n) 
{
  if(d < 0) { n_ = -n_; d=-d; } // make the denominator always non-negative.
    d_ = static_cast<unsigned int>(d);
}

std::ostream& operator<<(std::ostream& os, const Rational& r) {
  using namespace std;
  streamsize  n, s1, s2, s3;
  streamsize w = os.width(0); // width has to be zeroed before saving state.
//  boost::io::ios_all_saver  bia_saver (os); 
 
  boost::io::basic_oaltstringstream<char> oss;
  oss.copyfmt(os );
  oss << r.n_; 
  s1 = oss.size();
  oss << "/" << noshowpos; // a rational number needs only one sign !
  s2 = oss.size();
  oss << r.d_ ;
  s3 = oss.size();

  n = w - s3;
  if(n <= 0) {
      os.write(oss.begin(), oss.size());
  }
  else if(os.flags() & std::ios_base::internal) {
    std::streamsize n1 = w/2, n2 = w - n1,  t;
    t = (s3-s1) - n2; // is 2d part '/nnn' bigger than 1/2 w ?
    if(t > 0)  {
      n1 = w -(s3-s1); // put all paddings on first part.
      n2 = 0; // minimal width (s3-s2)
    } 
    else {
      n2 -= s2-s1; // adjust for '/',   n2 is still w/2.
    }
    os << setw(n1) << r.n_ << "/" << noshowpos << setw(n2) << r.d_;
  }
  else {
    if(! (os.flags() & std::ios_base::left)) { 
        // -> right align. (right bit is set, or no bit is set)
        os << string(n, ' ');
    }
    os.write(oss.begin(), s3);
    if( os.flags() & std::ios_base::left ) {
      os << string(n, ' ');
    }
  }

  return os;
}



int main(){
    using namespace std;
    using boost::format;
    using boost::io::group; 
    using boost::io::str;
    string s;

    Rational  r(16, 9);

    cout << "bonjour ! " << endl;

    cout << r << endl;
    //          prints : "16/9" 

    cout << showpos << r << ", " << 5 << endl;
    //          prints : "+16/9, +5"

    cout << format("%02d : [%0+9d] \n") % 1 % r ;

    cout << format("%02d : [%_+9d] \n") % 2 % Rational(9,160);

    cout << format("%02d : [%_+9d] \n") % 3 % r;

    cout << format("%02d : [%_9d] \n") % 4 % Rational(8,1234);
    
    cout << format("%02d : [%_9d] \n") % 5 % Rational(1234,8);

    cout << format("%02d : [%09d] \n") % 6 % Rational(8,1234);

    cout << format("%02d : [%0+9d] \n") % 7 % Rational(1234,8);

    cout << format("%02d : [%0+9d] \n") % 8 % Rational(7,12345);

    /* output :
bonjour !
16/9
+16/9, +5
01 : [+016/0009]
02 : [+  9/ 160]
03 : [+ 16/   9]
04 : [   8/1234]
05 : [1234/   8]
06 : [0008/1234]
07 : [+1234/008]
08 : [+07/12345]
    */


    cerr << "\n\nEverything went OK, exiting. \n";
    return 0;
}

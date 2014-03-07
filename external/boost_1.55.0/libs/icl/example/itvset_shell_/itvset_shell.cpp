/*-----------------------------------------------------------------------------+
Copyright (c) 2007-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
Copyright (c) 1999-2006: Cortex Software GmbH, Kantstrasse 57, Berlin
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------+    
itvset_shell.cpp provides  a simple test shells for interval sets.
The shell also gives you a good idea how interval container are working.
+-----------------------------------------------------------------------------*/
#include <iostream>

#include <boost/icl/split_interval_set.hpp>
#include <boost/icl/split_interval_map.hpp>

using namespace std;
using namespace boost;
using namespace boost::icl;

void instructions()
{
    cout << "+++++ Test shell for interval set +++++\n";
    cout << "Type: q e or 0  to quit\n";
    cout << "Type: +         for insertions\n";
    cout << "Type: -         for subtraction\n";
    cout << "Type: j         to join contiguous intervals\n";
    cout << "Type: s         to compute total size\n";
}

void wrongInput()
{
    cout << "Wrong Input ------------------\n";
    instructions();
}


template <class SetTV>
void setTestShell()
{
    SetTV m1;

    try {
        char cmd = 'b';
        typename SetTV::domain_type lwb = typename SetTV::domain_type();
        typename SetTV::domain_type upb = typename SetTV::domain_type();

        instructions();

        for(;;)
        {
            cout << "> ";
            cin >> cmd ;

            switch(cmd) 
            {
            case 'q':
            case 'e':
            case '0': cout << "good bye\n"; return;
            case '+': 
                {
                    cout << "input: lwb upb >> ";
                    cin >> lwb >> upb;
                    typename SetTV::interval_type itv 
                        = typename SetTV::interval_type(lwb,upb);
                    // SetTV::IntervalTD itv = rightOpenInterval(lwb,upb);
                    m1.insert(itv);

                    cout << "+" << itv << " =" << endl;
                    cout << "{" << m1 << "}" << endl;

                }
                break;
            case '-': 
                {
                    cout << "input: lwb upb >> ";
                    cin >> lwb >> upb;
                    typename SetTV::interval_type itv 
                        = typename SetTV::interval_type(lwb,upb);
                    // m1.subtract(itv);
                    SetTV tmp;
                    tmp.insert(itv);
                    m1 -= tmp;

                    cout << "-" << itv << " =" << endl;
                    cout << "{" << m1 << "}" << endl;

                }
                break;
            case 'j':
                {
                    icl::join(m1);
                    cout << "{" << m1 << "}" << endl;
                }
                break;
            case 's':
                {
                    cout << "size = " << m1.size() << endl;
                }
                break;

            default: wrongInput();
            }
        }

    }
    catch (exception& e)
    {
        cout << "itvset_shell: exception caught: " << endl
             << e.what() << endl;
    }    
    catch (...)
    {
        cout << "itvset_shell: unknown exception caught" << endl;
    }    
}




int main()
{
    cout << ">>Interval Container Library: Test itvset_shell.cpp <<\n";
    cout << "------------------------------------------------------\n";
    setTestShell< interval_set<int> >();

    return 0;
}


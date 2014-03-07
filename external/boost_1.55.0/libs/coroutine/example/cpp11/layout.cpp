
//          Copyright Nat Goodspeed 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <utility>

#include <boost/coroutine/all.hpp>

struct FinalEOL{
    ~FinalEOL(){
        std::cout << std::endl;
    }
};

int main(int argc,char* argv[]){
    std::vector<std::string> words{
        "peas", "porridge", "hot", "peas",
        "porridge", "cold", "peas", "porridge",
        "in", "the", "pot", "nine",
        "days", "old" };

    int num=5,width=15;
    boost::coroutines::coroutine<std::string>::push_type writer(
        [&](boost::coroutines::coroutine<std::string>::pull_type& in){
            // finish the last line when we leave by whatever means
            FinalEOL eol;
            // pull values from upstream, lay them out 'num' to a line
            for (;;){
                for(int i=0;i<num;++i){
                    // when we exhaust the input, stop
                    if(!in) return;
                    std::cout << std::setw(width) << in.get();
                    // now that we've handled this item, advance to next
                    in();
                }
                // after 'num' items, line break
                std::cout << std::endl;
            }
        });

    std::copy(std::begin(words),std::end(words),std::begin(writer));

    return 0;
}

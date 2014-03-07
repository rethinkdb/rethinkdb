#include <fstream>
#include <ios>
#include <iostream>
#include <boost/integer_traits.hpp>
#include <boost/archive/binary_iarchive.hpp>

void usage(const char * program_name){
    std::cout << "usage:";
    std::cout << program_name << " filename" << std::endl;
}

int main(int argc, char *argv[]){
    if(argc != 2){
        std::cout << "invalid number of arguments" << std::endl;
        usage(argv[0]);
        return 1;
    }
    std::filebuf fb;
    fb.open(
        argv[1], 
        std::ios_base::binary | std::ios_base::in | std::ios_base::out
    );
    if(!fb.is_open()){
        std::cout << argv[1] <<  " failed to open" << std::endl;
        return 1;
    }
    boost::archive::binary_iarchive ia(fb);
    boost::archive::library_version_type lvt = ia.get_library_version();
    if(boost::archive::library_version_type(6) != lvt){
        std::cout << "library version not equal to six" << std::endl;
        return 1;
    }
    lvt = boost::archive::library_version_type(7);
    fb.pubseekpos(26, std::ios_base::out);
    fb.sputn(reinterpret_cast<const char *>(& lvt), sizeof(lvt));
    fb.close();
}

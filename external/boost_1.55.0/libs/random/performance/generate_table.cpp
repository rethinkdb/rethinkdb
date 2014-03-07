// generate_table.cpp
//
// Copyright (c) 2009
// Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <vector>
#include <utility>
#include <iostream>
#include <cstring>
#include <fstream>
#include <boost/regex.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

boost::regex generator_regex("(?:fixed-range )?([^:]+): (\\d+(?:\\.\\d+)?) nsec/loop = \\d+(?:\\.\\d+)? CPU cycles");
boost::regex distribution_regex("([^\\s]+)( virtual function)? ([^:]+): (\\d+(?:\\.\\d+)?) nsec/loop = \\d+(?:\\.\\d+)? CPU cycles");

std::string template_name(std::string arg) {
    return boost::regex_replace(arg, boost::regex("[^\\w]"), "_");
}

struct compare_second {
    template<class Pair>
    bool operator()(const Pair& p1, const Pair& p2) const {
        return (p1.second < p2.second);
    }
};

typedef boost::multi_index_container<
    std::string,
    boost::mpl::vector<
        boost::multi_index::sequenced<>,
        boost::multi_index::hashed_unique<boost::multi_index::identity<std::string> >
    >
> unique_list;

int main(int argc, char** argv) {

    std::string suffix;
    std::string id;

    if(argc >= 2 && std::strcmp(argv[1], "-linux") == 0) {
        suffix = "linux";
        id = "Linux";
    } else  {
        suffix = "windows";
        id = "Windows";
    }

    std::vector<std::pair<std::string, double> > generator_info;
    std::string line;
    while(std::getline(std::cin, line)) {
        boost::smatch match;
        if(std::strncmp(line.c_str(), "counting ", 9) == 0) break;
        if(boost::regex_match(line, match, generator_regex)) {
            std::string generator(match[1]);
            double time = boost::lexical_cast<double>(match[2]);
            if(generator != "counting") {
                generator_info.push_back(std::make_pair(generator, time));
            }
        } else {
            std::cerr << "oops: " << line << std::endl;
        }
    }

    double min = std::min_element(generator_info.begin(), generator_info.end(), compare_second())->second;

    std::ofstream generator_defs("performance_data.qbk");
    std::ofstream generator_performance(("generator_performance_" + suffix + ".qbk").c_str());
    generator_performance << "[table Basic Generators (" << id << ")\n";
    generator_performance << "  [[generator] [M rn/sec] [time per random number \\[nsec\\]] "
                             "[relative speed compared to fastest \\[percent\\]]]\n";

    typedef std::pair<std::string, double> pair_type;
    BOOST_FOREACH(const pair_type& pair, generator_info) {
        generator_defs << boost::format("[template %s_speed[] %d%%]\n")
            % template_name(pair.first) % static_cast<int>(100*min/pair.second);
        generator_performance << boost::format("  [[%s][%g][%g][%d%%]]\n")
            % pair.first % (1000/pair.second) % pair.second % static_cast<int>(100*min/pair.second);
    }
    generator_performance << "]\n";
    
    std::map<std::pair<std::string, std::string>, double> distribution_info;
    unique_list generator_names;
    unique_list distribution_names;
    do {
        boost::smatch match;
        if(boost::regex_match(line, match, distribution_regex)) {
            if(!match[2].matched && match[1] != "counting") {
                std::string generator(match[1]);
                std::string distribution(match[3]);
                double time = boost::lexical_cast<double>(match[4]);
                generator_names.push_back(generator);
                distribution_names.push_back(distribution);
                distribution_info.insert(std::make_pair(std::make_pair(distribution, generator), time));
            }
        } else {
            std::cerr << "oops: " << line << std::endl;
        }
    } while(std::getline(std::cin, line));

    std::ofstream distribution_performance(("distribution_performance_" + suffix + ".qbk").c_str());

    distribution_performance << "[table Distributions (" << id << ")\n";
    distribution_performance << "  [[\\[M rn/sec\\]]";
    BOOST_FOREACH(const std::string& generator, generator_names) {
        distribution_performance << boost::format("[%s]") % generator;
    }
    distribution_performance << "]\n";
    BOOST_FOREACH(const std::string& distribution, distribution_names) {
        distribution_performance << boost::format("  [[%s]") % distribution;
        BOOST_FOREACH(const std::string& generator, generator_names) {
            std::map<std::pair<std::string, std::string>, double>::iterator pos =
                distribution_info.find(std::make_pair(distribution, generator));
            if(pos != distribution_info.end()) {
                distribution_performance << boost::format("[%g]") % (1000/pos->second);
            } else {
                distribution_performance << "[-]";
            }
        }
        distribution_performance << "]\n";
    }
    distribution_performance << "]\n";
}

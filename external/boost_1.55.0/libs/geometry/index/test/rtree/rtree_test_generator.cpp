// Boost.Geometry Index
// Rtree tests generator

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <fstream>
#include <vector>
#include <string>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

int main()
{
    typedef boost::tuple<std::string, std::string> CT;
    std::vector<CT> coordinate_types;
    coordinate_types.push_back(boost::make_tuple("double", "d"));
    //coordinate_types.push_back(boost::make_tuple("int", "i"));
    //coordinate_types.push_back(boost::make_tuple("float", "f"));

    std::vector<std::string> dimensions;
    dimensions.push_back("2");
    dimensions.push_back("3");

    typedef boost::tuple<std::string, std::string> P;
    std::vector<P> parameters;
    parameters.push_back(boost::make_tuple("bgi::linear<5, 2>()", "lin"));
    parameters.push_back(boost::make_tuple("bgi::dynamic_linear(5, 2)", "dlin"));
    parameters.push_back(boost::make_tuple("bgi::quadratic<5, 2>()", "qua"));
    parameters.push_back(boost::make_tuple("bgi::dynamic_quadratic(5, 2)", "dqua"));
    parameters.push_back(boost::make_tuple("bgi::rstar<5, 2>()", "rst"));
    parameters.push_back(boost::make_tuple("bgi::dynamic_rstar(5, 2)","drst"));
    
    std::vector<std::string> indexables;
    indexables.push_back("p");
    indexables.push_back("b");

    typedef std::pair<std::string, std::string> TS;
    std::vector<TS> testsets;
    testsets.push_back(std::make_pair("testset::modifiers", "mod"));
    testsets.push_back(std::make_pair("testset::queries", "que"));
    testsets.push_back(std::make_pair("testset::additional", "add"));

    BOOST_FOREACH(P const& p, parameters)
    {
        BOOST_FOREACH(TS const& ts, testsets)
        {
            BOOST_FOREACH(std::string const& i, indexables)
            {
                BOOST_FOREACH(std::string const& d, dimensions)
                {
                    BOOST_FOREACH(CT const& c, coordinate_types)   
                    {
                        std::string filename = std::string() +
                            "rtree_" + boost::get<1>(p) + '_' + ts.second + '_' + i + d + boost::get<1>(c) + ".cpp";

                        std::ofstream f(filename.c_str(), std::ios::trunc);

                        f <<
                            "// Boost.Geometry Index\n" <<
                            "// Unit Test\n" <<
                            "\n" <<
                            "// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.\n" <<
                            "\n" <<
                            "// Use, modification and distribution is subject to the Boost Software License,\n" <<
                            "// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at\n" <<
                            "// http://www.boost.org/LICENSE_1_0.txt)\n" <<
                            "\n";

                        f <<
                            "#include <rtree/test_rtree.hpp>\n" <<
                            "\n" <<
                            "#include <boost/geometry/geometries/point.hpp>\n" <<
                            "#include <boost/geometry/geometries/box.hpp>\n" <<
                            "\n";

                        std::string point_type = std::string("bg::model::point<") + boost::get<0>(c) + ", " + d + ", bg::cs::cartesian>";
                        std::string box_type = std::string("bg::model::box< ") + point_type + " >";
                        std::string indexable_type = (i == "p" ? point_type : box_type);

                        f <<
                            "int test_main(int, char* [])\n" <<
                            "{\n" <<
                            "    typedef " << indexable_type << " Indexable;\n" <<
                            "    " << ts.first << "<Indexable>(" << boost::get<0>(p) << ", std::allocator<int>());\n" <<
                            "    return 0;\n" <<
                            "}\n";
                    }
                }
            }           

        }
    }

    return 0;
}

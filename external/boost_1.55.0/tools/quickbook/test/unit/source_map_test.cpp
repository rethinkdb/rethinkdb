/*=============================================================================
    Copyright (c) 2012 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "fwd.hpp"
#include "files.hpp"
#include <boost/utility/string_ref.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/range/algorithm/find.hpp>

void simple_map_tests()
{
    boost::string_ref source("First Line\nSecond Line");
    quickbook::file_ptr fake_file = new quickbook::file(
        "(fake file)", source, 105u);

    quickbook::string_iterator line1 = fake_file->source().begin();
    quickbook::string_iterator line1_end = boost::find(fake_file->source(), '\n');
    quickbook::string_iterator line2 = line1_end + 1;
    quickbook::string_iterator line2_end = fake_file->source().end();

    quickbook::mapped_file_builder builder;
    
    { // Empty test
        builder.start(fake_file);
        BOOST_TEST(builder.empty());
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST(f1->source().empty());
    }
    
    { // Add full text
        builder.start(fake_file);
        builder.add(boost::string_ref(line1, line2_end - line1));
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(), source);
        BOOST_TEST_EQ(f1->position_of(f1->source().begin()),
            quickbook::file_position(1,1));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 2),
            quickbook::file_position(1,3));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + (line1_end - line1)),
            quickbook::file_position(1,line1_end - line1 + 1));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + (line2 - line1)),
            quickbook::file_position(2,1));
        BOOST_TEST_EQ(f1->position_of(f1->source().end()),
            fake_file->position_of(fake_file->source().end()));
    }

    { // Add first line
        builder.start(fake_file);
        builder.add(boost::string_ref(line1, line1_end - line1));
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(),
            boost::string_ref(source.begin(), line1_end - line1));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin()),
            quickbook::file_position(1,1));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 2),
            quickbook::file_position(1,3));
        BOOST_TEST_EQ(f1->position_of(f1->source().end()),
            quickbook::file_position(1,line1_end - line1 + 1));
    }

    { // Add second line
        builder.start(fake_file);
        builder.add(boost::string_ref(line2, line2_end - line2));
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(), boost::string_ref("Second Line"));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin()),
            quickbook::file_position(2,1));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 2),
            quickbook::file_position(2,3));
        BOOST_TEST_EQ(f1->position_of(f1->source().end()),
            quickbook::file_position(2,line2_end - line2 + 1));
    }

    { // Out of order
        builder.start(fake_file);
        builder.add(boost::string_ref(line2, line2_end - line2));
        builder.add(boost::string_ref(line1_end, 1));
        builder.add(boost::string_ref(line1, line1_end - line1));
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(),
            boost::string_ref("Second Line\nFirst Line"));

        BOOST_TEST_EQ(f1->position_of(f1->source().begin()),
            quickbook::file_position(2,1));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 2),
            quickbook::file_position(2,3));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + (line2_end - line2 - 1)),
            quickbook::file_position(2,line2_end - line2));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + (line2_end - line2)),
            quickbook::file_position(1,(line1_end - line1 + 1)));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + (line2_end - line2 + 1)),
            quickbook::file_position(1,1));
        BOOST_TEST_EQ(f1->position_of(f1->source().end()),
            quickbook::file_position(1,line1_end - line1 + 1));
    }

    { // Repeated text
        builder.start(fake_file);
        builder.add(boost::string_ref(line2, line2_end - line2));
        builder.add(boost::string_ref(line1_end, 1));
        builder.add(boost::string_ref(line2, line2_end - line2));
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(),
            boost::string_ref("Second Line\nSecond Line"));

        BOOST_TEST_EQ(f1->position_of(f1->source().begin()),
            quickbook::file_position(2,1));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 2),
            quickbook::file_position(2,3));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + (line2_end - line2 - 1)),
            quickbook::file_position(2,line2_end - line2));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + (line2_end - line2)),
            quickbook::file_position(1,(line1_end - line1 + 1)));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + (line2_end - line2 + 1)),
            quickbook::file_position(2,1));
        BOOST_TEST_EQ(f1->position_of(f1->source().end()),
            quickbook::file_position(2,line2_end - line2 + 1));
    }


    { // Generated text
        builder.start(fake_file);
        builder.add_at_pos("------\n", line1);
        builder.add(boost::string_ref(line1, line1_end - line1));
        builder.add_at_pos("\n------\n", line1_end);
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(),
            boost::string_ref("------\nFirst Line\n------\n"));
        
        quickbook::string_iterator newline = boost::find(f1->source(), '\n');

        BOOST_TEST_EQ(f1->position_of(f1->source().begin()),
            quickbook::file_position(1,1));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 2),
            quickbook::file_position(1,1));
        BOOST_TEST_EQ(f1->position_of(newline),
            quickbook::file_position(1,1));
        BOOST_TEST_EQ(f1->position_of(newline + 1),
            quickbook::file_position(1,1));
        BOOST_TEST_EQ(f1->position_of(newline + 2),
            quickbook::file_position(1,2));
        BOOST_TEST_EQ(f1->position_of(newline + (line1_end - line1)),
            quickbook::file_position(1,line1_end - line1));
        BOOST_TEST_EQ(f1->position_of(newline + (line1_end - line1 + 1)),
            quickbook::file_position(1,line1_end - line1 + 1));
        BOOST_TEST_EQ(f1->position_of(newline + (line1_end - line1 + 2)),
            quickbook::file_position(1,line1_end - line1 + 1));
        BOOST_TEST_EQ(f1->position_of(f1->source().end()),
            quickbook::file_position(1,line1_end - line1 + 1));
    }
}

void indented_map_tests()
{
    boost::string_ref source(
        "   Code line1\n"
        "   Code line2\n");
    quickbook::file_ptr fake_file = new quickbook::file(
        "(fake file)", source, 105u);

    quickbook::mapped_file_builder builder;
    
    {
        builder.start(fake_file);
        builder.unindent_and_add(fake_file->source());
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(),
            boost::string_ref("Code line1\nCode line2\n"));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin()),
            quickbook::file_position(1,4));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 1),
            quickbook::file_position(1,5));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 5),
            quickbook::file_position(1,9));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 10),
            quickbook::file_position(1,14));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 11),
            quickbook::file_position(2,4));
        // TODO: Shouldn't this be (3,1)? Does it matter?
        BOOST_TEST_EQ(f1->position_of(f1->source().end()),
            quickbook::file_position(3,1));
    }

    {
        builder.start(fake_file);
        {
            quickbook::mapped_file_builder builder2;
            builder2.start(fake_file);
            builder2.unindent_and_add(fake_file->source());
            builder.add(builder2);
        }
        quickbook::file_ptr f1 = builder.release();

        BOOST_TEST_EQ(f1->source(),
            boost::string_ref("Code line1\nCode line2\n"));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin()),
            quickbook::file_position(1,4));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 1),
            quickbook::file_position(1,5));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 5),
            quickbook::file_position(1,9));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 10),
            quickbook::file_position(1,14));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 11),
            quickbook::file_position(2,4));
        BOOST_TEST_EQ(f1->position_of(f1->source().end()),
            quickbook::file_position(3,1));
    }

    {
        builder.start(fake_file);
        builder.unindent_and_add(boost::string_ref(
            fake_file->source().begin() + 3,
            fake_file->source().end() - (fake_file->source().begin() + 3)));
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(),
            boost::string_ref("Code line1\n   Code line2\n"));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin()),
            quickbook::file_position(1,4));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 1),
            quickbook::file_position(1,5));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 5),
            quickbook::file_position(1,9));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 10),
            quickbook::file_position(1,14));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 11),
            quickbook::file_position(2,1));
        BOOST_TEST_EQ(f1->position_of(f1->source().end()),
            quickbook::file_position(3,1));
    }
}

void indented_map_tests2()
{
    boost::string_ref source(
        "   Code line1\n"
        "\n"
        "   Code line2\n");
    quickbook::file_ptr fake_file = new quickbook::file(
        "(fake file)", source, 105u);

    quickbook::mapped_file_builder builder;
    
    {
        builder.start(fake_file);
        builder.unindent_and_add(fake_file->source());
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(),
            boost::string_ref("Code line1\n\nCode line2\n"));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin()),
            quickbook::file_position(1,4));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 1),
            quickbook::file_position(1,5));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 5),
            quickbook::file_position(1,9));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 10),
            quickbook::file_position(1,14));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 11),
            quickbook::file_position(2,1));
        BOOST_TEST_EQ(f1->position_of(f1->source().begin() + 12),
            quickbook::file_position(3,4));
    }
}

void indented_map_leading_blanks_test()
{
    quickbook::mapped_file_builder builder;

    {
        boost::string_ref source("\n\n   Code line1\n");
        quickbook::file_ptr fake_file = new quickbook::file(
            "(fake file)", source, 105u);
        builder.start(fake_file);
        builder.unindent_and_add(fake_file->source());
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(),
            boost::string_ref("Code line1\n"));
    }

    {
        boost::string_ref source("    \n  \n   Code line1\n");
        quickbook::file_ptr fake_file = new quickbook::file(
            "(fake file)", source, 105u);
        builder.start(fake_file);
        builder.unindent_and_add(fake_file->source());
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(),
            boost::string_ref("Code line1\n"));
    }

    {
        boost::string_ref source("   Code line1\n \n   Code line2");
        quickbook::file_ptr fake_file = new quickbook::file(
            "(fake file)", source, 105u);
        builder.start(fake_file);
        builder.unindent_and_add(fake_file->source());
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(),
            boost::string_ref("Code line1\n\nCode line2"));
    }
}

void indented_map_trailing_blanks_test()
{
    quickbook::mapped_file_builder builder;

    {
        boost::string_ref source("\n\n   Code line1\n  ");
        quickbook::file_ptr fake_file = new quickbook::file(
            "(fake file)", source, 105u);
        builder.start(fake_file);
        builder.unindent_and_add(fake_file->source());
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(),
            boost::string_ref("Code line1\n"));
    }

    {
        boost::string_ref source("    \n  \n   Code line1\n    ");
        quickbook::file_ptr fake_file = new quickbook::file(
            "(fake file)", source, 105u);
        builder.start(fake_file);
        builder.unindent_and_add(fake_file->source());
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(),
            boost::string_ref("Code line1\n "));
    }

    {
        boost::string_ref source("   Code line1\n \n   Code line2\n  ");
        quickbook::file_ptr fake_file = new quickbook::file(
            "(fake file)", source, 105u);
        builder.start(fake_file);
        builder.unindent_and_add(fake_file->source());
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(),
            boost::string_ref("Code line1\n\nCode line2\n"));
    }

}

void indented_map_mixed_test()
{
    quickbook::mapped_file_builder builder;

    {
        boost::string_ref source("\tCode line 1\n    Code line 2\n\t    Code line 3\n    \tCode line 4");
        quickbook::file_ptr fake_file = new quickbook::file(
            "(fake file)", source, 105u);
        builder.start(fake_file);
        builder.unindent_and_add(fake_file->source());
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(),
            boost::string_ref("Code line 1\nCode line 2\n    Code line 3\n    Code line 4"));
    }

    {
        boost::string_ref source("  Code line 1\n\tCode line 2");
        quickbook::file_ptr fake_file = new quickbook::file(
            "(fake file)", source, 105u);
        builder.start(fake_file);
        builder.unindent_and_add(fake_file->source());
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(),
            boost::string_ref("Code line 1\n  Code line 2"));
    }

    {
        boost::string_ref source("  Code line 1\n  \tCode line 2");
        quickbook::file_ptr fake_file = new quickbook::file(
            "(fake file)", source, 105u);
        builder.start(fake_file);
        builder.unindent_and_add(fake_file->source());
        quickbook::file_ptr f1 = builder.release();
        BOOST_TEST_EQ(f1->source(),
            boost::string_ref("Code line 1\n\tCode line 2"));
    }
}


int main()
{
    simple_map_tests();
    indented_map_tests();
    indented_map_tests2();
    indented_map_leading_blanks_test();
    indented_map_trailing_blanks_test();
    indented_map_mixed_test();
    return boost::report_errors();
}


//          Copyright Nat Goodspeed 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <iomanip>
#include <string>
#include <cctype>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/coroutine/all.hpp>
#include <boost/foreach.hpp>

typedef boost::coroutines::coroutine<std::string> coro_t;

// deliver each line of input stream to sink as a separate string
void readlines(coro_t::push_type& sink, std::istream& in)
{
    std::string line;
    while (std::getline(in, line))
        sink(line);
}

void tokenize(coro_t::push_type& sink, coro_t::pull_type& source)
{
    // This tokenizer doesn't happen to be stateful: you could reasonably
    // implement it with a single call to push each new token downstream. But
    // I've worked with stateful tokenizers, in which the meaning of input
    // characters depends in part on their position within the input line. At
    // the time, I wished for a way to resume at the suspend point!
    BOOST_FOREACH(std::string line, source)
    {
        std::string::size_type pos = 0;
        while (pos < line.length())
        {
            if (line[pos] == '"')
            {
                std::string token;
                ++pos;              // skip open quote
                while (pos < line.length() && line[pos] != '"')
                    token += line[pos++];
                ++pos;              // skip close quote
                sink(token);        // pass token downstream
            }
            else if (std::isspace(line[pos]))
            {
                ++pos;              // outside quotes, ignore whitespace
            }
            else if (std::isalpha(line[pos]))
            {
                std::string token;
                while (pos < line.length() && std::isalpha(line[pos]))
                    token += line[pos++];
                sink(token);        // pass token downstream
            }
            else                    // punctuation
            {
                sink(std::string(1, line[pos++]));
            }
        }
    }
}

void only_words(coro_t::push_type& sink, coro_t::pull_type& source)
{
    BOOST_FOREACH(std::string token, source)
    {
        if (! token.empty() && std::isalpha(token[0]))
            sink(token);
    }
}

void trace(coro_t::push_type& sink, coro_t::pull_type& source)
{
    BOOST_FOREACH(std::string token, source)
    {
        std::cout << "trace: '" << token << "'\n";
        sink(token);
    }
}

struct FinalEOL
{
    ~FinalEOL() { std::cout << std::endl; }
};

void layout(coro_t::pull_type& source, int num, int width)
{
    // Finish the last line when we leave by whatever means
    FinalEOL eol;

    // Pull values from upstream, lay them out 'num' to a line
    for (;;)
    {
        for (int i = 0; i < num; ++i)
        {
            // when we exhaust the input, stop
            if (! source)
                return;

            std::cout << std::setw(width) << source.get();
            // now that we've handled this item, advance to next
            source();
        }
        // after 'num' items, line break
        std::cout << std::endl;
    }
}

int main(int argc, char *argv[])
{
    // For example purposes, instead of having a separate text file in the
    // local filesystem, construct an istringstream to read.
    std::string data(
        "This is the first line.\n"
        "This, the second.\n"
        "The third has \"a phrase\"!\n"
        );

    {
        std::cout << "\nreadlines:\n";
        std::istringstream infile(data);
        // Each coroutine-function has a small, specific job to do. Instead of
        // adding conditional logic to a large, complex input function, the
        // caller composes smaller functions into the desired processing
        // chain.
        coro_t::pull_type reader(boost::bind(readlines, _1, boost::ref(infile)));
        coro_t::pull_type tracer(boost::bind(trace, _1, boost::ref(reader)));
        BOOST_FOREACH(std::string line, tracer)
        {
            std::cout << "got: " << line << "\n";
        }
    }

    {
        std::cout << "\ncompose a chain:\n";
        std::istringstream infile(data);
        coro_t::pull_type reader(boost::bind(readlines, _1, boost::ref(infile)));
        coro_t::pull_type tokenizer(boost::bind(tokenize, _1, boost::ref(reader)));
        coro_t::pull_type tracer(boost::bind(trace, _1, boost::ref(tokenizer)));
        BOOST_FOREACH(std::string token, tracer)
        {
            // just iterate, we're already pulling through tracer
        }
    }

    {
        std::cout << "\nfilter:\n";
        std::istringstream infile(data);
        coro_t::pull_type reader(boost::bind(readlines, _1, boost::ref(infile)));
        coro_t::pull_type tokenizer(boost::bind(tokenize, _1, boost::ref(reader)));
        coro_t::pull_type filter(boost::bind(only_words, _1, boost::ref(tokenizer)));
        coro_t::pull_type tracer(boost::bind(trace, _1, boost::ref(filter)));
        BOOST_FOREACH(std::string token, tracer)
        {
            // just iterate, we're already pulling through tracer
        }
    }

    {
        std::cout << "\nlayout() as coroutine::push_type:\n";
        std::istringstream infile(data);
        coro_t::pull_type reader(boost::bind(readlines, _1, boost::ref(infile)));
        coro_t::pull_type tokenizer(boost::bind(tokenize, _1, boost::ref(reader)));
        coro_t::pull_type filter(boost::bind(only_words, _1, boost::ref(tokenizer)));
        coro_t::push_type writer(boost::bind(layout, _1, 5, 15));
        BOOST_FOREACH(std::string token, filter)
        {
            writer(token);
        }
    }

    {
        std::cout << "\ncalling layout() directly:\n";
        std::istringstream infile(data);
        coro_t::pull_type reader(boost::bind(readlines, _1, boost::ref(infile)));
        coro_t::pull_type tokenizer(boost::bind(tokenize, _1, boost::ref(reader)));
        coro_t::pull_type filter(boost::bind(only_words, _1, boost::ref(tokenizer)));
        // Because of the symmetry of the API, we can directly call layout()
        // instead of using it as a coroutine-function.
        layout(filter, 5, 15);
    }

    {
        std::cout << "\nfiltering output:\n";
        std::istringstream infile(data);
        coro_t::pull_type reader(boost::bind(readlines, _1, boost::ref(infile)));
        coro_t::pull_type tokenizer(boost::bind(tokenize, _1, boost::ref(reader)));
        coro_t::push_type writer(boost::bind(layout, _1, 5, 15));
        // Because of the symmetry of the API, we can use any of these
        // chaining functions in a push_type coroutine chain as well.
        coro_t::push_type filter(boost::bind(only_words, boost::ref(writer), _1));
        BOOST_FOREACH(std::string token, tokenizer)
        {
            filter(token);
        }
    }

    return 0;
}

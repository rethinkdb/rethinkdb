// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_DICTIONARY_FILTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_DICTIONARY_FILTER_HPP_INCLUDED

#include <algorithm>         // swap.
#include <cassert>
#include <cstdio>            // EOF.
#include <iostream>          // cin, cout.
#include <cctype>
#include <map>
#include <boost/config.hpp>  // BOOST_NO_STDC_NAMESPACE.
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/filter/stdio.hpp>
#include <boost/iostreams/operations.hpp>

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std {
    using ::isalpha;
    using ::isupper;
    using ::toupper;
    using ::tolower;
}
#endif

namespace boost { namespace iostreams { namespace example {

class dictionary {
public:
    void add(std::string key, const std::string& value);
    void replace(std::string& key);
private:
    typedef std::map<std::string, std::string> map_type;
    void tolower(std::string& str);
    map_type map_;
};

class dictionary_stdio_filter : public stdio_filter {
public:
    dictionary_stdio_filter(dictionary& d) : dictionary_(d) { }
private:
    void do_filter()
    {
        using namespace std;
        while (true) {
            int c = std::cin.get();
            if (c == EOF || !std::isalpha((unsigned char) c)) {
                dictionary_.replace(current_word_);
                cout.write( current_word_.data(),
                            static_cast<std::streamsize>(current_word_.size()) );
                current_word_.erase();
                if (c == EOF)
                    break;
                cout.put(c);
            } else {
                current_word_ += c;
            }
        }
    }
    dictionary&  dictionary_;
    std::string  current_word_;
};

class dictionary_input_filter : public input_filter {
public:
    dictionary_input_filter(dictionary& d)
        : dictionary_(d), off_(std::string::npos), eof_(false)
        { }

    template<typename Source>
    int get(Source& src)
        {
            // Handle unfinished business.
            if (off_ != std::string::npos && off_ < current_word_.size())
                return current_word_[off_++];
            if (off_ == current_word_.size()) {
                current_word_.erase();
                off_ = std::string::npos;
            }
            if (eof_)
                return EOF;

            // Compute curent word.
            while (true) {
                int c;
                if ((c = iostreams::get(src)) == WOULD_BLOCK)
                    return WOULD_BLOCK;

                if (c == EOF || !std::isalpha((unsigned char) c)) {
                    dictionary_.replace(current_word_);
                    off_ = 0;
                    if (c == EOF)
                        eof_ = true;
                    else
                        current_word_ += c;
                    break;
                } else {
                    current_word_ += c;
                }
            }

            return this->get(src); // Note: current_word_ is not empty.
        }

    template<typename Source>
    void close(Source&)
    {
        current_word_.erase();
        off_ = std::string::npos;
        eof_ = false;
    }
private:
    dictionary&             dictionary_;
    std::string             current_word_;
    std::string::size_type  off_;
    bool                    eof_;
};

class dictionary_output_filter : public output_filter {
public:
    typedef std::map<std::string, std::string> map_type;
    dictionary_output_filter(dictionary& d)
        : dictionary_(d), off_(std::string::npos)
        { }

    template<typename Sink>
    bool put(Sink& dest, int c)
    {
        if (off_ != std::string::npos && !write_current_word(dest))
            return false;
        if (!std::isalpha((unsigned char) c)) {
            dictionary_.replace(current_word_);
            off_ = 0;
        }

        current_word_ += c;
        return true;
    }

    template<typename Sink>
    void close(Sink& dest)
    {
        // Reset current_word_ and off_, saving old values.
        std::string             current_word;
        std::string::size_type  off = 0;
        current_word.swap(current_word_);
        std::swap(off, off_);

        // Write remaining characters to dest.
        if (off == std::string::npos) {
            dictionary_.replace(current_word);
            off = 0;
        }
        if (!current_word.empty())
            iostreams::write(
                dest,
                current_word.data() + off,
                static_cast<std::streamsize>(current_word.size() - off)
            );
    }
private:
    template<typename Sink>
    bool write_current_word(Sink& dest)
    {
        using namespace std;
        std::streamsize amt = 
            static_cast<std::streamsize>(current_word_.size() - off_);
        std::streamsize result =
            iostreams::write(dest, current_word_.data() + off_, amt);
        if (result == amt) {
            current_word_.erase();
            off_ = string::npos;
            return true;
        } else {
            off_ += result;
            return false;
        }
    }

    dictionary&             dictionary_;
    std::string             current_word_;
    std::string::size_type  off_;
};

//------------------Implementation of dictionary------------------------------//

inline void dictionary::add(std::string key, const std::string& value)
{
    tolower(key);
    map_[key] = value;
}

inline void dictionary::replace(std::string& key)
{
    using namespace std;
    string copy(key);
    tolower(copy);
    map_type::iterator it = map_.find(key);
    if (it == map_.end())
        return;
    string& value = it->second;
    if (!value.empty() && !key.empty() && std::isupper((unsigned char) key[0]))
        value[0] = std::toupper((unsigned char) value[0]);
    key = value;
    return;
}

inline void dictionary::tolower(std::string& str)
{
    for (std::string::size_type z = 0, len = str.size(); z < len; ++z)
        str[z] = std::tolower((unsigned char) str[z]);
}

} } }       // End namespaces example, iostreams, boost.

#endif      // #ifndef BOOST_IOSTREAMS_DICTIONARY_FILTER_HPP_INCLUDED

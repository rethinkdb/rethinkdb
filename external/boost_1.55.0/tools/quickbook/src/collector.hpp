/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_QUICKBOOK_COLLECTOR_HPP)
#define BOOST_SPIRIT_QUICKBOOK_COLLECTOR_HPP

#include <string>
#include <stack>
#include <boost/ref.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_stream.hpp>

namespace quickbook
{
    struct string_stream
    {
        typedef boost::iostreams::filtering_ostream ostream;

        string_stream();
        string_stream(string_stream const& other);
        string_stream& operator=(string_stream const& other);

        std::string const& str() const
        {
            stream_ptr->flush();
            return *buffer_ptr.get();
        }
    
        std::ostream& get() const
        {
            return *stream_ptr.get();
        }
    
        void clear()
        {
            buffer_ptr->clear();
        }

        void swap(std::string& other)
        {
            stream_ptr->flush();
            std::swap(other, *buffer_ptr.get());
        }

        void append(std::string const& other)
        {
            stream_ptr->flush();
            *buffer_ptr.get() += other;
        }

    private:

        boost::shared_ptr<std::string> buffer_ptr;
        boost::shared_ptr<ostream> stream_ptr;
    };

    struct collector : boost::noncopyable
    {
        collector();
        collector(string_stream& out);
        ~collector();
        
        void push();
        void pop();

        std::ostream& get() const
        {
            return top.get().get();
        }
        
        std::string const& str() const
        {
            return top.get().str();
        }
        
        void clear()
        {
            top.get().clear();
        }
        
        void swap(std::string& other)
        {
            top.get().swap(other);
        }

        void append(std::string const& other)
        {
            top.get().append(other);
        }

    private:

        std::stack<string_stream> streams;
        boost::reference_wrapper<string_stream> main;
        boost::reference_wrapper<string_stream> top;
        string_stream default_;
    };
    
    template <typename T>
    inline collector& 
    operator<<(collector& out, T const& val)
    {
        out.get() << val;
        return out;
    }

    inline collector& 
    operator<<(collector& out, std::string const& val)
    {
        out.append(val);
        return out;
    }
}

#endif // BOOST_SPIRIT_QUICKBOOK_COLLECTOR_HPP


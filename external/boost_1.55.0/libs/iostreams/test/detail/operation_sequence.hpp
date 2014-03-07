/*
 *
 * Distributed under the Boost Software License, Version 1.0.(See accompanying 
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)
 * 
 * See http://www.boost.org/libs/iostreams for documentation.
 *
 * Defines the classes operation_sequence and operation, in the namespace
 * boost::iostreams::test, for verifying that all elements of a sequence of 
 * operations are executed, and that they are executed in the correct order.
 *
 * File:        libs/iostreams/test/detail/operation_sequence.hpp
 * Date:        Mon Dec 10 18:58:19 MST 2007
 * Copyright:   2007-2008 CodeRage, LLC
 * Author:      Jonathan Turkanis
 * Contact:     turkanis at coderage dot com
 */

#ifndef BOOST_IOSTREAMS_TEST_OPERATION_SEQUENCE_HPP_INCLUDED
#define BOOST_IOSTREAMS_TEST_OPERATION_SEQUENCE_HPP_INCLUDED

#include <boost/config.hpp>  // make sure size_t is in namespace std
#include <cstddef>
#include <climits>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>  // pair
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/weak_ptr.hpp>

#ifndef BOOST_IOSTREAMS_TEST_MAX_OPERATION_ERROR
# define BOOST_IOSTREAMS_TEST_MAX_OPERATION_ERROR 20
#endif

#define BOOST_CHECK_OPERATION_SEQUENCE(seq) \
    BOOST_CHECK_MESSAGE(seq.is_success(), seq.message()) \
    /**/

namespace boost { namespace iostreams { namespace test {

// Simple exception class with error code built in to type
template<int Code>
struct operation_error { };

class operation_sequence;

// Represent an operation in a sequence of operations to be executed
class operation {
public:
    friend class operation_sequence;
    operation() : pimpl_() { }
    void execute();
private:
    static void remove_operation(operation_sequence& seq, int id);

    struct impl {
        impl(operation_sequence& seq, int id, int error_code = -1)
            : seq(seq), id(id), error_code(error_code)
            { }
        ~impl() { remove_operation(seq, id); }
        impl& operator=(const impl&); // Supress VC warning 4512
        operation_sequence&  seq;
        int                  id;
        int                  error_code;
    };
    friend struct impl;

    operation(operation_sequence& seq, int id, int error_code = -1)
        : pimpl_(new impl(seq, id, error_code))
        { }

    shared_ptr<impl> pimpl_;
};

// Represents a sequence of operations to be executed in a particular order
class operation_sequence {
public:
    friend class operation;
    operation_sequence() { reset(); }

    //
    // Returns a new operation.
    // Parameters:
    //
    //   id - The operation id, determining the position
    //        of the new operation in the operation sequence
    //   error_code - If supplied, indicates that the new
    //        operation will throw operation_error<error_code>
    //        when executed. Must be an integer between 0 and
    //        BOOST_IOSTREAMS_TEST_MAX_OPERATION_ERROR,
    //        inclusive.
    // 
    operation new_operation(int id, int error_code = INT_MAX);

    bool is_success() const { return success_; }
    bool is_failure() const { return failed_; }
    std::string message() const;
    void reset();
private:
    void execute(int id);
    void remove_operation(int id);
    operation_sequence(const operation_sequence&);
    operation_sequence& operator=(const operation_sequence&);

    typedef weak_ptr<operation::impl>  ptr_type;      
    typedef std::map<int, ptr_type>    map_type;

    map_type          operations_;
    std::vector<int>  log_;
    std::size_t       total_executed_;
    int               last_executed_;
    bool              success_;
    bool              failed_;
};

//--------------Implementation of operation-----------------------------------//

void operation::execute()
{
    pimpl_->seq.execute(pimpl_->id);
    switch (pimpl_->error_code) {

    // Implementation with one or more cleanup operations
    #define BOOST_PP_LOCAL_MACRO(n) \
    case n: throw operation_error<n>(); \
    /**/

    #define BOOST_PP_LOCAL_LIMITS (1, BOOST_IOSTREAMS_TEST_MAX_OPERATION_ERROR)
    #include BOOST_PP_LOCAL_ITERATE()
    #undef BOOST_PP_LOCAL_MACRO

    default:
        break;
    }
}

inline void operation::remove_operation(operation_sequence& seq, int id)
{
    seq.remove_operation(id);
}

//--------------Implementation of operation_sequence--------------------------//

inline operation operation_sequence::new_operation(int id, int error_code)
{
    using namespace std;
    if ( error_code < 0 || 
         (error_code > BOOST_IOSTREAMS_TEST_MAX_OPERATION_ERROR &&
             error_code != INT_MAX) )
    {
        throw runtime_error( string("The error code ") + 
                             lexical_cast<string>(error_code) +
                             " is out of range" );
    }
    if (last_executed_ != INT_MIN)
        throw runtime_error( "Operations in progress; call reset() "
                             "before creating more operations" );
    map_type::const_iterator it = operations_.find(id);
    if (it != operations_.end())
        throw runtime_error( string("The operation ") + 
                             lexical_cast<string>(id) +
                             " already exists" );
    operation op(*this, id, error_code);
    operations_.insert(make_pair(id, ptr_type(op.pimpl_)));
    return op;
}

inline std::string operation_sequence::message() const
{
    using namespace std;
    if (success_) 
        return "success";
    std::string msg = failed_ ?
        "operations occurred out of order: " :
        "operation sequence is incomplete: ";
    typedef vector<int>::size_type size_type;
    for (size_type z = 0, n = log_.size(); z < n; ++z) {
        msg += lexical_cast<string>(log_[z]);
        if (z < n - 1)
            msg += ',';
    }
    return msg;
}

inline void operation_sequence::reset() 
{ 
    log_.clear();
    total_executed_ = 0; 
    last_executed_ = INT_MIN;
    success_ = false;
    failed_ = false;
}

inline void operation_sequence::execute(int id)
{
    log_.push_back(id);
    if (!failed_ && last_executed_ < id) {
        if (++total_executed_ == operations_.size())
            success_ = true;
        last_executed_ = id;
    } else {
        success_ = false;
        failed_ = true;
    }
}

inline void operation_sequence::remove_operation(int id)
{
    using namespace std;
    map_type::iterator it = operations_.find(id);
    if (it == operations_.end())
        throw runtime_error( string("No such operation: ") + 
                             lexical_cast<string>(id) );
    operations_.erase(it);
}

} } } // End namespace boost::iostreams::test.

#endif // #ifndef BOOST_IOSTREAMS_TEST_OPERATION_SEQUENCE_HPP_INCLUDED

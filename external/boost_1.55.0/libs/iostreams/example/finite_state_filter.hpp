// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// Adapted from an example of James Kanze, with suggestions from Peter Dimov.
// See http://www.gabi-soft.fr/codebase-en.html.

#ifndef BOOST_IOSTREAMS_FINITE_STATE_FILTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_FINITE_STATE_FILTER_HPP_INCLUDED

#include <cassert>
#include <cstdio>    // EOF.
#include <iostream>  // cin, cout.
#include <locale>
#include <string>
#include <boost/config.hpp>                 // JOIN, member template friends.
#include <boost/detail/workaround.hpp>
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/char_traits.hpp>
#include <boost/iostreams/checked_operations.hpp>   // put_if.
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/detail/ios.hpp>           // openmode.
#include <boost/iostreams/filter/stdio.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/preprocessor/control/expr_if.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>

namespace boost { namespace iostreams {

//------------------Definition of basic character classes---------------------//

struct finite_state_machine_base {

    static const int initial_state = 0;

        // All-inclusive character class.

    struct is_any {
        template<typename Ch>
        static bool test(Ch, const std::locale&) { return true; }
    };

        // Locale-sensitive character classes.

    #define BOOST_IOSTREAMS_CHARACTER_CLASS(class) \
        struct BOOST_JOIN(is_, class) { \
            template<typename Ch> \
            static bool test(Ch event, const std::locale& loc) \
            { return std::BOOST_JOIN(is, class)(event, loc); } \
        }; \
        /**/

    BOOST_IOSTREAMS_CHARACTER_CLASS(alnum)
    BOOST_IOSTREAMS_CHARACTER_CLASS(alpha)
    BOOST_IOSTREAMS_CHARACTER_CLASS(cntrl)
    BOOST_IOSTREAMS_CHARACTER_CLASS(digit)
    BOOST_IOSTREAMS_CHARACTER_CLASS(graph)
    BOOST_IOSTREAMS_CHARACTER_CLASS(lower)
    BOOST_IOSTREAMS_CHARACTER_CLASS(print)
    BOOST_IOSTREAMS_CHARACTER_CLASS(punct)
    BOOST_IOSTREAMS_CHARACTER_CLASS(space)
    BOOST_IOSTREAMS_CHARACTER_CLASS(upper)
    BOOST_IOSTREAMS_CHARACTER_CLASS(xdigit)

    #undef BOOST_IOSTREAMS_CHARACTER_CLASS
};

template<typename Ch>
struct finite_state_machine_base_ex : finite_state_machine_base {
    template<Ch C>
    struct is {
        static bool test(Ch event, const std::locale&)
        {
            return event == C;
        }
    };
};

//------------------Definition of base class for finite state filters---------//

namespace detail {

template<typename FiniteStateMachine>
class finite_state_filter_impl;

} // End namespace detail.

template<typename Derived, typename Ch = char>
class finite_state_machine : public finite_state_machine_base_ex<Ch> {
public:
    typedef Ch                                  char_type;
    typedef typename char_traits<Ch>::int_type  int_type;
    void imbue(const std::locale& loc) { loc_ = loc; }
    const std::locale& getloc() const { return loc_; }
protected:
    finite_state_machine() : off_(0) { }

    // Template whose instantiations make up transition table.

    template< int State,
              typename CharacterClass,
              int NextState,
              void (Derived::*Action)(char_type) >
    struct row {
        typedef CharacterClass  character_class;
        static const int        state = State;
        static const int        next_state = NextState;
        static void execute(Derived& d, char_type event)
        {
            (d.*Action)(event);
        }
    };

    // Stack interface.

    bool empty() const
    {
        return off_ == buf_.size();
    }
    void push(char c) { buf_ += c; }
    char_type pop()
    {
        char_type result = buf_[off_++];
        if (off_ == buf_.size())
            clear();
        return result;
    }
    char_type& top() { return buf_[off_]; }
    void clear()
    {
        buf_.clear();
        off_ = 0;
    }

    // Default event handlers.

    void on_eof() { }
    void skip(char_type) { }

#if BOOST_WORKAROUND(__MWERKS__, <= 0x3206)
    template<typename Ch>
    void _push_impl(Ch c) { push(c); }
#endif

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
    template<typename FiniteStateFilter>
    friend class detail::finite_state_filter_impl;
#else
    public:
#endif
    void on_any(char_type) { }
private:
    typedef std::basic_string<char_type>     string_type;
    typedef typename string_type::size_type  size_type;
    std::locale  loc_;
    string_type  buf_;
    size_type    off_;
};

#if !BOOST_WORKAROUND(__MWERKS__, <= 0x3206)
# define BOOST_IOSTREAMS_FSM(fsm) \
    template<typename Ch> \
    void push(Ch c) \
    { ::boost::iostreams::finite_state_machine<fsm, Ch>::push(c); } \
    template<typename Ch> \
    void skip(Ch c) { (void) c; } \
    /**/
#else // #ifndef __MWERKS__
# define BOOST_IOSTREAMS_FSM(fsm) \
    void push(char c) { this->_push_impl(c); } \
    void push(wchar_t c) { this->_push_impl(c); } \
    void skip(char c) { (void) c; } \
    void skip(wchar_t c) { (void) c; } \
    /**/
#endif

//------------------Definition of finite_state_filter_impl--------------------//

namespace detail {

template<typename FiniteStateMachine>
class finite_state_filter_impl : protected FiniteStateMachine
{
private:
    template<typename First, typename Last>
    struct process_event_impl;
public:
    typedef typename char_type_of<FiniteStateMachine>::type char_type;

    finite_state_filter_impl() : state_(FiniteStateMachine::initial_state) { }

    template<typename T0>
    explicit finite_state_filter_impl(const T0& t0)
        : FiniteStateMachine(t0), state_(FiniteStateMachine::initial_state)
        { }

    template<typename T0, typename T1>
    finite_state_filter_impl(const T0& t0, const T1& t1)
        : FiniteStateMachine(t0, t1), state_(FiniteStateMachine::initial_state)
        { }

    template<typename T0, typename T1, typename T2>
    finite_state_filter_impl(const T0& t0, const T1& t1, const T2& t2)
        : FiniteStateMachine(t0, t1, t2),
          state_(FiniteStateMachine::initial_state)
        { }
protected:
    void process_event(char_type c)
    {
        typedef typename FiniteStateMachine::transition_table  transitions;
        typedef typename mpl::begin<transitions>::type         first;
        typedef typename mpl::end<transitions>::type           last;
        state_ = process_event_impl<first, last>::execute(*this, state_, c);
    }
    int& state() { return state_; }
    void reset()
    {
        state_ = FiniteStateMachine::initial_state;
        this->clear();
    }
private:
    template<typename First, typename Last>
    struct process_event_impl {
        static int execute(FiniteStateMachine& fsm, int state, char_type event)
        {
            typedef typename mpl::deref<First>::type  rule;
            typedef typename mpl::next<First>::type   next;
            typedef typename rule::character_class    character_class;

            if ( state == rule::state &&
                 character_class::test(event, fsm.getloc()) )
            {
                // Rule applies.
                rule::execute(fsm, event);
                return rule::next_state;
            }

            // Rule is inapplicable: try next rule.
            return process_event_impl<next, Last>::execute(fsm, state, event);
        }
    };

    template<typename Last>
    struct process_event_impl<Last, Last> {
        static int execute(FiniteStateMachine& fsm, int state, char_type c)
        {
            on_any(fsm, c);
            return state;
        }
    };
#if BOOST_WORKAROUND(__DECCXX_VER, BOOST_TESTED_AT(60590042)) /* Tru64 */ \
 || BOOST_WORKAROUND(__MWERKS__,   BOOST_TESTED_AT(0x3205))   /* CW9.4 */
    public:
#endif
    template<typename FSM>
    static void on_any(FSM& fsm, char_type c) { fsm.on_any(c); }
private:
    int state_;
};

} // End namespace detail.

//------------------Definition of finite_state_filter-------------------------//

template<typename FiniteStateMachine>
class finite_state_filter
    : public detail::finite_state_filter_impl<FiniteStateMachine>
{
private:
    typedef detail::finite_state_filter_impl<FiniteStateMachine>  base_type;
public:
    typedef typename base_type::char_type                         char_type;
    typedef char_traits<char_type>                                traits_type;
    typedef typename base_type::int_type                          int_type;
    struct category
        : dual_use, filter_tag, closable_tag, localizable_tag
        { };

    finite_state_filter() : flags_(0) { }

    template<typename T0>
    finite_state_filter(const T0& t0)
        : base_type(t0), flags_(0)
        { }

    template<typename T0, typename T1>
    finite_state_filter(const T0& t0, const T1& t1)
        : base_type(t0, t1), flags_(0)
        { }

    template<typename T0, typename T1, typename T2>
    finite_state_filter(const T0& t0, const T1& t1, const T2& t2)
        : base_type(t0, t1, t2), flags_(0)
        { }

    template<typename Source>
    int_type get(Source& src)
    {
        assert((flags_ & f_write) == 0);
        flags_ |= f_read;

        while (true) {
            if ((flags_ & f_eof) == 0) {

                // Read a character and process it.
                int_type c;
                if (traits_type::is_eof(c = iostreams::get(src))) {
                    flags_ |= f_eof;
                    this->on_eof();
                } else if (!traits_type::would_block(c)) {
                    this->process_event(c);
                }
            }

            // Return a character, if available.
            if (!this->empty())
                return this->pop();
            else if ((flags_ & f_eof) != 0)
                return traits_type::eof();
        }
    }

    template<typename Sink>
    bool put(Sink& dest, char_type c)
    {
        assert((flags_ & f_read) == 0);
        flags_ |= f_write;

        this->process_event(c);
        while (!this->empty() && iostreams::put(dest, this->top()))
            this->pop();

        return true;
    }

    template<typename Device>
    void close(Device& dev, BOOST_IOS::openmode which)
    {
        if (which == BOOST_IOS::out) {
            if (flags_ & f_write)
                while (!this->empty())
                    iostreams::put_if(dev, this->pop());
            this->reset();
            flags_ = 0;
        }
    }
private:
    enum flags {
        f_read    = 1,
        f_write   = f_read << 1,
        f_eof     = f_write << 1
    };

    int flags_;
};

} }       // End namespaces iostreams, boost.

#endif // #ifndef BOOST_IOSTREAMS_FINITE_STATE_FILTER_HPP_INCLUDED


#include <boost/phoenix.hpp>

#include <iostream>
#include <sstream>

namespace proto = boost::proto;
namespace phoenix = boost::phoenix;

template <typename Rule>
struct identity_transform;

struct identity_actions
{
    template <typename Rule>
    struct when : phoenix::call<identity_transform<Rule> > {};
};

template <>
struct identity_actions::when<phoenix::rule::argument>
    : proto::call<
        identity_transform<phoenix::rule::argument>(proto::_value, phoenix::_context)
    > {};

template <>
struct identity_actions::when<phoenix::rule::terminal>
    : proto::call<
        identity_transform<phoenix::rule::terminal>(proto::_value, phoenix::_context)
    > {};

template <>
struct identity_actions::when<phoenix::rule::custom_terminal>
    : proto::lazy<
        identity_transform<proto::_value>(proto::_value, phoenix::_context)
    > {};

template <>
struct identity_transform<phoenix::rule::terminal>
{
    typedef std::string result_type;

    template <typename Terminal, typename Context>
    std::string operator()(Terminal const & terminal, Context) const
    {
        std::stringstream ss;
        ss << "val(" << terminal << ")";
        return ss.str();
    }

    template <typename Context>
    std::string operator()(char const * terminal, Context) const
    {
        std::stringstream ss;
        ss << "val(\"" << terminal << "\")";
        return ss.str();
    }
};

template <typename T>
struct identity_transform<boost::reference_wrapper<T> >
{
    typedef std::string result_type;

    template <typename Terminal, typename Context>
    std::string operator()(Terminal const & terminal, Context) const
    {
        std::stringstream ss;
        ss << "ref(" << terminal << ")";
        return ss.str();
    }


    template <int N, typename Context>
    std::string operator()(boost::reference_wrapper<char const *> terminal, Context) const
    {
        std::stringstream ss;
        ss << "ref(\"" << terminal << "\")";
        return ss.str();
    }
    
    template <int N, typename Context>
    std::string operator()(boost::reference_wrapper<char const [N]> terminal, Context) const
    {
        std::stringstream ss;
        ss << "ref(\"" << terminal << "\")";
        return ss.str();
    }
};

template <>
struct identity_transform<phoenix::rule::argument>
{
    typedef std::string result_type;

    template <typename N, typename Context>
    std::string operator()(N, Context) const
    {
        std::stringstream ss;
        ss << "_" << N::value;
        return ss.str();
    }
};


template <typename Expr>
void identity(Expr const & expr)
{
    std::cout << phoenix::eval(expr, phoenix::context(int(), identity_actions())) << "\n";
}

int main()
{

    identity(phoenix::val(8));
    identity(phoenix::val("8"));
    identity(phoenix::ref("blubb"));
    identity(phoenix::arg_names::_1);
}

#ifndef REDIS_REDIS_GRAMMAR_INTERNAL_HPP_
#define REDIS_REDIS_GRAMMAR_INTERNAL_HPP_

//These macros greatly reduce the boiler plate needed to add a new command.
//They are structured in such as way as to allow a new command to be included by specifying
//the type signature of the corresponding redis_interface_t method. This expands to a parser
//that will match that command name and parse arguments of the given types and which will then
//call the corresponding redis_interface_t method.

//Begin starts off a command block with a rule guaranteed to fail. This allows us to not have to
//special case the first actual command. All commands can then begin with '|'.
#define BEGIN(RULE) RULE = (!qi::eps)

//Alas! there is no macro overloading or a sufficiently robust variatic macro for our purposes here.
#define CMD_0(CNAME)\
        | command(1, std::string(#CNAME))\
        [px::bind(&redis_grammar::output_response, this, \
         px::bind(&redis_ext::CNAME, this \
         ))]

#define CMD_1(CNAME, ARG_TYPE_ONE)\
        | command(2, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg)\
        [px::bind(&redis_grammar::output_response, this, \
         px::bind(&redis_ext::CNAME, this, qi::_1 \
         ))]

#define CMD_2(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO)\
        | command(3, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg)\
        [px::bind(&redis_grammar::output_response, this, \
         px::bind(&redis_ext::CNAME, this, qi::_1, qi::_2 \
         ))]

#define CMD_3(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE)\
        | command(4, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg >> ARG_TYPE_THREE##_arg)\
        [px::bind(&redis_grammar::output_response, this, \
         px::bind(&redis_ext::CNAME, this, qi::_1, qi::_2, qi::_3 \
         ))]

#define CMD_4(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE, ARG_TYPE_FOUR)\
        | command(5, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg >> ARG_TYPE_THREE##_arg >> ARG_TYPE_FOUR##_arg)\
        [px::bind(&redis_grammar::output_response, this, \
         px::bind(&redis_ext::CNAME, this, qi::_1, qi::_2, qi::_3, qi::_4 \
         ))]

#define CMD_6(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE, ARG_TYPE_FOUR, ARG_TYPE_FIVE, ARG_TYPE_SIX)\
        | command(7, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg >> ARG_TYPE_THREE##_arg >> ARG_TYPE_FOUR##_arg >> ARG_TYPE_FIVE##_arg >> ARG_TYPE_SIX##_arg)\
        [px::bind(&redis_grammar::output_response, this, \
         px::bind(&redis_ext::CNAME, this, qi::_1, qi::_2, qi::_3, qi::_4, qi::_5, qi::_6 \
         ))]

#define CMD_7(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE, ARG_TYPE_FOUR, ARG_TYPE_FIVE, ARG_TYPE_SIX, ARG_TYPE_SEVEN)\
        | command(8, std::string(#CNAME)) >> (ARG_TYPE_ONE##_arg >> ARG_TYPE_TWO##_arg >> ARG_TYPE_THREE##_arg >> ARG_TYPE_FOUR##_arg >> ARG_TYPE_FIVE##_arg >> ARG_TYPE_SIX##_arg >> ARG_TYPE_SEVEN##_arg)\
        [px::bind(&redis_grammar::output_response, this, \
         px::bind(&redis_ext::CNAME, this, qi::_1, qi::_2, qi::_3, qi::_4, qi::_5, qi::_6, qi::_7 \
         ))]


//Some commands take an unsecified number of arguments. These are parsed into a vector of strings. (sorry no other types)
#define CMD_N(CNAME)\
        | command_n(std::string(#CNAME))\
        [px::bind(&redis_grammar::output_response, this, \
         px::bind(&redis_ext::CNAME, this, qi::_1 \
         ))]

#define COMMANDS_END ;

#endif  // REDIS_REDIS_GRAMMAR_INTERNAL_HPP_

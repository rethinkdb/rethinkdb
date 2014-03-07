
// Copyright 2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*!
    \class example::example
    
    \brief Documentation for class example
 */

/*!
    \def EXAMPLE
    
    \brief Documentation for macro example
 */

int global_integer;
static int global_static_integer;
const int global_const_integer = 1;
static const int global_static_const_integer = 2;
enum global_enum { enumerator1 = 1, enumerator2 };

namespace example
{
    int namespace_integer;
    static int namespace_static_integer;
    const int namespace_const_integer = 1;
    static const int namespace_static_const_integer = 2;
    enum namespace_enum { enumerator };

    class example
    {
    public:
        example(example const&) = default;
        example& operator=(example const&) = delete;

        int integer;
        static int static_integer;
        mutable int mutable_integer;
        const int const_integer;
        static const int static_const_integer;

        // Visual check of typedef alignment.
        /** This type has documentation. */
        typedef int documented_type1;
        /** \brief This type has documentation. */
        typedef long documented_type2;
        /** This type has documentation. */
        typedef long double documented_type3;
        typedef short undocumented_type1;
        typedef double undocumented_type2;
        
        class inner_class {
        public:
            int x;
        };

        enum class_enum { enumerator };
        
        /// INTERNAL ONLY
        enum internal_enum { internal_enumerator };

        explicit operator int();
    protected:
        int protected_integer;
        static int protected_static_integer;
        mutable int protected_mutable_integer;
        const int protected_const_integer;
        static const int protected_static_const_integer;

        enum protected_class_enum { enumerator2 };
    private:
        int private_integer;
        static int private_static_integer;
        mutable int private_mutable_integer;
        const int private_const_integer;
        static const int private_static_const_integer;

        enum private_class_enum { enumerator3 };
    };
    
    /**
     * Test some doxygen markup
     *
     * \warning This is just an example.
     *
     * Embedded docbook list:
     *
     * \xmlonly
     * <orderedlist><listitem><simpara>1</simpara></listitem><listitem><simpara>2</simpara></listitem></orderedlist>
     * \endxmlonly
     *
     * \a Special \b Bold \c Typewriter \e Italics \em emphasis \p parameter
     *
     * \arg Arg1 first argument.
     * \arg Arg2 second argument.
     *
     * \li First list item.
     * \li Second list item
     *
     * Line 1\n
     * Line 2
     *
     * \code
     *     void foo() {}
     * \endcode
     *
     * \tparam TypeParameter A template parameter
     * \tparam NonTypeParameter This is a non-type template parameter
     * \tparam TypeParameterWithDefault This is a template parameter with a default argument
     */

    template <typename TypeParameter, int NonTypeParameter,
        typename TypeParameterWithDefault = int>
    struct example_template {};

    /**
     * \param i A function parameter
     * \param j Another
     * \return The answer
     * \pre i > j
     *
     * This is a test function.
     * \ref example::example "Link to class"
     * \ref example_template "Link to class template"
     * \note This is a note.
     *
     * \see example::example and example_template
     */
    int namespace_func(int i, int j);
    
    /**
     * Testing a function template.
     * \tparam TypeParameter A template parameter
     * \tparam NonTypeParameter This is a non-type template parameter
     */
    template <typename TypeParameter, int NonTypeParameter>
    void namespace_func_template();

    template<class T>
    struct specialization_test {
    };

    template<class T>
    struct specialization_test<T*> {
        /** A constructor. */
        specialization_test();
        /** A destructor. */
        ~specialization_test();
        /** An assignment operator. */
        specialization_test& operator=(const specialization_test&);
    };
}

#define EXAMPLE(m) The macro

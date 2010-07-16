#ifndef __STATS_HPP__
#define __STATS_HPP__

#include <vector>
#include <string>

/* Variable Types */
struct BaseType {
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > custom_stringstream;

    custom_string description;    
    virtual custom_string getValue();

};

struct IntType : public BaseType, public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, IntType> {
        IntType(int* a) : value(a) {}
        int* value;
        int min;
        int max;
        custom_string getValue();
};


struct DoubleType : public BaseType, public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, DoubleType> {
        DoubleType(double* a) : value(a) {}
        double* value;
        double min;
        double max;
        custom_string getValue();
};


struct FloatType : public BaseType, public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, FloatType> {
        FloatType(float* a) : value(a) {}
        float* value;
        float min;
        float max;
        custom_string getValue();
};

/* The actual Stats Module */
struct Stats {
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > custom_string;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > custom_stringstream;

    Stats() {}
    ~Stats();
    custom_string get();
    void add(int *val, const char* desc);
    void add(double *val, const char* desc);
    void add(float *val, const char* desc);
    std::vector<BaseType *, gnew_alloc<BaseType*> > registry;
};

/* Example:

        Stats g;
        int a = 100;
        g.add(&a, "my variable");
        a = 200;
        cout << g.get() << endl;
*/


#endif
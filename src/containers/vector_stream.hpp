#ifndef CONTAINERS_VECTOR_STREAM_HPP_
#define CONTAINERS_VECTOR_STREAM_HPP_

#include <streambuf>
#include <vector>
#include <ios>

template< typename C = char, typename T = std::char_traits<C>, typename A = std::allocator<C> >
class vector_streambuf_t : public std::basic_streambuf<C, T> {
public:
    typedef C char_type;
    typedef T traits_type;
    typedef A allocator_type;

    typedef typename T::int_type int_type;

    vector_streambuf_t();
    explicit vector_streambuf_t(std::vector<C, A> &underlying_);

    std::streamsize showmanyc();

    std::streamsize xsgetn(char_type* s, std::streamsize n);
    std::streamsize xsputn(const char_type* s, std::streamsize n);

    int_type overflow(int_type c);

    std::vector<C, A>& vector() { return underlying; }

private:
    void init();
    void update_pointers(std::streamsize putback_size);
    void reserve(size_t n);

    std::vector<C, A> inner;   // this array is only used when the parameterless constructor is used
    std::vector<C, A> &underlying;
};

#include "containers/vector_stream.tcc"

#endif  // CONTAINERS_VECTOR_STREAM_HPP_


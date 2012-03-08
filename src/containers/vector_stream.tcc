#include <iterator>

template<typename C, typename T, typename A>
vector_streambuf_t<C, T, A>::vector_streambuf_t() : inner(), underlying(inner) {
    init();
}

template<typename C, typename T, typename A>
vector_streambuf_t<C, T, A>::vector_streambuf_t(std::vector<C,A> &underlying_) : underlying(underlying_) {
    init();
}

template<typename C, typename T, typename A>
std::streamsize vector_streambuf_t<C, T, A>::showmanyc() {
    return underlying.size();
}

template<typename C, typename T, typename A>
std::streamsize vector_streambuf_t<C, T, A>::xsgetn(char_type* s, std::streamsize n) {
    n = std::min(n, static_cast<std::streamsize>(underlying.size()));
    std::copy(this->gptr(), this->gptr() + n, s);
    this->gbump(n);
    return n;
}

template<typename C, typename T, typename A>
std::streamsize vector_streambuf_t<C, T, A>::xsputn(const char_type* s, std::streamsize n) {
    std::streamsize putback_size = this->gptr() - this->eback();

    reserve(underlying.size() + n);
    std::copy(s, s+n, std::back_inserter(underlying));

    update_pointers(putback_size);
    return n;
}

template<typename C, typename T, typename A>
typename vector_streambuf_t<C, T, A>::int_type vector_streambuf_t<C,T,A>::overflow(int_type c) {
    std::streamsize putback_size = this->gptr() - this->eback();

    underlying.push_back(c);

    update_pointers(putback_size);
    return 0;
}

template<typename C, typename T, typename A>
void vector_streambuf_t<C, T, A>::init() {
    reserve(1);

    this->setg(underlying.data(), underlying.data(), underlying.data()+underlying.size());
    this->setp(underlying.data(), underlying.data()+underlying.size());
    this->pbump(underlying.size());
}

template<typename C, typename T, typename A>
void vector_streambuf_t<C, T, A>::update_pointers(std::streamsize putback_size) {
    this->setg(underlying.data(), underlying.data() + putback_size, underlying.data() + underlying.size());
    this->setp(underlying.data(), underlying.data() + underlying.size());
    this->pbump(underlying.size());
}

template<typename C, typename T, typename A>
void vector_streambuf_t<C, T, A>::reserve(size_t n) {
    if (n > underlying.capacity()) {
        const size_t max_reserve = 0x100000;
        size_t extra = n - underlying.capacity();
        size_t to_reserve = underlying.capacity() + std::max(extra, std::min(underlying.capacity(), max_reserve));
        underlying.reserve(to_reserve);
    }
}


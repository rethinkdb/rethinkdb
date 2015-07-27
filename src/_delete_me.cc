

template <class T>
bool foo(T *out) {
  int called;
  bar([&](int arg){
      other(arg, out);
      called = true;
    });
  return called;
}

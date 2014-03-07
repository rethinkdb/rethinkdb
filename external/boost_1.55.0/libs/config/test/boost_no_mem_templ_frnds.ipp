//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_MEMBER_TEMPLATE_FRIENDS
//  TITLE:         member template friends
//  DESCRIPTION:   Member template friend syntax
//                 ("template<class P> friend class frd;")
//                 described in the C++ Standard,
//                 14.5.3, not supported.


namespace boost_no_member_template_friends{

template <class T>
class foobar;

template <class T>
class foo;

template <class T>
bool must_be_friend_proc(const foo<T>& f);

template <class T>
class foo
{
private:
   template<typename Y> friend class foobar;
   template<typename Y> friend class foo;
   template<typename Y> friend bool must_be_friend_proc(const foo<Y>& f);
   int i;
public:
   foo(){ i = 0; }
   template <class U>
   foo(const foo<U>& f){ i = f.i; }
};

template <class T>
bool must_be_friend_proc(const foo<T>& f)
{ return f.i != 0; }

template <class T>
class foobar
{
   int i;
public:
   template <class U>
   foobar(const foo<U>& f)
   { i = f.i; }
};


int test()
{
   foo<int> fi;
   foo<double> fd(fi);
   must_be_friend_proc(fd);
   foobar<long> fb(fi);
   (void) &fb;           // avoid "unused variable" warning
   return 0;
}

}







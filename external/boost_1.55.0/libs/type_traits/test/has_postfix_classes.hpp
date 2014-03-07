//  (C) Copyright Frederic Bron 2009-2011.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef TT_HAS_POSTFIX_CLASSES_HPP
#define TT_HAS_POSTFIX_CLASSES_HPP

struct ret { };
ret ret_val;

class C000 { C000(); public: C000(int) { } };
void operator++(C000, int) { }

class C001 { C001(); public: C001(int) { } };
ret  operator++(C001, int) { return ret_val; }

class C002 { C002(); public: C002(int) { } };
ret  const operator++(C002, int) { return ret_val; }

class C005 { C005(); public: C005(int) { } };
ret  & operator++(C005, int) { return ret_val; }

class C006 { C006(); public: C006(int) { } };
ret  const & operator++(C006, int) { return ret_val; }

class C009 { C009(); public: C009(int) { } };
void operator++(C009 const, int) { }

class C010 { C010(); public: C010(int) { } };
ret  operator++(C010 const, int) { return ret_val; }

class C011 { C011(); public: C011(int) { } };
ret  const operator++(C011 const, int) { return ret_val; }

class C014 { C014(); public: C014(int) { } };
ret  & operator++(C014 const, int) { return ret_val; }

class C015 { C015(); public: C015(int) { } };
ret  const & operator++(C015 const, int) { return ret_val; }

class C036 { C036(); public: C036(int) { } };
void operator++(C036 &, int) { }

class C037 { C037(); public: C037(int) { } };
ret  operator++(C037 &, int) { return ret_val; }

class C038 { C038(); public: C038(int) { } };
ret  const operator++(C038 &, int) { return ret_val; }

class C041 { C041(); public: C041(int) { } };
ret  & operator++(C041 &, int) { return ret_val; }

class C042 { C042(); public: C042(int) { } };
ret  const & operator++(C042 &, int) { return ret_val; }

class C045 { C045(); public: C045(int) { } };
void operator++(C045 const &, int) { }

class C046 { C046(); public: C046(int) { } };
ret  operator++(C046 const &, int) { return ret_val; }

class C047 { C047(); public: C047(int) { } };
ret  const operator++(C047 const &, int) { return ret_val; }

class C050 { C050(); public: C050(int) { } };
ret  & operator++(C050 const &, int) { return ret_val; }

class C051 { C051(); public: C051(int) { } };
ret  const & operator++(C051 const &, int) { return ret_val; }

#endif

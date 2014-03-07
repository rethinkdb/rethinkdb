//  (C) Copyright Frederic Bron 2009-2011.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef TT_HAS_BINARY_CLASSES_HPP
#define TT_HAS_BINARY_CLASSES_HPP

struct ret { };
ret ret_val;

class C000 { C000(); public: C000(int) { } };
void operator+(C000, C000) { }

class C001 { C001(); public: C001(int) { } };
ret  operator+(C001, C001) { return ret_val; }

class C002 { C002(); public: C002(int) { } };
ret  const operator+(C002, C002) { return ret_val; }

class C005 { C005(); public: C005(int) { } };
ret  & operator+(C005, C005) { return ret_val; }

class C006 { C006(); public: C006(int) { } };
ret  const & operator+(C006, C006) { return ret_val; }

class C009 { C009(); public: C009(int) { } };
void operator+(C009, C009 const) { }

class C010 { C010(); public: C010(int) { } };
ret  operator+(C010, C010 const) { return ret_val; }

class C011 { C011(); public: C011(int) { } };
ret  const operator+(C011, C011 const) { return ret_val; }

class C014 { C014(); public: C014(int) { } };
ret  & operator+(C014, C014 const) { return ret_val; }

class C015 { C015(); public: C015(int) { } };
ret  const & operator+(C015, C015 const) { return ret_val; }

class C036 { C036(); public: C036(int) { } };
void operator+(C036, C036 &) { }

class C037 { C037(); public: C037(int) { } };
ret  operator+(C037, C037 &) { return ret_val; }

class C038 { C038(); public: C038(int) { } };
ret  const operator+(C038, C038 &) { return ret_val; }

class C041 { C041(); public: C041(int) { } };
ret  & operator+(C041, C041 &) { return ret_val; }

class C042 { C042(); public: C042(int) { } };
ret  const & operator+(C042, C042 &) { return ret_val; }

class C045 { C045(); public: C045(int) { } };
void operator+(C045, C045 const &) { }

class C046 { C046(); public: C046(int) { } };
ret  operator+(C046, C046 const &) { return ret_val; }

class C047 { C047(); public: C047(int) { } };
ret  const operator+(C047, C047 const &) { return ret_val; }

class C050 { C050(); public: C050(int) { } };
ret  & operator+(C050, C050 const &) { return ret_val; }

class C051 { C051(); public: C051(int) { } };
ret  const & operator+(C051, C051 const &) { return ret_val; }

class C072 { C072(); public: C072(int) { } };
void operator+(C072 const, C072) { }

class C073 { C073(); public: C073(int) { } };
ret  operator+(C073 const, C073) { return ret_val; }

class C074 { C074(); public: C074(int) { } };
ret  const operator+(C074 const, C074) { return ret_val; }

class C077 { C077(); public: C077(int) { } };
ret  & operator+(C077 const, C077) { return ret_val; }

class C078 { C078(); public: C078(int) { } };
ret  const & operator+(C078 const, C078) { return ret_val; }

class C081 { C081(); public: C081(int) { } };
void operator+(C081 const, C081 const) { }

class C082 { C082(); public: C082(int) { } };
ret  operator+(C082 const, C082 const) { return ret_val; }

class C083 { C083(); public: C083(int) { } };
ret  const operator+(C083 const, C083 const) { return ret_val; }

class C086 { C086(); public: C086(int) { } };
ret  & operator+(C086 const, C086 const) { return ret_val; }

class C087 { C087(); public: C087(int) { } };
ret  const & operator+(C087 const, C087 const) { return ret_val; }

class C108 { C108(); public: C108(int) { } };
void operator+(C108 const, C108 &) { }

class C109 { C109(); public: C109(int) { } };
ret  operator+(C109 const, C109 &) { return ret_val; }

class C110 { C110(); public: C110(int) { } };
ret  const operator+(C110 const, C110 &) { return ret_val; }

class C113 { C113(); public: C113(int) { } };
ret  & operator+(C113 const, C113 &) { return ret_val; }

class C114 { C114(); public: C114(int) { } };
ret  const & operator+(C114 const, C114 &) { return ret_val; }

class C117 { C117(); public: C117(int) { } };
void operator+(C117 const, C117 const &) { }

class C118 { C118(); public: C118(int) { } };
ret  operator+(C118 const, C118 const &) { return ret_val; }

class C119 { C119(); public: C119(int) { } };
ret  const operator+(C119 const, C119 const &) { return ret_val; }

class C122 { C122(); public: C122(int) { } };
ret  & operator+(C122 const, C122 const &) { return ret_val; }

class C123 { C123(); public: C123(int) { } };
ret  const & operator+(C123 const, C123 const &) { return ret_val; }

class C288 { C288(); public: C288(int) { } };
void operator+(C288 &, C288) { }

class C289 { C289(); public: C289(int) { } };
ret  operator+(C289 &, C289) { return ret_val; }

class C290 { C290(); public: C290(int) { } };
ret  const operator+(C290 &, C290) { return ret_val; }

class C293 { C293(); public: C293(int) { } };
ret  & operator+(C293 &, C293) { return ret_val; }

class C294 { C294(); public: C294(int) { } };
ret  const & operator+(C294 &, C294) { return ret_val; }

class C297 { C297(); public: C297(int) { } };
void operator+(C297 &, C297 const) { }

class C298 { C298(); public: C298(int) { } };
ret  operator+(C298 &, C298 const) { return ret_val; }

class C299 { C299(); public: C299(int) { } };
ret  const operator+(C299 &, C299 const) { return ret_val; }

class C302 { C302(); public: C302(int) { } };
ret  & operator+(C302 &, C302 const) { return ret_val; }

class C303 { C303(); public: C303(int) { } };
ret  const & operator+(C303 &, C303 const) { return ret_val; }

class C324 { C324(); public: C324(int) { } };
void operator+(C324 &, C324 &) { }

class C325 { C325(); public: C325(int) { } };
ret  operator+(C325 &, C325 &) { return ret_val; }

class C326 { C326(); public: C326(int) { } };
ret  const operator+(C326 &, C326 &) { return ret_val; }

class C329 { C329(); public: C329(int) { } };
ret  & operator+(C329 &, C329 &) { return ret_val; }

class C330 { C330(); public: C330(int) { } };
ret  const & operator+(C330 &, C330 &) { return ret_val; }

class C333 { C333(); public: C333(int) { } };
void operator+(C333 &, C333 const &) { }

class C334 { C334(); public: C334(int) { } };
ret  operator+(C334 &, C334 const &) { return ret_val; }

class C335 { C335(); public: C335(int) { } };
ret  const operator+(C335 &, C335 const &) { return ret_val; }

class C338 { C338(); public: C338(int) { } };
ret  & operator+(C338 &, C338 const &) { return ret_val; }

class C339 { C339(); public: C339(int) { } };
ret  const & operator+(C339 &, C339 const &) { return ret_val; }

class C360 { C360(); public: C360(int) { } };
void operator+(C360 const &, C360) { }

class C361 { C361(); public: C361(int) { } };
ret  operator+(C361 const &, C361) { return ret_val; }

class C362 { C362(); public: C362(int) { } };
ret  const operator+(C362 const &, C362) { return ret_val; }

class C365 { C365(); public: C365(int) { } };
ret  & operator+(C365 const &, C365) { return ret_val; }

class C366 { C366(); public: C366(int) { } };
ret  const & operator+(C366 const &, C366) { return ret_val; }

class C369 { C369(); public: C369(int) { } };
void operator+(C369 const &, C369 const) { }

class C370 { C370(); public: C370(int) { } };
ret  operator+(C370 const &, C370 const) { return ret_val; }

class C371 { C371(); public: C371(int) { } };
ret  const operator+(C371 const &, C371 const) { return ret_val; }

class C374 { C374(); public: C374(int) { } };
ret  & operator+(C374 const &, C374 const) { return ret_val; }

class C375 { C375(); public: C375(int) { } };
ret  const & operator+(C375 const &, C375 const) { return ret_val; }

class C396 { C396(); public: C396(int) { } };
void operator+(C396 const &, C396 &) { }

class C397 { C397(); public: C397(int) { } };
ret  operator+(C397 const &, C397 &) { return ret_val; }

class C398 { C398(); public: C398(int) { } };
ret  const operator+(C398 const &, C398 &) { return ret_val; }

class C401 { C401(); public: C401(int) { } };
ret  & operator+(C401 const &, C401 &) { return ret_val; }

class C402 { C402(); public: C402(int) { } };
ret  const & operator+(C402 const &, C402 &) { return ret_val; }

class C405 { C405(); public: C405(int) { } };
void operator+(C405 const &, C405 const &) { }

class C406 { C406(); public: C406(int) { } };
ret  operator+(C406 const &, C406 const &) { return ret_val; }

class C407 { C407(); public: C407(int) { } };
ret  const operator+(C407 const &, C407 const &) { return ret_val; }

class C410 { C410(); public: C410(int) { } };
ret  & operator+(C410 const &, C410 const &) { return ret_val; }

class C411 { C411(); public: C411(int) { } };
ret  const & operator+(C411 const &, C411 const &) { return ret_val; }

#endif

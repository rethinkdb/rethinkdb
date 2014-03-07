// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python.hpp>
#include <iostream>
using namespace boost::python;
using namespace boost;

struct Product {};
typedef shared_ptr<Product> ProductPtr;


struct Creator
{
   virtual ~Creator() {}
   virtual ProductPtr create() = 0;
};


struct Factory
{
   void reg(Creator* c) { mC = c; }
   ProductPtr create()
   {
      std::cout << "Name: " << (typeid(*mC)).name() << std::endl;
      return mC->create();
   }

private:
   Creator* mC;
};

struct CreatorWrap : public Creator
{
   CreatorWrap(PyObject* self) : mSelf(self) {}
   ProductPtr create() { return call_method<ProductPtr>(mSelf, "create"); }
   PyObject* mSelf;
};

BOOST_PYTHON_MODULE(ben_scott1_ext)
{
   class_<Product, ProductPtr>("Product");

   class_<Creator, CreatorWrap, noncopyable>("Creator")
       .def("create", &CreatorWrap::create)
       ;

   class_<Factory>("Factory")
      .def("reg", &Factory::reg, with_custodian_and_ward<1,2>())
      .def("create", &Factory::create)
       ;
}

#include "../test/module_tail.cpp"

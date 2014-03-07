// Copyright Stefan Seefeld 2007.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python.hpp>

#include <boost/detail/lightweight_test.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <sstream>

namespace bpl = boost::python;

void import_test(char const *py_file_path)
{
  // Retrieve the main module
  bpl::object main = bpl::import("__main__");
  
  // Retrieve the main module's namespace
  bpl::object global(main.attr("__dict__"));

  // Inject search path for import_ module

  bpl::str script(
      "import sys, os.path\n"
      "path = os.path.dirname(%r)\n" 
      "sys.path.insert(0, path)"
      % bpl::str(py_file_path));
  
  bpl::object result = bpl::exec(script, global, global);
  
  // Retrieve the main module
  bpl::object import_ = bpl::import("import_");
  int value = bpl::extract<int>(import_.attr("value")) BOOST_EXTRACT_WORKAROUND;
  std::cout << value << std::endl;
  BOOST_TEST(value == 42);
}

int main(int argc, char **argv)
{
  BOOST_TEST(argc == 2);

  // Initialize the interpreter
  Py_Initialize();
  
  if (bpl::handle_exception(boost::bind(import_test,argv[1])))
  {
    if (PyErr_Occurred())
    {
      BOOST_ERROR("Python Error detected");
      PyErr_Print();
    }
    else
    {
        BOOST_ERROR("A C++ exception was thrown  for which "
                    "there was no exception handler registered.");
    }
  }
    
  // Boost.Python doesn't support Py_Finalize yet.
  // Py_Finalize();
  return boost::report_errors();
}

// Including this file makes sure
// that on Windows, any crashes (e.g. null pointer dereferences) invoke
// the debugger immediately, rather than being translated into structured
// exceptions that can interfere with debugging.
#include "module_tail.cpp"

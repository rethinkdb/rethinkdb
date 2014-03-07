//  (C) Copyright Niels Dekker 2010. 
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_COMPLETE_VALUE_INITIALIZATION
//  TITLE:         No complete value-initialization
//  DESCRIPTION:   The C++ compiler does not to have implemented value-initialization completely.
//                 See also boost/libs/utility/value_init.htm#compiler_issues

#include <iostream>

//  This test checks various forms of value-initialization:
//  - doing subobject initialization inside a constructor
//  - creating a temporary object by T()
//  - creating a heap object by doing new T()
//  It checks various DefaultConstructible types, including fundamental types,
//  enum, union, pointer types, array types, POD and non-POD class types. For
//  each type of object, a helper function is_value_initialized(const T&) tells 
//  whether the object is value-initialized.
//
//  Note: It appeared insufficient to just check a single POD and a single 
//  non-POD class type, because some compilers correctly value-initialize some
//  POD and some non-POD objects, while failing to value-initialize others.
//
//  The test returns the number of encountered value-initialization failures.
  
namespace boost_no_complete_value_initialization
{
  enum enum_type { negative_number = -1, magic_number = 42 };
  
  class incomplete_class;
  
  typedef int (*function_ptr_type)(int);
  typedef int (incomplete_class::*member_function_ptr_type)(int);

  // A POD struct.
  struct pod_struct
  {
    enum_type e;
    bool b;
    char c;
    unsigned char uc;
    short s;
    int i;
    unsigned u;
    long l;
    float f;
    double d;
    long double ld;
    void* p;
  };

  bool is_value_initialized(const pod_struct& arg)
  {
    return
      arg.b == 0 &&
      arg.e == 0 &&
      arg.c == 0 &&
      arg.uc == 0 &&
      arg.s == 0 &&
      arg.i == 0 &&
      arg.u == 0 &&
      arg.l == 0 &&
      arg.f == 0 &&
      arg.d == 0 &&
      arg.p == 0;
  }

  // A POD struct derived from another POD struct.
  struct derived_pod_struct: pod_struct
  {
    int derived_data;
  };

  bool is_value_initialized(const derived_pod_struct& arg)
  {
    const pod_struct& base_subobject = arg; 
    return arg.derived_data == 0 && is_value_initialized(base_subobject);
  }


  struct empty_struct
  {
  };


  // A POD aggregate struct derived from an empty struct.
  // Similar to struct Foo1 from Microsoft Visual C++ bug report 484295,
  // "VC++ does not value-initialize members of derived classes without 
  // user-declared constructor", reported in 2009 by Sylvester Hesp:
  // https://connect.microsoft.com/VisualStudio/feedback/details/484295
  struct derived_struct: empty_struct
  {
    int data;
  };

  bool is_value_initialized(const derived_struct& arg)
  {
    return arg.data == 0;
  }


  // A struct, having a bit-field.
  struct bit_field_struct
  {
    bool b : 1;
    char c : 7;
    unsigned u: 8 * sizeof(unsigned) - 1;  
  };
  
  bool is_value_initialized(const bit_field_struct& arg)
  {
    return arg.b == false && arg.c == '\0'&& arg.u == 0U;
  }
  
  // A struct, having a function pointer.
  struct function_ptr_struct
  {
    function_ptr_type data;
  };

  bool is_value_initialized(const function_ptr_struct& arg)
  {
    return arg.data == 0;
  }

  // A struct, having a member function pointer.
  struct member_function_ptr_struct
  {
    member_function_ptr_type data;
  };

  bool is_value_initialized(const member_function_ptr_struct& arg)
  {
    return arg.data == 0;
  }

  struct int_pair_struct
  {
    int first;
    int second;
  };

  typedef int int_pair_struct::*ptr_to_member_type;

  struct ptr_to_member_struct
  {
    ptr_to_member_type data;
  };

  bool is_value_initialized(const ptr_to_member_struct& arg)
  {
    return arg.data == 0;
  }

  // A struct, having an int. Equivalent to the struct TData, from CodeGear bug
  // report 51854, "Value-initialization: POD struct should be zero-initialized",
  // reported by me (Niels Dekker, LKEB) in 2007:
  // http://qc.embarcadero.com/wc/qcmain.aspx?d=51854
  struct int_struct
  {
    int data;
  };

  bool is_value_initialized(const int_struct& arg)
  {
    return arg.data == 0;
  }


  // A struct, having an int_struct.
  struct int_struct_holder
  {
    int_struct data;
  };

  bool is_value_initialized(const int_struct_holder& arg)
  {
    return is_value_initialized(arg.data);
  }


  // A struct derived from int_struct.
  struct derived_int_struct: int_struct
  {
  };

  bool is_value_initialized(const derived_int_struct& arg)
  {
    return arg.data == 0;
  }


  struct char_array_struct
  {
    char data[42];
  };

  bool is_value_initialized(const char_array_struct& arg)
  {
    for ( unsigned i = 0; i < sizeof(arg.data); ++i)
    {
      if ( arg.data[i] != 0 )
      {
        return false;
      }
    }
    return true;
  }


  class private_int_holder
  {
  private:
    int m_data;

    friend bool is_value_initialized(const private_int_holder& arg)
    {
      return arg.m_data == 0;
    }
  };


  // Equivalent to the Stats class from GCC Bug 33916,
  // "Default constructor fails to initialize array members", reported in 2007 by
  // Michael Elizabeth Chastain: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=33916
  class private_int_array_pair
  {
    friend bool is_value_initialized(const private_int_array_pair& arg);
  private:
    int first[12];
    int second[12];
  };

  bool is_value_initialized(const private_int_array_pair& arg)
  {
    for ( unsigned i = 0; i < 12; ++i)
    {
      if ( (arg.first[i] != 0) || (arg.second[i] != 0) )
      {
        return false;
      }
    }
    return true;
  }


  union pod_struct_and_int_union
  {
    pod_struct first;
    int second;
  };

  bool is_value_initialized(const pod_struct_and_int_union& arg)
  {
    // When a union is zero-initialized, its first non-static 
    // named data member is zero-initialized ([dcl.init]).
    return is_value_initialized(arg.first);
  }


  union int_and_pod_struct_union
  {
    int first;
    pod_struct second;
  };

  bool is_value_initialized(const int_and_pod_struct_union& arg)
  {
    return arg.first == 0;
  }


  // A class that holds a "magic" enum value.
  // Note: This is not a POD class, because it has a user-defined
  // default constructor.
  class enum_holder
  {
    enum_type m_enum;
  public:

    enum_holder()
    :
    m_enum(magic_number)
    {
    }

    bool is_value_initialized() const
    {
      return m_enum == magic_number;
    }
  };

  bool is_value_initialized(const enum_holder& arg)
  {
    return arg.is_value_initialized();
  }


  // An aggregate struct of a non-POD class and an int.
  // Similar to struct A from Microsoft Visual C++ bug report 100744,
  // "Value-initialization in new-expression", reported in 2005 by
  // Pavel Kuznetsov (MetaCommunications Engineering):
  // https://connect.microsoft.com/VisualStudio/feedback/details/100744
  struct enum_holder_and_int
  {
    enum_holder e;
    int i;
  };

  bool is_value_initialized(const enum_holder_and_int& arg)
  {
    return arg.e.is_value_initialized() && arg.i == 0;
  }

  class user_defined_copy_constructor_holder
  {
  public:
    int data;

    user_defined_copy_constructor_holder()
    :
    data(0)
    {
    }

    user_defined_copy_constructor_holder(const user_defined_copy_constructor_holder& arg)
    :
    data(arg.data)
    {
    }
  };

  // An aggregate struct that has a data member which has a user-defined
  // copy constructor and a data member of a scalar type.
  // Similar to struct B from Microsoft Visual C++ bug report 499606,
  // "Presence of copy constructor breaks member class initialization", 
  // reported in 2009 by Alex Vakulenko:
  // https://connect.microsoft.com/VisualStudio/feedback/details/499606
  struct user_defined_copy_constructor_holder_and_int
  {
    user_defined_copy_constructor_holder first;
    int second;
  };

  bool is_value_initialized(const user_defined_copy_constructor_holder_and_int& arg)
  {
    return arg.first.data == 0 && arg.second == 0;
  }


  // An class that has a private and a protected int data member.
  class private_and_protected_int
  {
  private:
    int private_int;
  protected:
    int protected_int;
  public:
    friend bool is_value_initialized(const private_and_protected_int& arg)
    {
      return arg.private_int == 0 && arg.protected_int == 0;
    }
  };


  class user_defined_destructor_holder
  {
  public:
    int i;
    ~user_defined_destructor_holder()
    {
    }
  };

  bool is_value_initialized(const user_defined_destructor_holder& arg)
  {
    return arg.i == 0;
  }


  class virtual_destructor_holder
  {
  public:
    int i;
    virtual ~virtual_destructor_holder()
    {
    }
  };

  bool is_value_initialized(const virtual_destructor_holder& arg)
  {
    return arg.i == 0;
  }


  // A class that is not a POD type.
  class non_pod_class
  {
  private:
    enum_holder m_enum_holder;

  public:
    int i;

    virtual bool is_value_initialized() const
    {
      return m_enum_holder.is_value_initialized() && i == 0;
    }

    virtual ~non_pod_class() {}
  };

  bool is_value_initialized(const non_pod_class& arg)
  {
    return arg.is_value_initialized();
  }


  typedef char _2d_char_array_type[3][4];

  bool is_value_initialized(const _2d_char_array_type& arg)
  {
    for(unsigned i = 0; i < sizeof(_2d_char_array_type); ++i)
    {
      if ((*arg)[i] != 0)
      {
        return false;
      }
    }
    return true;
  }

  typedef char _3d_char_array_type[5][6][7];

  bool is_value_initialized(const _3d_char_array_type& arg)
  {
    for(unsigned i = 0; i < sizeof(_3d_char_array_type); ++i)
    {
      if ((**arg)[i] != 0)
      {
        return false;
      }
    }
    return true;
  }



  // Tells whether an object of a scalar type T is value-initialized.
  template <class T>
  bool is_value_initialized(const T& arg)
  {
    return arg == 0;
  }


  // Wraps a heap object that it has allocated by doing new T().
  template <class T>
  class heap_object_wrapper
  {
  private:
    T* const m_ptr;

    // The following function is intentionally left unimplemented
    // (as if deleted, "= delete", in C++0x):
    void operator=(heap_object_wrapper);

  public:
    heap_object_wrapper()
      :
    m_ptr(new T())
    {
    }

    ~heap_object_wrapper()
    {
      delete m_ptr;
    }

    // The copy-constructor is intentionally left unimplemented.
    heap_object_wrapper(const heap_object_wrapper&);

    bool is_wrapped_object_value_initialized() const
    {
      return (m_ptr != 0) && is_value_initialized(*m_ptr);
    }
  };

  template <class T>
  bool is_value_initialized(const heap_object_wrapper<T>& arg)
  {
    return arg.is_wrapped_object_value_initialized();
  }


  // Returns zero when the specified object is value-initializated, and one otherwise.
  // Prints a message to standard output if the value-initialization has failed.
  template <class T>
  unsigned failed_to_value_initialized(const T& object, const char *const object_name)
  {
    if ( is_value_initialized(object) )
    {
      return 0u;
    }
    else
    {
      std::cout << "Note: Failed to value-initialize " << object_name << '.' << std::endl;
      return 1u;
    }
  }

// A macro that passed both the name and the value of the specified object to
// the function above here.
#define FAILED_TO_VALUE_INITIALIZE(value) failed_to_value_initialized(value, #value)


  // value_initializer initializes each of its data members by means
  // of an empty set of parentheses, and allows checking whether
  // each of them is indeed value-initialized, as specified by 
  // the C++ Standard ([dcl.init]).
  //
  // Note: its base class, int_struct, is there to try to reproduce GCC Bug 30111,
  // "Value-initialization of POD base class doesn't initialize members", reported
  // by Jonathan Wakely in 2006: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=30111
  class value_initializer: private int_struct
  {
  private:
    enum_holder m_enum_holder;
    enum_holder m_enum_holder_array[2];
    enum_type m_enum;
    enum_type m_enum_array[2];
    bool m_bool;
    bool m_bool_array[2];
    char m_char;
    char m_char_array[2];
    _2d_char_array_type m_2d_char_array;
    _3d_char_array_type m_3d_char_array;
    unsigned char m_unsigned_char;
    unsigned char m_unsigned_char_array[2];
    short m_short;
    short m_short_array[2];
    int m_int;
    int m_int_array[2];
    unsigned m_unsigned;
    unsigned m_unsigned_array[2];
    long m_long;
    long m_long_array[2];
    float m_float;
    float m_float_array[2];
    double m_double;
    double m_double_array[2];
    long double m_long_double;
    long double m_long_double_array[2];
    void* m_void_ptr;
    void* m_void_ptr_array[2];
    function_ptr_type m_function_ptr;
    function_ptr_type m_function_ptr_array[2];
    function_ptr_struct m_function_ptr_struct;
    function_ptr_struct m_function_ptr_struct_array[2];
    member_function_ptr_type m_member_function_ptr;
    member_function_ptr_type m_member_function_ptr_array[2];
    member_function_ptr_struct m_member_function_ptr_struct;
    member_function_ptr_struct m_member_function_ptr_struct_array[2];
    ptr_to_member_type  m_ptr_to_member;
    ptr_to_member_type  m_ptr_to_member_array[2];
    ptr_to_member_struct m_ptr_to_member_struct;
    ptr_to_member_struct m_ptr_to_member_struct_array[2];
    bit_field_struct m_bit_field_struct;
    bit_field_struct m_bit_field_struct_array[2];
    int_struct m_int_struct;
    int_struct m_int_struct_array[2];
    int_struct m_int_struct_holder;
    int_struct m_int_struct_holder_array[2];
    pod_struct m_pod_struct;
    pod_struct m_pod_struct_array[2];
    derived_pod_struct m_derived_pod_struct;
    derived_pod_struct m_derived_pod_struct_array[2];
    derived_struct m_derived_struct;
    derived_struct m_derived_struct_array[2];
    derived_int_struct m_derived_int_struct;
    derived_int_struct m_derived_int_struct_array[2];
    private_int_holder m_private_int_holder;
    private_int_holder m_private_int_holder_array[2];
    char_array_struct m_char_array_struct;
    char_array_struct m_char_array_struct_array[2];
    private_int_array_pair m_private_int_array_pair;
    private_int_array_pair m_private_int_array_pair_array[2];
    enum_holder_and_int m_enum_holder_and_int;
    enum_holder_and_int m_enum_holder_and_int_array[2];
    private_and_protected_int m_private_and_protected_int;
    private_and_protected_int m_private_and_protected_int_array[2];
    user_defined_copy_constructor_holder_and_int m_user_defined_copy_constructor_holder_and_int;
    user_defined_copy_constructor_holder_and_int m_user_defined_copy_constructor_holder_and_int_array[2];
    user_defined_destructor_holder m_user_defined_destructor_holder;
    user_defined_destructor_holder m_user_defined_destructor_holder_array[2];
    virtual_destructor_holder m_virtual_destructor_holder;
    virtual_destructor_holder m_virtual_destructor_holder_array[2];
    non_pod_class m_non_pod;
    non_pod_class m_non_pod_array[2];
    pod_struct_and_int_union m_pod_struct_and_int_union;
    pod_struct_and_int_union m_pod_struct_and_int_union_array[2];
    int_and_pod_struct_union m_int_and_pod_struct_union;
    int_and_pod_struct_union m_int_and_pod_struct_union_array[2];

  public:
    // Default constructor. Tries to value-initialize its base subobject and all
    // of its data.members.
    value_initializer()
    :
    // Note: CodeGear/Borland may produce a warning, W8039, for each data member
    // whose type is an array type, saying "Constructor initializer list ignored".
    // If it does, it probably won't value-initialize those arrays, as reported
    // by me (Niels Dekker, LKEB) in 2010, report 83751, "Value-initialization:
    // arrays should have each element value-initialized",
    // http://qc.embarcadero.com/wc/qcmain.aspx?d=83751
    // On the other hand, Microsoft Visual C++ may produce warnings of type C4351,
    // saying "new behavior: elements of array '...' will be default initialized",
    // which is actually the right behavior!
    int_struct(),
    m_enum_holder(),
    m_enum_holder_array(),
    m_enum(),
    m_enum_array(),
    m_bool(),
    m_bool_array(),
    m_char(),
    m_char_array(),
    m_2d_char_array(),
    m_3d_char_array(),
    m_unsigned_char(),
    m_unsigned_char_array(),
    m_short(),
    m_short_array(),
    m_int(),
    m_int_array(),
    m_unsigned(),
    m_unsigned_array(),
    m_long(),
    m_long_array(),
    m_float(),
    m_float_array(),
    m_double(),
    m_double_array(),
    m_long_double(),
    m_long_double_array(),
    m_void_ptr(),
    m_void_ptr_array(),
    m_function_ptr(),
    m_function_ptr_array(),
    m_function_ptr_struct(),
    m_function_ptr_struct_array(),
    m_member_function_ptr(),
    m_member_function_ptr_array(),
    m_member_function_ptr_struct(),
    m_member_function_ptr_struct_array(),
    m_ptr_to_member(),
    m_ptr_to_member_array(),
    m_ptr_to_member_struct(),
    m_ptr_to_member_struct_array(),
    m_bit_field_struct(),
    m_bit_field_struct_array(),
    m_int_struct(),
    m_int_struct_array(),
    m_int_struct_holder(),
    m_int_struct_holder_array(),
    m_pod_struct(),
    m_pod_struct_array(),
    m_derived_pod_struct(),
    m_derived_pod_struct_array(),
    m_derived_struct(),
    m_derived_struct_array(),
    m_derived_int_struct(),
    m_derived_int_struct_array(),
    m_private_int_holder(),
    m_private_int_holder_array(),
    m_char_array_struct(),
    m_char_array_struct_array(),
    m_private_int_array_pair(),
    m_private_int_array_pair_array(),
    m_enum_holder_and_int(),
    m_enum_holder_and_int_array(),
    m_private_and_protected_int(),
    m_private_and_protected_int_array(),
    m_user_defined_copy_constructor_holder_and_int(),
    m_user_defined_copy_constructor_holder_and_int_array(),
    m_user_defined_destructor_holder(),
    m_user_defined_destructor_holder_array(),
    m_virtual_destructor_holder(),
    m_virtual_destructor_holder_array(),
    m_non_pod(),
    m_non_pod_array(),
    m_pod_struct_and_int_union(),
    m_pod_struct_and_int_union_array(),
    m_int_and_pod_struct_union(),
    m_int_and_pod_struct_union_array()
    {
    }

    // Returns the number of failures.
    unsigned check_value_initialization_of_subobjects() const
    {
      const unsigned num_failures = 
        FAILED_TO_VALUE_INITIALIZE(int_struct::data) +
        FAILED_TO_VALUE_INITIALIZE(m_enum_holder) +
        FAILED_TO_VALUE_INITIALIZE(m_enum_holder_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_enum_holder_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_enum) +
        FAILED_TO_VALUE_INITIALIZE(m_enum_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_enum_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_bool) +
        FAILED_TO_VALUE_INITIALIZE(m_bool_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_bool_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_char) +
        FAILED_TO_VALUE_INITIALIZE(m_char_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_char_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_2d_char_array) +
        FAILED_TO_VALUE_INITIALIZE(m_3d_char_array) +
        FAILED_TO_VALUE_INITIALIZE(m_unsigned_char) +
        FAILED_TO_VALUE_INITIALIZE(m_unsigned_char_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_unsigned_char_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_short) +
        FAILED_TO_VALUE_INITIALIZE(m_short_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_short_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_int) +
        FAILED_TO_VALUE_INITIALIZE(m_int_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_int_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_unsigned) +
        FAILED_TO_VALUE_INITIALIZE(m_unsigned_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_unsigned_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_long) +
        FAILED_TO_VALUE_INITIALIZE(m_long_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_long_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_float) +
        FAILED_TO_VALUE_INITIALIZE(m_float_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_float_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_double) +
        FAILED_TO_VALUE_INITIALIZE(m_double_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_double_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_long_double) +
        FAILED_TO_VALUE_INITIALIZE(m_long_double_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_long_double_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_void_ptr) +
        FAILED_TO_VALUE_INITIALIZE(m_void_ptr_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_void_ptr_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_function_ptr) +
        FAILED_TO_VALUE_INITIALIZE(m_function_ptr_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_function_ptr_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_function_ptr_struct) +
        FAILED_TO_VALUE_INITIALIZE(m_function_ptr_struct_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_function_ptr_struct_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_member_function_ptr) +
        FAILED_TO_VALUE_INITIALIZE(m_member_function_ptr_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_member_function_ptr_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_member_function_ptr_struct) +
        FAILED_TO_VALUE_INITIALIZE(m_member_function_ptr_struct_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_member_function_ptr_struct_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_ptr_to_member) +
        FAILED_TO_VALUE_INITIALIZE(m_ptr_to_member_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_ptr_to_member_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_ptr_to_member_struct) +
        FAILED_TO_VALUE_INITIALIZE(m_ptr_to_member_struct_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_ptr_to_member_struct_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_bit_field_struct) +
        FAILED_TO_VALUE_INITIALIZE(m_bit_field_struct_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_bit_field_struct_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_int_struct) +
        FAILED_TO_VALUE_INITIALIZE(m_int_struct_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_int_struct_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_int_struct_holder) +
        FAILED_TO_VALUE_INITIALIZE(m_int_struct_holder_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_int_struct_holder_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_pod_struct) +
        FAILED_TO_VALUE_INITIALIZE(m_pod_struct_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_pod_struct_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_derived_pod_struct) +
        FAILED_TO_VALUE_INITIALIZE(m_derived_pod_struct_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_derived_pod_struct_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_derived_struct) +
        FAILED_TO_VALUE_INITIALIZE(m_derived_struct_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_derived_struct_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_derived_int_struct) +
        FAILED_TO_VALUE_INITIALIZE(m_derived_int_struct_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_derived_int_struct_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_private_int_holder) +
        FAILED_TO_VALUE_INITIALIZE(m_private_int_holder_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_private_int_holder_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_char_array_struct) +
        FAILED_TO_VALUE_INITIALIZE(m_char_array_struct_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_char_array_struct_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_private_int_array_pair) +
        FAILED_TO_VALUE_INITIALIZE(m_private_int_array_pair_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_private_int_array_pair_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_enum_holder_and_int) +
        FAILED_TO_VALUE_INITIALIZE(m_enum_holder_and_int_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_enum_holder_and_int_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_private_and_protected_int) +
        FAILED_TO_VALUE_INITIALIZE(m_private_and_protected_int_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_private_and_protected_int_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_user_defined_copy_constructor_holder_and_int) +
        FAILED_TO_VALUE_INITIALIZE(m_user_defined_copy_constructor_holder_and_int_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_user_defined_copy_constructor_holder_and_int_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_user_defined_destructor_holder) +
        FAILED_TO_VALUE_INITIALIZE(m_user_defined_destructor_holder_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_user_defined_destructor_holder_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_virtual_destructor_holder) +
        FAILED_TO_VALUE_INITIALIZE(m_virtual_destructor_holder_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_virtual_destructor_holder_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_non_pod) +
        FAILED_TO_VALUE_INITIALIZE(m_non_pod_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_non_pod_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_pod_struct_and_int_union) +
        FAILED_TO_VALUE_INITIALIZE(m_pod_struct_and_int_union_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_pod_struct_and_int_union_array[1]) +
        FAILED_TO_VALUE_INITIALIZE(m_int_and_pod_struct_union) +
        FAILED_TO_VALUE_INITIALIZE(m_int_and_pod_struct_union_array[0]) +
        FAILED_TO_VALUE_INITIALIZE(m_int_and_pod_struct_union_array[1]);
      return num_failures;
    }
  };

  // Checks value-initialization of a number of small temporary objects.
  // Returns the number of failures.
  unsigned check_value_initialization_of_temporaries()
  {
    typedef long double long_double_type;
    typedef unsigned char unsigned_char_type;
    typedef void* void_ptr_type;

    const unsigned num_failures = 
      FAILED_TO_VALUE_INITIALIZE(enum_holder()) +
      FAILED_TO_VALUE_INITIALIZE(enum_type()) +
      FAILED_TO_VALUE_INITIALIZE(bool()) +
      FAILED_TO_VALUE_INITIALIZE(char()) +
      FAILED_TO_VALUE_INITIALIZE(unsigned_char_type()) +
      FAILED_TO_VALUE_INITIALIZE(short()) +
      FAILED_TO_VALUE_INITIALIZE(int()) +
      FAILED_TO_VALUE_INITIALIZE(unsigned()) +
      FAILED_TO_VALUE_INITIALIZE(long()) +
      FAILED_TO_VALUE_INITIALIZE(float()) +
      FAILED_TO_VALUE_INITIALIZE(double()) +
      FAILED_TO_VALUE_INITIALIZE(long_double_type()) +
      FAILED_TO_VALUE_INITIALIZE(void_ptr_type()) +
      FAILED_TO_VALUE_INITIALIZE(bit_field_struct()) +
      FAILED_TO_VALUE_INITIALIZE(function_ptr_type()) +
      FAILED_TO_VALUE_INITIALIZE(function_ptr_struct()) +
      FAILED_TO_VALUE_INITIALIZE(member_function_ptr_type()) +
      FAILED_TO_VALUE_INITIALIZE(member_function_ptr_struct()) +
      FAILED_TO_VALUE_INITIALIZE(ptr_to_member_type()) +
      FAILED_TO_VALUE_INITIALIZE(ptr_to_member_struct()) +
      FAILED_TO_VALUE_INITIALIZE(int_struct()) +
      FAILED_TO_VALUE_INITIALIZE(int_struct_holder()) +
      FAILED_TO_VALUE_INITIALIZE(pod_struct()) +
      FAILED_TO_VALUE_INITIALIZE(derived_pod_struct()) +
      FAILED_TO_VALUE_INITIALIZE(derived_struct()) +
      FAILED_TO_VALUE_INITIALIZE(derived_int_struct()) +
      FAILED_TO_VALUE_INITIALIZE(private_int_holder()) +
      FAILED_TO_VALUE_INITIALIZE(char_array_struct()) +
      FAILED_TO_VALUE_INITIALIZE(private_int_array_pair()) +
      // IBM's XL V10.1.0.0 may fail to value-initialize a temporary of a non-POD
      // type like enum_holder_and_int, virtual_destructor_holder, or non_pod_class, 
      // as appeared at the Boost Config/trunk regression page in April 2010.
      // Michael Wong (IBM Canada Ltd) confirmed the issue to me (Niels Dekker, LKEB),
      // and gave it high priority.
      FAILED_TO_VALUE_INITIALIZE(enum_holder_and_int()) +
      FAILED_TO_VALUE_INITIALIZE(private_and_protected_int()) +
      FAILED_TO_VALUE_INITIALIZE(user_defined_copy_constructor_holder_and_int()) +
      // The following line, doing user_defined_destructor_holder(), causes  
      // a compilation error on Embarcadero 2010 (Borland/CodeGear 6.21), 
      // as reported by me (Niels Dekker, LKEB) in 2010, bug report 83851,
      // "Value-initialized temporary triggers internal backend error C1798",
      // http://qc.embarcadero.com/wc/qcmain.aspx?d=83851
      FAILED_TO_VALUE_INITIALIZE(user_defined_destructor_holder()) +
      FAILED_TO_VALUE_INITIALIZE(virtual_destructor_holder()) +
      FAILED_TO_VALUE_INITIALIZE(non_pod_class()) +
      FAILED_TO_VALUE_INITIALIZE(pod_struct_and_int_union()) +
      FAILED_TO_VALUE_INITIALIZE(int_and_pod_struct_union());
    return num_failures;
  }

  // Checks value-initialization of small heap objects.
  // Returns the number of failures.
  unsigned check_value_initialization_of_heap_objects()
  {
    const unsigned num_failures = 
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<enum_holder>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<enum_type>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<bool>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<char>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<unsigned char>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<short>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<int>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<unsigned>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<long>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<float>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<double>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<long double>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<void*>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<function_ptr_type>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<function_ptr_struct>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<member_function_ptr_type>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<member_function_ptr_struct>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<ptr_to_member_type>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<ptr_to_member_struct>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<bit_field_struct>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<int_struct>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<int_struct>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<pod_struct>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<derived_pod_struct>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<derived_struct>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<derived_int_struct>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<char_array_struct>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<private_int_holder>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<private_int_array_pair>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<enum_holder_and_int>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<private_and_protected_int>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<user_defined_copy_constructor_holder_and_int>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<user_defined_destructor_holder>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<virtual_destructor_holder>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<non_pod_class>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<pod_struct_and_int_union>() ) +
      FAILED_TO_VALUE_INITIALIZE( heap_object_wrapper<int_and_pod_struct_union>() );
    return num_failures;
  }

  // Equivalent to the dirty_stack() function from GCC Bug 33916,
  // "Default constructor fails to initialize array members", reported in 2007 by
  // Michael Elizabeth Chastain: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=33916
  void dirty_stack()
  {
    unsigned char array_on_stack[sizeof(value_initializer) + 256];
    for (unsigned i = 0; i < sizeof(array_on_stack); ++i)
    {
      array_on_stack[i] = 0x11;
    }
  }


  // Checks value-initialization of the subobjects of a temporary object, 
  // an object on the stack, an object on the heap; furthermore it checks 
  // value-initialization of a number of smaller temporary objects and 
  // heap objects.
  int test()
  {
    unsigned total_num_failures = 0;

    dirty_stack();
    const unsigned num_failures_of_subobjects_of_a_temporary =
      value_initializer().check_value_initialization_of_subobjects();

    total_num_failures += num_failures_of_subobjects_of_a_temporary;
    if ( total_num_failures > 0 )
    {
      std::cout << "- Number of subobject initialization failures of a temporary: "
        << num_failures_of_subobjects_of_a_temporary << std::endl;
    }
    dirty_stack();
    value_initializer object_on_stack;
    const unsigned num_failures_of_subobjects_on_stack =
      object_on_stack.check_value_initialization_of_subobjects();

    total_num_failures += num_failures_of_subobjects_on_stack;
    if ( total_num_failures > 0 )
    {
      std::cout << "- Number of subobject initialization failures on the stack: "
        << num_failures_of_subobjects_on_stack << std::endl;
    }
    const value_initializer* const ptr = new value_initializer();
    const unsigned num_failures_of_subobjects_on_heap = ptr->check_value_initialization_of_subobjects();
    delete ptr;

    total_num_failures += num_failures_of_subobjects_on_heap;
    if ( total_num_failures > 0 )
    {
      std::cout << "- Number of subobject initialization failures on the heap: "
        << num_failures_of_subobjects_on_heap << std::endl;
    }

    dirty_stack();
    const unsigned num_failures_of_temporaries = check_value_initialization_of_temporaries();

    total_num_failures += num_failures_of_temporaries;
    if ( total_num_failures > 0 )
    {
      std::cout << "- Number of initialization failures of temporary objects: "
        << num_failures_of_temporaries << std::endl;
    }

    const unsigned num_failures_of_heap_objects = check_value_initialization_of_heap_objects();

    total_num_failures += num_failures_of_heap_objects;
    if ( total_num_failures > 0 )
    {
      std::cout << "- Number of failures of heap objects: "
        << num_failures_of_heap_objects << std::endl;
    }

    if ( total_num_failures > 0 )
    {
      std::cout << "-- Total number of initialization failures ("
        << num_failures_of_subobjects_of_a_temporary << '+'
        << num_failures_of_subobjects_on_stack << '+'
        << num_failures_of_subobjects_on_heap << '+'
        << num_failures_of_temporaries << '+'
        << num_failures_of_heap_objects << "): "
        << total_num_failures
        << "\nDetected by boost_no_complete_value_initialization::test() revision 32."
        << std::endl;
    }
    return static_cast<int>(total_num_failures);
  }

}  // End of namespace.

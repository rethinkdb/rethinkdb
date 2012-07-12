# ADT-extended protobuf preprocessor

Input is approximately protobuf syntax, with a few extensions and a few missing
parts. Output is (a) protobuf syntax (b) C++ code that checks structures for
well-formedness.

In particular:

1. Fields are "required" by default and are automatically given tags based on
   their order in the file. So instead of:

       required int foo = 1;
       optional string bar = 2;

   You write:

       int foo;
       optional string bar;

   These are arguably misfeatures.

2. Adds a "data" construct to define an algebraic data type / variant / tagged
   union. Inside of "data" you define "branches". Branches can have associated
   data, which can either be a message, another type, or nothing.

   Example:

       data Example {
           branch Branch1 {
               // A message body goes here.
               int foo;
               string blah;
               repeated Example children;
           };

           // this branch is just an int, no message type
           branch Branch2 = int;

           // this branch has no associated data
           branch Branch3;
       };

   This turns into the following protobuf code:

       message Example {
           enum ExampleType {
               BRANCH1 = 0;
               BRANCH2 = 1;
               BRANCH3 = 2;
           };
           required ExampleType type = 1;

           message Branch1 {
               required int foo = 1;
               required string blah = 2;
               repeated Example children = 3;
           };
           optional Branch1 branch1;

           optional int branch2;
       };

   And the following C++ checker code:

       bool well_formed(const Example &self) {
            if (self.type == Example::BRANCH1 && !self.has_branch1())
                return false;

            if (self.has_branch1() && self.type != Example::BRANCH1)
                return false;

            if (self.type == Example::BRANCH2 && !self.has_branch2())
                return false;

            if (self.has_branch2() && self.type != Example::BRANCH2)
                return false;

            return true;
       }

# Bugs

Doesn't recursively check for well-formedness yet.

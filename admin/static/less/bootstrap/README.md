# Bootstrap's less files #

## Note ##
This is a fork of Bootstrap.

The reason why we did a fork is because this version of Bootstrap does not compile with Less 1.4

From Less 1.4 changelog:  
(~".myclass_@{index}") { ... selector interpolation is deprecated, do this instead .myclass_@{index} { .... This works in 1.3.1 onwards.


This change has been done at the following lines:
Line 529
-      (~".span@{index}") { .span(@index); }
+      .span@{index} { .span(@index); }

Line 535
-      (~".offset@{index}") { .offset(@index); }
+      .offset@{index} { .offset(@index); }

Line 572
-      (~"> .span@{index}") { .span(@index); }
+      > .span@{index} { .span(@index); }

Line 601
-      (~"input.span@{index}, textarea.span@{index}, .uneditable-input.span@{index}") { .span(@index); }
+      input.span@{index}, textarea.span@{index}, .uneditable-input.span@{index} { .span(@index); }

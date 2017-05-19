.. title:: clang-tidy - misc-assertion-count

misc-assertion-count
====================

Allows to impose limits on the lines-per-assertions ratio for functions.

Finds functions that have more than `LinesThreshold` lines, counts assertions
in them, and if the ratio of lines-per-assert is higher than configured,
emits a warning.

The lines-per-assert ratio can be controlled via four options, which form
a stair-step curve, with first two options controlling the static part, and
the second two controlling the dynamic part - i.e. how the expected minimal
assertion count changes with the line count.

Options
-------

.. option:: LineThreshold

   How many lines the function should have to be checked. This parameter can be
   considered as LOC cut-off threshold - if the function is smaller than that,
   no check will be performed. The default is `-1` (do not check any functions).

.. option:: AssertsThreshold

   If the function is checked (i.e. it has no less than `LineThreshold` lines),
   then the function should have no less than `AssertsThreshold` assertions.
   The default is `0` (there is no static part of the assertion count curve).

.. option:: LinesStep

   These two options control the dynamic part of the curve, how the expected
   minimal assertion count changes with number of lines.
   The default is `0` (there is no dynamic part of the curve. i.e. if the
   function is checked, then regardless of the function LOC count, it must have
   no less than `AssertsThreshold` assertions).

.. option:: AssertsStep

   These two options control the dynamic part of the curve, how the expected
   minimal assertion count changes with number of lines.
   `LinesStep` and `AssertsStep` options work together. If the function is
   checked (i.e. it has no less than `LineThreshold` lines), then for each
   `LinesStep` lines over the `LineThreshold`, the expected minimal assertion
   count is increased by `AssertsStep`.
   The default is `0` (there is no dynamic part of the curve. i.e. if the
   function is checked, then regardless of the function LOC count, it must have
   no less than `AssertsStep` assertions).

.. option:: AssertNames

   A comma-separated list of the names of macros, and non-member functions to
   be counted as an assertion.
   Additionally, `static_assert()` is also counted as an assertion.

Note:
   If `AssertsThreshold` is zero, and either one (or both) of `LinesStep` and
   `AssertsStep` is zero, then no check will be performerd. Because such a
   config says that no limits are imposed.

Config examples:
   - Any function over ten lines should have at least one assertion:
     ```
     - key:             misc-assertion-count.LineThreshold
       value:           '10'
     - key:             misc-assertion-count.AssertsThreshold
       value:           '1'
     - key:             misc-assertion-count.LinesStep
       value:           '0'
     - key:             misc-assertion-count.AssertsStep
       value:           '0'
     ```
   - One assertion per each ten lines of function:
     ```
     - key:             misc-assertion-count.LineThreshold
       value:           '0'
     - key:             misc-assertion-count.AssertsThreshold
       value:           '0'
     - key:             misc-assertion-count.LinesStep
       value:           '10'
     - key:             misc-assertion-count.AssertsStep
       value:           '1'
     ```
     or, equivalently, any function over ten lines should have at least one
     assertion, and then one assertion per each ten lines of code:
     ```
     - key:             misc-assertion-count.LineThreshold
       value:           '10'
     - key:             misc-assertion-count.AssertsThreshold
       value:           '1'
     - key:             misc-assertion-count.LinesStep
       value:           '10'
     - key:             misc-assertion-count.AssertsStep
       value:           '1'
     ```
   - Any function over ten lines should have at least one assertion, and then
     two assertion per each twenty lines of code
     ```
     - key:             misc-assertion-count.LineThreshold
       value:           '10'
     - key:             misc-assertion-count.AssertsThreshold
       value:           '1'
     - key:             misc-assertion-count.LinesStep
       value:           '20'
     - key:             misc-assertion-count.AssertsStep
       value:           '2'
     ```

.. title:: clang-tidy - hicpp-multiway-paths-covered

hicpp-multiway-paths-covered
============================

This check catches multiple occasion, where not all possible code paths are covered.
The `rule 6.1.2 <http://www.codingstandard.com/rule/6-1-2-explicitly-cover-all-paths-through-multi-way-selection-statements/>`_
and `rule 6.1.4 <http://www.codingstandard.com/rule/6-1-4-ensure-that-a-switch-statement-has-at-least-two-case-labels-distinct-from-the-default-label/>`_
of the High Integrity C++ Coding Standard are enforced.

``if-else if`` chains that miss a final ``else`` branch might lead to unexpected 
program execution and be the result of a logical error.
If the missing ``else`` branch is intended, you can leave it empty with a clarifying
comment.
Since this warning might be very noise on many codebases it is configurable and by 
default deactivated.

.. code-block:: c++

  void f1() {
    int i = determineTheNumber();

     if(i > 0) { 
       // Some Calculation 
     } else if (i < 0) { 
       // Precondition violated or something else. 
     }
     // ...
  }

Similar arguments hold for ``switch`` statements, that do not cover all possible code paths.

.. code-block:: c++

  // The missing default branch might be a logical error. It can be kept empty
  // if there is nothing to do, making it explicit.
  void f2(int i) {
    switch (i) {
    case 0: // something
      break;
    case 1: // something else
      break;
    }
    // All other numbers?
  }

  // Violates this rule as well, but already emits a compiler warning (-Wswitch).
  enum Color { Red, Green, Blue, Yellow };
  void f3(enum Color c) {
    switch (c) {
    case Red: // We can't drive for now.
      break;
    case Green:  // We are allowed to drive.
      break;
    }
    // Other cases missing
  }


The `rule 6.1.4 <http://www.codingstandard.com/rule/6-1-4-ensure-that-a-switch-statement-has-at-least-two-case-labels-distinct-from-the-default-label/>`_
requires every ``switch`` statement to have at least two ``case`` labels, that are not default.
Otherwise the ``switch`` could be better expressed with a common ``if`` statement.
Completly degenerated ``switch`` statements without any labels are catched as well.

.. code-block:: c++

  // Degenerated switch, that could be better written as if()
  int i = 42;
  switch(i) {
    case 1: // do something here
    default: // do somethe else here
  }

  // Should rather be the following:
  if (i == 1) { 
    // do something here 
  }
  else { 
    // do something here 
  }


.. code-block:: c++
  
  // Completly degenerated switch, will be warned.
  int i = 42;
  switch(i) {}


Options
-------

.. option:: WarnOnMissingElse

  Activates warning for missing``else`` branches. Default is `0`.

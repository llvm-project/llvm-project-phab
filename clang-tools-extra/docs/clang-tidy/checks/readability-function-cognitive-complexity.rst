.. title:: clang-tidy - readability-function-cognitive-complexity

readability-function-cognitive-complexity
=========================================

Checks function Cognitive Complexity metric.

The metric is implemented as per `COGNITIVE COMPLEXITY by SonarSource
<https://www.sonarsource.com/docs/CognitiveComplexity.pdf>`_ specification
version 1.2 (19 April 2017), with two notable exceptions:
   * `preprocessor conditionals` (`#ifdef`, `#if`, `#elif`, `#else`, `#endif`)
     are not accounted for. Could be done.
   * `each method in a recursion cycle` is not accounted for. It can't be fully
     implemented, because cross-translational-unit analysis would be needed,
     which is not possible in clang-tidy.

Options
-------

.. option:: Threshold

   Flag functions with Cognitive Complexity exceeding this number.
   The default is `25`.

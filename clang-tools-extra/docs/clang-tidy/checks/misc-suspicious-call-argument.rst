misc-suspicious-call-argument
=============================

This checker finds those function calls where the function arguments are 
provided in an incorrect order. It compares the name of the given variable 
to the argument name in the function definition.

It issues a message if the given variable name is similar to an another 
function argument in a function call. It uses case insensitive comparison.

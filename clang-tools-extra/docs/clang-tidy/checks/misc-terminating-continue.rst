.. title:: clang-tidy - misc-terminating-continue

misc-terminating-continue
=========================

Detects `do while` loops with `false` conditions that have `continue` statement
as this `continue` terminates the loop effectively.

.. code:: c++

  Parser::TPResult Parser::TryParseProtocolQualifiers() {
    assert(Tok.is(tok::less) && "Expected '<' for qualifier list");
    ConsumeToken();
    do {
      if (Tok.isNot(tok::identifier))
        return TPResult::Error;
      ConsumeToken();

      if (Tok.is(tok::comma)) {
        ConsumeToken();
        continue;
      }

      if (Tok.is(tok::greater)) {
        ConsumeToken();
        return TPResult::Ambiguous;
      }
    } while (false);

    return TPResult::Error;
  }

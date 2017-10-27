// Check names may only contain alphanumeric characters, '-', '_', and '.'.
// RUN: clang-tidy -checks=*,-clang-diagnostic* -list-checks | grep '^    ' | cut -b5- | not grep -v '^[a-zA-Z0-9_.\-]\+$'

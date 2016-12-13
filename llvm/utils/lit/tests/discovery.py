# Check the basic discovery process, including a sub-suite.
#
# RUN: %{lit} %{inputs}/discovery \
# RUN:   -j 1 --debug --show-tests --show-suites \
# RUN:   -v > %t.out 2> %t.err
# RUN: FileCheck --check-prefix=CHECK-BASIC-OUT < %t.out %s
# RUN: FileCheck --check-prefix=CHECK-BASIC-ERR < %t.err %s
#
# CHECK-BASIC-ERR: loading suite config '{{.*}}[[SEP:[/\\]+]]discovery[[SEP]]lit.cfg'
# CHECK-BASIC-ERR-DAG: loading suite config '{{.*}}[[SEP]]discovery[[SEP]]subsuite[[SEP]]lit.cfg'
# CHECK-BASIC-ERR-DAG: loading local config '{{.*}}[[SEP]]discovery[[SEP]]subdir[[SEP]]lit.local.cfg'
#
# CHECK-BASIC-OUT: -- Test Suites --
# CHECK-BASIC-OUT:   sub-suite - 2 tests
# CHECK-BASIC-OUT:     Source Root: {{.*}}[[SEP:[/\\]+]]discovery[[SEP]]subsuite{{$}}
# CHECK-BASIC-OUT:     Exec Root  : {{.*}}[[SEP]]discovery[[SEP]]subsuite{{$}}
# CHECK-BASIC-OUT:   top-level-suite - 3 tests
# CHECK-BASIC-OUT:     Source Root: {{.*}}[[SEP]]discovery{{$}}
# CHECK-BASIC-OUT:     Exec Root  : {{.*}}[[SEP]]discovery{{$}}
#
# CHECK-BASIC-OUT: -- Available Tests --
# CHECK-BASIC-OUT: sub-suite :: test-one
# CHECK-BASIC-OUT: sub-suite :: test-two
# CHECK-BASIC-OUT: top-level-suite :: subdir/test-three
# CHECK-BASIC-OUT: top-level-suite :: test-one
# CHECK-BASIC-OUT: top-level-suite :: test-two


# Check discovery when exact test names are given.
#
# RUN: %{lit} \
# RUN:     %{inputs}/discovery/subdir/test-three.py \
# RUN:     %{inputs}/discovery/subsuite/test-one.txt \
# RUN:   -j 1 --show-tests --show-suites -v > %t.out
# RUN: FileCheck --check-prefix=CHECK-EXACT-TEST < %t.out %s
#
# CHECK-EXACT-TEST: -- Available Tests --
# CHECK-EXACT-TEST: sub-suite :: test-one
# CHECK-EXACT-TEST: top-level-suite :: subdir/test-three


# Check discovery when using an exec path.
#
# RUN: %{lit} %{inputs}/exec-discovery \
# RUN:   -j 1 --debug --show-tests --show-suites \
# RUN:   -v > %t.out 2> %t.err
# RUN: FileCheck --check-prefix=CHECK-ASEXEC-OUT < %t.out %s
# RUN: FileCheck --check-prefix=CHECK-ASEXEC-ERR < %t.err %s
#
# CHECK-ASEXEC-ERR: loading suite config '{{.*}}[[SEP:[/\\]+]]exec-discovery[[SEP]]lit.site.cfg'
# CHECK-ASEXEC-ERR: load_config from '{{.*}}[[SEP]]discovery[[SEP]]lit.cfg'
# CHECK-ASEXEC-ERR: loaded config '{{.*}}[[SEP]]discovery[[SEP]]lit.cfg'
# CHECK-ASEXEC-ERR: loaded config '{{.*}}[[SEP]]exec-discovery[[SEP]]lit.site.cfg'
# CHECK-ASEXEC-ERR-DAG: loading suite config '{{.*}}[[SEP]]discovery[[SEP]]subsuite[[SEP]]lit.cfg'
# CHECK-ASEXEC-ERR-DAG: loading local config '{{.*}}[[SEP]]discovery[[SEP]]subdir[[SEP]]lit.local.cfg'
#
# CHECK-ASEXEC-OUT: -- Test Suites --
# CHECK-ASEXEC-OUT:   sub-suite - 2 tests
# CHECK-ASEXEC-OUT:     Source Root: {{.*}}[[SEP:[/\\]+]]discovery[[SEP]]subsuite{{$}}
# CHECK-ASEXEC-OUT:     Exec Root  : {{.*}}[[SEP]]discovery[[SEP]]subsuite{{$}}
# CHECK-ASEXEC-OUT:   top-level-suite - 3 tests
# CHECK-ASEXEC-OUT:     Source Root: {{.*}}[[SEP]]discovery{{$}}
# CHECK-ASEXEC-OUT:     Exec Root  : {{.*}}[[SEP]]exec-discovery{{$}}
#
# CHECK-ASEXEC-OUT: -- Available Tests --
# CHECK-ASEXEC-OUT: sub-suite :: test-one
# CHECK-ASEXEC-OUT: sub-suite :: test-two
# CHECK-ASEXEC-OUT: top-level-suite :: subdir/test-three
# CHECK-ASEXEC-OUT: top-level-suite :: test-one
# CHECK-ASEXEC-OUT: top-level-suite :: test-two

# Check discovery when exact test names are given.
#
# FIXME: Note that using a path into a subsuite doesn't work correctly here.
#
# RUN: %{lit} \
# RUN:     %{inputs}/exec-discovery/subdir/test-three.py \
# RUN:   -j 1 --show-tests --show-suites -v > %t.out
# RUN: FileCheck --check-prefix=CHECK-ASEXEC-EXACT-TEST < %t.out %s
#
# CHECK-ASEXEC-EXACT-TEST: -- Available Tests --
# CHECK-ASEXEC-EXACT-TEST: top-level-suite :: subdir/test-three


# Check that we don't recurse infinitely when loading an site specific test
# suite located inside the test source root.
#
# RUN: %{lit} \
# RUN:     %{inputs}/exec-discovery-in-tree/obj/ \
# RUN:   -j 1 --show-tests --show-suites -v > %t.out
# RUN: FileCheck --check-prefix=CHECK-ASEXEC-INTREE < %t.out %s
#
#      CHECK-ASEXEC-INTREE:   exec-discovery-in-tree-suite - 1 tests
# CHECK-ASEXEC-INTREE-NEXT:     Source Root: {{.*}}[[SEP:[/\\]+]]exec-discovery-in-tree{{$}}
# CHECK-ASEXEC-INTREE-NEXT:     Exec Root  : {{.*}}[[SEP]]exec-discovery-in-tree[[SEP]]obj{{$}}
# CHECK-ASEXEC-INTREE-NEXT: -- Available Tests --
# CHECK-ASEXEC-INTREE-NEXT: exec-discovery-in-tree-suite :: test-one

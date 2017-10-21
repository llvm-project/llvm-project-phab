
from clang.cindex import LinkageKind
from clang.cindex import Cursor
from clang.cindex import TranslationUnit

from .util import get_cursor
from .util import get_tu

def test_linkage():
    """Ensure that linkage specifers are available on cursors"""

    tu = get_tu("""
void foo() { int no_linkage; }
static int internal;
extern int external;
""", lang = 'cpp')

    no_linkage = get_cursor(tu.cursor, 'no_linkage')
    assert no_linkage.linkage == LinkageKind.NO_LINKAGE;

    internal = get_cursor(tu.cursor, 'internal')
    assert internal.linkage == LinkageKind.INTERNAL

    external = get_cursor(tu.cursor, 'external')
    assert external.linkage == LinkageKind.EXTERNAL


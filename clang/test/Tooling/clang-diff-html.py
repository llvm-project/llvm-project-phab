# RUN: clang-diff -html -no-compilation-database %S/Inputs/clang-diff-basic-src.cpp %S/clang-diff-basic.cpp | env python3 %s > %t.filecheck
# RUN: clang-diff -m    -no-compilation-database %S/Inputs/clang-diff-basic-src.cpp %S/clang-diff-basic.cpp | FileCheck %t.filecheck

from html.parser import HTMLParser
from sys import stdin

class LeftParser(HTMLParser):
    def __init__(self):
        HTMLParser.__init__(self)
        self.active = False
    def handle_starttag(self, tag, attrs):
        a = {key: val for key, val in attrs}
        if tag == 'div' and a.get('id') == 'L':
            self.active = True
            return
        if not self.active:
            return
        assert tag == 'span'
        if '-1' in a['tid']:
            check('Delete %s' % shownode(a))

    def handle_endtag(self, tag):
        if tag == 'div':
            self.active = False

class RightParser(HTMLParser):
    def __init__(self):
        HTMLParser.__init__(self)
        self.active = False
    def handle_starttag(self, tag, attrs):
        a = {key: val for key, val in attrs}
        if tag == 'div' and a.get('id') == 'R':
            self.active = True
            return
        if not self.active:
            return
        assert tag == 'span'
        if '-1' in a['tid']:
            check('Insert %s' % shownode(a))
        else:
            check('Match {{.*}} to %s' % shownode(a))

    def handle_endtag(self, tag):
        if tag == 'div':
            self.active = False

def check(*args):
    print('CHECK:', *args)

def shownode(attrdict):
    title = attrdict['title'].splitlines()
    kind = title[0]
    value = '' if len(title) < 3 else (': ' + title[2])
    id = attrdict['id'][1:]
    return '%s%s(%s)' % (kind, value, id)

input = stdin.read()
RightParser().feed(input)
LeftParser().feed(input)

from __future__ import print_function

import clang.cindex


def traverse(node):
    print('L:', node.location.line, node.spelling, node.type.spelling, node.mangled_name)
    for child in node.get_children():
        traverse(child)

with open('test.cpp', 'w') as o:
    o.write('''struct S { 
  void(*g)(void); 
  void(*f)(void*); 
};
''')

index = clang.cindex.Index.create()
tu = index.parse('test.cpp', args=['-Werror'])
traverse(tu.cursor)

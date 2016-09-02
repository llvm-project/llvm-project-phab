'''
Minimal clang-refactor rename integration with Vim.

Before installing make sure one of the following is satisfied:

* clang-refactor is in your PATH
* `g:clang_rename_path` in ~/.vimrc points to valid clang-refactor executable
* `binary` in clang-refactor rename.py points to valid to clang-refactor
executable

To install, simply put this into your ~/.vimrc

    noremap <leader>cr :pyf <path-to>/clang-refactor-rename.py<cr>

IMPORTANT NOTE: Before running the tool, make sure you saved the file.

All you have to do now is to place a cursor on a variable/function/class which
you would like to rename and press '<leader>cr'. You will be prompted for a new
name if the cursor points to a valid symbol.
'''

import vim
import subprocess
import sys

def main():
    binary = 'clang-refactor'
    if vim.eval('exists("g:clang_rename_path")') == "1":
        binary = vim.eval('g:clang_rename_path')

    # Get arguments for clang-refactor binary.
    offset = int(vim.eval('line2byte(line("."))+col(".")')) - 2
    if offset < 0:
        print >> sys.stderr, '''Couldn\'t determine cursor position.
                                Is your file empty?'''
        return
    filename = vim.current.buffer.name

    new_name_request_message = 'type new name:'
    new_name = vim.eval("input('{}\n')".format(new_name_request_message))

    # Call clang-refactor rename module.
    command = [binary,
               filename,
               'rename',
               '-i',
               '-offset', str(offset),
               '-new-name', str(new_name)]
    # FIXME: make it possible to run the tool on unsaved file.
    p = subprocess.Popen(command,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()

    if stderr:
        print stderr

    # Reload all buffers in Vim.
    vim.command("bufdo edit")


if __name__ == '__main__':
    main()

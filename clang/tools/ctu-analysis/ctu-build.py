#!/usr/bin/env python

import argparse
import json
import logging
import multiprocessing
import os
import re
import signal
import subprocess


SOURCE_PATTERN = re.compile('.*\.(C|c|cc|cpp|cxx|ii|m|mm)$', re.IGNORECASE)
TIMEOUT = 86400
DEFINED_FUNCTIONS_FILENAME = 'definedFns.txt'
EXTERNAL_FUNCTIONS_FILENAME = 'externalFns.txt'
EXTERNAL_FUNCTION_MAP_FILENAME = 'externalFnMap.txt'


def get_args():
    parser = argparse.ArgumentParser(
        description='Executes 1st pass of CTU analysis where we preprocess '
                    'all files in the compilation database and generate '
                    'AST dumps and other necessary information from those '
                    'to be used later by the 2nd pass of '
                    'Cross Translation Unit analysis',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-b', required=True, dest='buildlog',
                        metavar='build.json',
                        help='JSON Compilation Database to be used')
    parser.add_argument('-p', metavar='preanalyze-dir', dest='ctuindir',
                        help='Target directory for preanalyzation data',
                        default='.ctu')
    parser.add_argument('-j', metavar='threads', dest='threads', type=int,
                        help='Number of threads to be used',
                        default=int(multiprocessing.cpu_count() * 1.0))
    parser.add_argument('-v', dest='verbose', action='store_true',
                        help='Verbose output')
    parser.add_argument('--clang-path', metavar='clang-path',
                        dest='clang_path',
                        help='Set path to directory of clang binaries used '
                             '(default taken from CLANG_PATH envvar)',
                        default=os.environ.get('CLANG_PATH'))
    mainargs = parser.parse_args()

    if mainargs.verbose:
        logging.getLogger().setLevel(logging.INFO)

    if mainargs.clang_path is None:
        clang_path = ''
    else:
        clang_path = os.path.abspath(mainargs.clang_path)
    logging.info('CTU uses clang dir: ' +
                 (clang_path if clang_path != '' else '<taken from PATH>'))

    return mainargs, clang_path


def process_buildlog(buildlog_filename, src_2_dir, src_2_cmd, src_order,
                     cmd_2_src, cmd_order):
    with open(buildlog_filename, 'r') as buildlog_file:
        buildlog = json.load(buildlog_file)
    for step in buildlog:
        if SOURCE_PATTERN.match(step['file']):
            if step['file'] not in src_2_dir:
                src_2_dir[step['file']] = step['directory']
                src_2_cmd[step['file']] = step['command']
                src_order.append(step['file'])
            if step['command'] not in cmd_2_src:
                cmd_2_src[step['command']] = [step['file']]
                cmd_order.append(step['command'])
            else:
                cmd_2_src[step['command']].append(step['file'])


def clear_file(filename):
    try:
        os.remove(filename)
    except OSError:
        pass


def clear_workspace(ctuindir):
    clear_file(os.path.join(ctuindir, DEFINED_FUNCTIONS_FILENAME))
    clear_file(os.path.join(ctuindir, EXTERNAL_FUNCTIONS_FILENAME))
    clear_file(os.path.join(ctuindir, EXTERNAL_FUNCTION_MAP_FILENAME))


def get_command_arguments(cmd):
    had_command = False
    args = []
    for arg in cmd.split():
        if had_command and not SOURCE_PATTERN.match(arg):
            args.append(arg)
        if not had_command and arg.find('=') == -1:
            had_command = True
    return args


def generate_ast(params):
    source, command, directory, clang_path, ctuindir = params
    args = get_command_arguments(command)
    arch_command = [os.path.join(clang_path, 'clang-cmdline-arch-extractor')]
    arch_command.extend(args)
    arch_command.append(source)
    arch_command_str = ' '.join(arch_command)
    logging.info(arch_command_str)
    arch_output = subprocess.check_output(arch_command_str, shell=True)
    arch = arch_output[arch_output.rfind('@')+1:].strip()
    ast_joined_path = os.path.join(ctuindir, 'ast', arch,
                                   os.path.realpath(source)[1:] + '.ast')
    ast_path = os.path.abspath(ast_joined_path)
    try:
        os.makedirs(os.path.dirname(ast_path))
    except OSError:
        if os.path.isdir(os.path.dirname(ast_path)):
            pass
        else:
            raise
    dir_command = ['cd', directory]
    ast_command = [os.path.join(clang_path, 'clang'), '-emit-ast']
    ast_command.extend(args)
    ast_command.append('-w')
    ast_command.append(source)
    ast_command.append('-o')
    ast_command.append(ast_path)
    ast_command_str = ' '.join(dir_command) + " && " + ' '.join(ast_command)
    logging.info(ast_command_str)
    subprocess.call(ast_command_str, shell=True)


def map_functions(params):
    command, sources, directory, clang_path, ctuindir = params
    args = get_command_arguments(command)
    dir_command = ['cd', directory]
    funcmap_command = [os.path.join(clang_path, 'clang-func-mapping')]
    funcmap_command.append('--ctu-dir')
    funcmap_command.append(os.path.abspath(ctuindir))
    funcmap_command.extend(sources)
    funcmap_command.append('--')
    funcmap_command.extend(args)
    funcmap_command_str = ' '.join(dir_command) + \
        " && " + ' '.join(funcmap_command)
    logging.info(funcmap_command_str)
    subprocess.call(funcmap_command_str, shell=True)


def run_parallel(threads, workfunc, funcparams):
    original_handler = signal.signal(signal.SIGINT, signal.SIG_IGN)
    workers = multiprocessing.Pool(processes=threads)
    signal.signal(signal.SIGINT, original_handler)
    try:
        # Block with timeout so that signals don't get ignored, python bug 8296
        workers.map_async(workfunc, funcparams).get(TIMEOUT)
    except KeyboardInterrupt:
        workers.terminate()
        workers.join()
        exit(1)
    else:
        workers.close()
        workers.join()


def generate_external_funcmap(mainargs):
    func_2_file = {}
    extfunc_2_file = {}
    func_2_fileset = {}
    defined_fns_filename = os.path.join(mainargs.ctuindir,
                                        DEFINED_FUNCTIONS_FILENAME)
    with open(defined_fns_filename, 'r') as defined_fns_file:
        for line in defined_fns_file:
            funcname, filename = line.strip().split(' ')
            if funcname.startswith('!'):
                funcname = funcname[1:]  # main function
            if funcname not in func_2_file.keys():
                func_2_fileset[funcname] = set([filename])
            else:
                func_2_fileset[funcname].add(filename)
            func_2_file[funcname] = filename
    extern_fns_filename = os.path.join(mainargs.ctuindir,
                                       EXTERNAL_FUNCTIONS_FILENAME)
    with open(extern_fns_filename, 'r') as extern_fns_file:
        for line in extern_fns_file:
            line = line.strip()
            if line in func_2_file and line not in extfunc_2_file:
                extfunc_2_file[line] = func_2_file[line]
    extern_fns_map_filename = os.path.join(mainargs.ctuindir,
                                           EXTERNAL_FUNCTION_MAP_FILENAME)
    with open(extern_fns_map_filename, 'w') as out_file:
        for func, fname in extfunc_2_file.items():
            if len(func_2_fileset[func]) == 1:
                out_file.write('%s %s.ast\n' % (func, fname))


def main():
    mainargs, clang_path = get_args()
    clear_workspace(mainargs.ctuindir)

    src_2_dir = {}
    src_2_cmd = {}
    src_order = []
    cmd_2_src = {}
    cmd_order = []
    process_buildlog(mainargs.buildlog, src_2_dir, src_2_cmd, src_order,
                     cmd_2_src, cmd_order)

    run_parallel(mainargs.threads, generate_ast,
                 [(src, src_2_cmd[src], src_2_dir[src], clang_path,
                   mainargs.ctuindir) for src in src_order])

    run_parallel(mainargs.threads, map_functions,
                 [(cmd, cmd_2_src[cmd], src_2_dir[cmd_2_src[cmd][0]],
                   clang_path, mainargs.ctuindir) for cmd in cmd_order])

    generate_external_funcmap(mainargs)


if __name__ == "__main__":
    main()

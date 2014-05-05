#!/usr/bin/env python

from __future__ import print_function
import re
import os
import sys
import random
import subprocess
import multiprocessing

from collections import namedtuple
ProcessParams = namedtuple("ProcessParams", "root_dir work_dir")
LaunchParams = namedtuple("LaunchParams", "exec_file exec_args")

# execve strace output looks like:
# PID Syscall_name("Syscall_cmd", ["Syscall_args_in_quotes_splitted_with_{, }"]
#   PID    Syscall name   Process name  Argument string  Environment variables
# "(\d+) ......(\w+).....\(\"([\w/\\\]+)\",... \[(.*)\], \[(.*)\]"
strace_execve_pattern = re.compile("(\d+) +(\w+)\((\"[\S]+\"), \[(.*)\], \[(.*)\]")

# Only return value (pid) is interesting for us
strace_fork_pattern = re.compile(".*?(\d+)$")

# Select string (path) in brackets
strace_chroot_chdir_pattern = re.compile("\d+ +\w+\(\"(.*)\"")

# Select pid and system call name
strace_main_pattern = re.compile("^(\d+) +([\+\w]+)")

# Select pid
strace_exit_pattern = re.compile("(\d+) +\+\+\+ exited")

# Select name, pid and return result of resumed syscall
strace_resumed_pattern = re.compile("(\d+) +<\.\.\. (.*) resumed>.*= (\d+)")

# We can set regexp for compiler detection via environment variable
compiler_regexp_str = os.environ.get('COMPILER_REGEXP',
#        "^\"(?:.*/)?(?:cc1(?:plus)?|clang)\"$")
        "^\"(?:.*/)?(?:cc|(?:\S+-)?(?:gcc|g\+\+)|clang(?:\+\+)?)(?:-[\d\.]+)?\"$")
compiler_regexp = re.compile(compiler_regexp_str)

compiler_include_regexp = re.compile("(-[IL])([\S]+)")

script_dir = os.path.dirname(__file__)

need_debug = os.environ.get('INTERCEPTOR_DEBUG')
debug_file = os.environ.get('INTERCEPTOR_DEBUG_FILE')

unprocessed_list = dict()
exec_interrupted_list = dict()

def debug_print(str) :
    if need_debug :
        if debug_file :
            with open(debug_file, 'a') as debug_out :
                debug_out.write('%s\n' % str)
        else :
            print(str)

# Android build system uses scripts with names same as compilers.
# We just ignore them and intercept binary executives only.
def is_script(path) :
    file = open(path, "r")
    line = file.readline()
    file.close()
    return line.startswith("#!")

def start_analyzer(cmd, env) :
    os.environ.update(env)
    return os.system(cmd)

analyzer_process_pool = multiprocessing.Pool(processes = int(os.environ.get('NUM_PROCESSES',
                                             multiprocessing.cpu_count())))

def add_root(path_str, root_dir) :
    if path_str.startswith("/") :
        return root_dir + "/" + path_str
    is_include = compiler_include_regexp.search(path_str)
    if is_include :
        return is_include.groups()[0] + add_root(is_include.groups()[1], root_dir)
    return path_str

def analyzer_cmd(root_dir, work_dir, exec_file, exec_args) :
    arg_list = [ ("\"" + add_root(arg[1:], root_dir)) \
        for arg in exec_args.split(", ")[1:] ]
    # Try to guess some additional includes
    if (exec_file.startswith("\"")) :
        exec_file = exec_file[1:-1]
    sysroot = os.path.dirname(os.path.dirname(exec_file)) + "/sysroot"
    if (os.path.exists(sysroot)) :
        sysroot = add_root(sysroot, root_dir)
    else :
        sysroot = root_dir
    exec_file = add_root(exec_file, root_dir)
    if is_script(exec_file) :
        return ""
    return "cd %s/%s && IS_INTERCEPTED=true COMPILER=\"%s\" %s/ccc-analyzer %s" % \
        (root_dir, work_dir, exec_file, script_dir, \
         ' '.join(arg_list + ["-g", "-I\"%s/usr/include\"" % sysroot]))

# Parsing functions
# ------------------------------------------------------------------------------
def parse_exec(pid, process_set, strace_out) :
    exec_args = strace_execve_pattern.search(strace_out).groups()[2:]
    exec_file = exec_args[0]
    if compiler_regexp.match(exec_file) :
        if strace_out.endswith("= 0\n") or strace_out.endswith("<unfinished ...>\n") :
            if pid in process_set :
                cmd = analyzer_cmd(process_set[pid].root_dir,
                                   process_set[pid].work_dir,
                                   exec_file, exec_args[1])
                env_list = [ arg[1:-1] for arg in exec_args[2].split(", ") ]
                env_dict = dict();
                for env in env_list :
                    if env.find('=') != -1 :
                        key, value = env.split("=", 1)
                        env_dict[key] = value
                debug_print('Environment: %s' % env_dict)
                debug_print('Command: %s' % cmd)
                if not cmd :
                    debug_print('Compiler cmd is a script')
                elif cmd.find('conftest.c') == -1 : # Not a 'configure'
                    analyzer_process_pool.apply_async(start_analyzer, [cmd, env_dict])
                else :
                    debug_print('conftest found, omitting analysis')
            else :
                waiting_vfork_list[pid] = LaunchParams(exec_file, exec_args[1])

def resume_exec(pid, process_set, strace_out):
    if strace_out.endswith(" = 0\n") :
        parse_exec(pid, process_set, exec_interrupted_list[pid])
    else :
        del process_set[pid] # does not need to be tracked
    del exec_interrupted_list[pid]

def parse_fork(pid, process_set, strace_out) :
    match = strace_fork_pattern.search(strace_out)
    if match :
        newpid = match.groups()[0]
        process_set[newpid] = ProcessParams(process_set[pid].root_dir,
                                            process_set[pid].work_dir)
        if newpid in unprocessed_list :
            for line in unprocessed_list[newpid] :
                parse_input_string(process_set, line)
            del unprocessed_list[newpid]

def resume_fork(pid, process_set, retval):
    process_set[retval] = ProcessParams(process_set[pid].root_dir,
                                        process_set[pid].work_dir)
    if retval in unprocessed_list :
        for line in unprocessed_list[retval] :
            parse_input_string(process_set, line)
        del unprocessed_list[retval]

# Model 'cd' cmd
def changedir(old_dir_path, new_dir_path) :
    if new_dir_path.startswith("/") :
        return new_dir_path
    return old_dir_path + "/" + new_dir_path

# Changes work_dir of calling process
def parse_chdir(pid, process_set, strace_out) :
    new_workdir = strace_chroot_chdir_pattern.search(strace_out).groups()[0]
    process_set[pid] = ProcessParams(process_set[pid].root_dir,
                                    changedir(process_set[pid].work_dir, new_workdir))

# Changes root_dir of calling process
def parse_chroot(pid, process_set, strace_out) :
    new_rootdir = strace_chroot_chdir_pattern.search(strace_out).groups()[0]
    process_set[pid] = ProcessParams(changedir(process_set[pid].root_dir,
                                              new_rootdir), process_set[pid].work_dir)

def parse_exit(pid, process_set):
    del process_set[pid]

def parse_input_string(process_set, strace_out) :
    is_resumed = strace_resumed_pattern.search(strace_out)
    is_success = strace_out.endswith("= 0\n");
    if is_resumed :     # syscall resumed
        pid, syscall, retval = is_resumed.groups()
        if pid in process_set :
            if syscall == "execve" :
                resume_exec(pid, process_set, strace_out)
            elif syscall == "clone" or syscall == "vfork" :
                # All neccessary information can be taken from resuming message
                resume_fork(pid, process_set, retval)
        else :
            if pid in unprocessed_list :
                unprocessed_list[pid] += [strace_out]
            else :
                unprocessed_list[pid] = [strace_out]
    else :
        search_res = strace_main_pattern.search(strace_out)
        if not search_res :
            return
        pid, syscall = search_res.groups()
        if pid in process_set :
            if syscall == "execve" :
                if strace_out.endswith("<unfinished ...>\n") :
                    # Keep strace string to analyze it when syscall is finished
                    exec_interrupted_list[pid] = strace_out
                elif is_success :
                    parse_exec(pid, process_set, strace_out)
            elif syscall == "clone" or syscall == "vfork" :
                parse_fork(pid, process_set, strace_out)
            elif syscall == "chdir" :
                parse_chdir(pid, process_set, strace_out)
            elif syscall == "chroot" :
                parse_chroot(pid, process_set, strace_out)
            elif syscall == "+++" :     # message like '28845 +++ exited with 0 +++'
                parse_exit(pid, process_set)
        else :
            if pid in unprocessed_list :
                unprocessed_list[pid] += [strace_out]
            else :
                unprocessed_list[pid] = [strace_out]

# ------------------------------------------------------------------------------
process_set = dict()

env_trace_file_name = os.environ.get('TRACE_FILE')
if not env_trace_file_name :
    trace_file_name = "/tmp/strace%d" % random.randint(0, sys.maxint)
    os.mkfifo(trace_file_name)
    strace_args = ["strace", "-f", "-v", "-s", "1000000", "-o", trace_file_name, "-e",
                "trace=vfork,fork,clone,execve,chdir,chroot", "-e", "signal="] +\
        sys.argv[1:]
    subprocess.Popen(strace_args)
else :
    trace_file_name = env_trace_file_name

strace_out_file = open(trace_file_name, "r")
line = strace_out_file.readline()
pid, syscall = strace_main_pattern.search(line).groups()

# 1st syscall should be execve
if syscall != "execve" :
    print("Oops...\n")
    exit(1)
process_set[pid] = ProcessParams("/", os.getcwd())

while line :
    parse_input_string(process_set, line)
    line = strace_out_file.readline()

analyzer_process_pool.close()
analyzer_process_pool.join()
strace_out_file.close()
if not env_trace_file_name :
    os.remove(trace_file_name)

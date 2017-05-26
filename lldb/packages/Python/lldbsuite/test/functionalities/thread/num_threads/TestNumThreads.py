"""
Test number of threads.
"""

from __future__ import print_function


import os
import time
import lldb
from lldbsuite.test.lldbtest import *
import lldbsuite.test.lldbutil as lldbutil


class NumberOfThreadsTestCase(TestBase):

    mydir = TestBase.compute_mydir(__file__)

    def setUp(self):
        # Call super's setUp().
        TestBase.setUp(self)
        # Find the line number to break inside main().
        self.thread3_notify_all_line = line_number('main.cpp', '// Set thread3 break point on notify_all at this line.')
        self.thread3_before_lock_line = line_number('main.cpp', '// Set thread3 break point on lock at this line.')
        self.main_method_first_wait_line = line_number('main.cpp', '// Set break for first wait in thread3 at this line.')

    def test_number_of_threads(self):
        """Test number of threads."""
        self.build()
        exe = os.path.join(os.getcwd(), "a.out")
        self.runCmd("file " + exe, CURRENT_EXECUTABLE_SET)

        # This should create a breakpoint with 1 location.
        lldbutil.run_break_set_by_file_and_line(
            self, "main.cpp", self.thread3_notify_all_line, num_expected_locations=1)

        # The breakpoint list should show 1 location.
        self.expect(
            "breakpoint list -f",
            "Breakpoint location shown correctly",
            substrs=[
                "1: file = 'main.cpp', line = %d, exact_match = 0, locations = 1" %
                self.thread3_notify_all_line])

        # Run the program.
        self.runCmd("run", RUN_SUCCEEDED)

        # Stopped once.
        self.expect("thread list", STOPPED_DUE_TO_BREAKPOINT,
                    substrs=["stop reason = breakpoint 1."])

        # Get the target process
        target = self.dbg.GetSelectedTarget()
        process = target.GetProcess()

        # Get the number of threads
        num_threads = process.GetNumThreads()

        # Using std::thread may involve extra threads, so we assert that there are
        # at least 4 rather than exactly 4.
        self.assertTrue(
            num_threads >= 13,
            'Number of expected threads and actual threads do not match.')
        
    def test_unique_stacks(self):
        """Test backtrace unique with multiple threads executing the same stack."""
        self.build()
        exe = os.path.join(os.getcwd(), "a.out")
        self.runCmd("file " + exe, CURRENT_EXECUTABLE_SET)

        # Set a break point in the main method for the first thread3 wait.
        lldbutil.run_break_set_by_file_and_line(
            self, "main.cpp", self.main_method_first_wait_line, num_expected_locations=1)

        # The breakpoint list should show 1 location.
        self.expect(
            "breakpoint list -f",
            "Breakpoint location shown correctly",
            substrs=[
                "1: file = 'main.cpp', line = %d, exact_match = 0, locations = 1" % self.main_method_first_wait_line])

        # Run the program.
        self.runCmd("run", RUN_SUCCEEDED)

        # Stopped once.
        self.expect("thread list", STOPPED_DUE_TO_BREAKPOINT,
                    substrs=["stop reason = breakpoint 1."])

        # Set a break point on the thread3 notify all (should get hit on threads 4-13).
        lldbutil.run_break_set_by_file_and_line(
            self, "main.cpp", self.thread3_before_lock_line, num_expected_locations=1)

        # Continue the program to let all threads go try to hit the break points.
        self.runCmd("continue", RUN_SUCCEEDED)

        target = self.dbg.GetSelectedTarget()
        process = target.GetProcess()

        # Get the number of threads
        num_threads = process.GetNumThreads()

        # Using std::thread may involve extra threads, so we assert that there are
        # at least 10 thread3's rather than exactly 10.
        self.assertTrue(
            num_threads >= 10,
            'Number of expected threads and actual threads do not match.')
        
        # Attempt to walk each of the thread's executing the thread3 function (4 through 13)
        # to the same breakpoint.
        for thread_index in range(4, 14):
            thread = process.GetThreadAtIndex(thread_index)
            if thread.IsValid():
                while thread.GetStopReason() != lldb.eStopReasonBreakpoint:
                    thread.StepInstruction(True)

        # Now that we are stopped, we should have 10 threads waiting in the
        # thread3 function. All of these threads should show as one stack.
        self.expect("thread backtrace unique",
                    "Backtrace with unique stack shown correctly",
                    substrs=["10 thread(s) #4 #5 #6 #7 #8 #9 #10 #11 #12 #13"])

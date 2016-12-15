#!/usr/bin/python
#
# VH Integration Test - a script for the Verlihub Python plugin
# Copyright (C) 2016-2017 Verlihub Team, info at verlihub dot net
# Created in 2016 by Frog (frogged), the_frog at wp dot pl
# License: GNU General Public License v.3, see http://www.gnu.org/licenses/
# Last update: 2016-03-03
#
# This script performs automated tests of Verlihub. It normally does nothing.
# To activate it, use the vh_run_integration_test program, which creates
# a special hub just to be used by this script. After the test is finished,
# the script will automatically shut the hub down.

from __future__ import print_function
import vh
import sys
import time
from datetime import datetime, timedelta

special_hub_name = "_vh_integration_test_"  # this is how we recognize the test hub
my_name = "VH Integration Test"  # we will try to set name and version in the test
my_version = "1.0.1-testing"
event_logger_name = "VH Event Logger"  # we will try communicating with that script
event_logger_cmd = "vh_event_log"
seconds = 0
max_run_time = 20  # we stop the hub after this many seconds hve passed no matter what

class Tester(object):
    def __init__(self):
        self.test_is_active = False
        self.start_time = 0
        self.all_tests_count = 0
        self.failed_tests_count = 0

    def test(self, message, value="____no_result", info_if_failed=""):
        if not self.test_is_active:
            return False
        if value == "____no_result":
            # Not really a test. We are just printing a message.
            head = "----"
            test_is_positive = True
        elif not value:
            head = "FAIL"
            test_is_positive = False
            self.all_tests_count += 1
            self.failed_tests_count += 1
        else:
            head = "-OK-"
            test_is_positive = True
            self.all_tests_count += 1
        added = " -- %s" % info_if_failed if info_if_failed and not test_is_positive else ""
        print("%s   %s%s" % (head, message, added))
        sys.stdout.flush()
        return test_is_positive

    def test_eq(self, message, val1, val2):
        if val1 == val2:
            return self.test(message, True)
        else:
            return self.test(message, False, "Error: '%s' != '%s'" % (val1, val2))

    def test_in(self, message, val1, val2):
        not_in = True
        try:
            if val1 in val2:
                not_in = False
        except:
            pass
        if not_in:
            return self.test(message, False, "Error: '%s' not in '%s'" % (val1, val2))
        else:
            return self.test(message, True)

    def start_testing(self, callback=None):
        if not self.test_is_active:
            self.test_is_active = True
            self.start_time = time.time()
            sys.stdout.flush()
            vh.mc("I'm running n test, and there aren't supposed to be any people online.")
            vh.mc("If this surprises you, you shouldn't have set the hub_name to %s." % special_hub_name)
            print("\n================ Starting VH Integration Test ================\n")
            sys.stdout.flush()
            callback()

    def finish_testing(self, exit_code):
        if self.test_is_active:
            now = time.time()
            if self.failed_tests_count == 0 and self.all_tests_count > 0:
                s = "All %d tests succeeded! " % self.all_tests_count
            else:
                s = "%d out of %d tests failed. " % (self.failed_tests_count, self.all_tests_count)
            s += "Testing took %s seconds" % (now - self.start_time)
            self.test(s)
            self.test_is_active = False
            print("\n================= Ending VH Integration Test =================\n")
            sys.stdout.flush()
            vh.StopHub(exit_code)



tester = Tester()

def my_test():
    global tester
    t = tester
    if not t.test_is_active:
        return

    t.test("It's now %s. Hub has been running for %s seconds" % (datetime.now(), int(time.time() - vh.starttime)))
    t.test("Script: %s (%s), Python plugin ver. %s / %s" % (vh.name, vh.path, vh.__version__, vh.__version_string__))
    
    t1 = t.test("Checking vh.botname and vh.opchatname values", vh.botname == "Verlihub" and vh.opchatname == "OpChat",
        "They are: '%s' and '%s'" % (vh.botname, vh.opchatname))
    t2 = t.test_eq("Confirming that hublist_host is empty", vh.GetConfig("config", "hublist_host"), "")
    t3 = t.test_eq("Confirming that send_crash_report is 0", vh.GetConfig("config", "send_crash_report"), "0")
    t4 = t.test_eq("Checking usercount", vh.GetUsersCount(), 0)
    if not (t1 and t2 and t3 and t4):
        t.test("Cannot continue testing with these errors", 0)
        return t.finish_testing(1)


    t.test("Trying to set vh.name_and_version", vh.name_and_version(my_name, my_version))
    x = vh.name_and_version()
    t.test("Reading and verifying vh.name_and_version", x and x[0] == my_name and x[1] == my_version, x)

    t.test("Checking nicklists...")
    x = vh.GetRawNickList()
    t.test("Checking vh.GetRawNickList", "Verlihub$$" in x and "OpChat$$" in x and x.startswith("$NickList "), x)
    x = vh.GetRawOpList()
    t.test("Checking vh.GetRawOpList", "Verlihub$$" in x and "OpChat$$" in x and x.startswith("$OpList "), x)
    x = vh.GetRawBotList()
    t.test("Checking vh.GetRawBotList", "Verlihub$$" in x and "OpChat$$" in x and x.startswith("$BotList "), x)

    x = vh.GetNickList()
    t.test("Checking vh.GetNickList", "Verlihub" in x and "OpChat" in x and isinstance(x, list), x)
    x = vh.GetOpList()
    t.test("Checking vh.GetOpList", "Verlihub" in x and "OpChat" in x and isinstance(x, list), x)
    x = vh.GetBotList()
    t.test("Checking vh.GetBotList", "Verlihub" in x and "OpChat" in x and isinstance(x, list), x)


    t.test("")
    t.test("Testing vh.ScriptQuery and communication between scripts...")
    x = vh.ScriptQuery("_get_script_file", "")
    t.test("Getting a list of scripts", x and isinstance(x, list), "List is empty. Impossible!")
    t.test_in("Confirming that we are on it", vh.path, x)
    t.test("There are %s scripts on the list" % len(x))

    x = vh.ScriptQuery("_get_script_file", "", "python")
    if t.test("Getting a list of python scripts", x and isinstance(x, list), "List is empty. Impossible!"):
        py_len = len(x)
        if t.test_in("Confirming that we are on it", vh.path, x):
            my_pos = x.index(vh.path)
            t.test("The paths are: %s" % x)

            x = vh.ScriptQuery("_get_script_name", "", "python", True)
            if t.test("Getting a list of python script names", x and isinstance(x, list), "Got '%s'" % x):
                if t.test_eq("Confirming that results in long form have two elements", len(x[0]), 2):
                    t.test("The names are: %s" % [a[0] for a in x])
                    t.test("Checking our entry", x[my_pos][0] == my_name and x[my_pos][1] == vh.path, x)

            x = vh.ScriptQuery("_get_script_version", "", "", True)
            if t.test("Getting a list of python script versions", x and isinstance(x, list), "Got '%s'" % x):
                if t.test_eq("Confirming that results in long form have two elements", len(x[0]), 2):
                    t.test("The versions are: %s" % [a[0] for a in x])
                    t.test("Checking our entry", x[my_pos][0] == my_version and x[my_pos][1] == vh.path, x)
        else:
            t.test("Aborting this branch", False)
    else:
        t.test("Aborting this branch", False)

    x = vh.ScriptQuery("_get_script_file", "", vh.path)
    t.test_eq("Asking for our path directly", x, [vh.path])
    x = vh.ScriptQuery("_get_script_name", "", vh.path)
    t.test_eq("Asking for our name directly", x, [my_name])
    x = vh.ScriptQuery("_get_script_version", "", vh.path)
    t.test_eq("Asking for our version directly", x, [my_version])


    t.test("")
    t.test("Testing vh.ScriptQuery and vh.ScriptCommand for communication with scripts...")
    scripts = vh.ScriptQuery("_get_script_name", "", "python", True)
    for entry in scripts:
        if entry[0] == event_logger_name:
            script = entry[1]
            t.test("Found %s in %s" % (event_logger_name, script))
            if t.test_eq("Turning on logging", vh.ScriptQuery(event_logger_cmd, "1", script), ["is_on"]):
                t.test("Now you should start seeing output from %s" % event_logger_name)
                t.test_eq("Confirming that logging is active", vh.ScriptQuery(event_logger_cmd, "", script), ["is_on"])
                t.test_eq("Calling logger with sh2p43g cmd", vh.ScriptQuery("sh2p43g", "data", script), [])
                t.test_eq("Calling python with jm5f03c cmd", vh.ScriptQuery("jm5f03c", "data", "python"), [])
                t.test_eq("Calling all with pf6j76q cmd", vh.ScriptQuery("pf6j76q", "data"), [])
                t.test("Using ScriptCommand('wn3f98g', 'data')", vh.ScriptCommand("wn3f98g", "data"))
                t.test_eq("Turning off logging", vh.ScriptQuery(event_logger_cmd, "0", script), ["is_off"])


    return t.finish_testing(0)



def maybe_start_testing():
    hub_name = vh.GetConfig("config", "hub_name")
    if hub_name == special_hub_name:
        ver = vh.__version__
        if ver[0] == 1 and (ver[1] > 2 or ver[1] == 2 and ver[2] > 3):
            tester.start_testing(my_test)
        else:
            print("ERROR: Cannot run VH Integration Test. Expected version <2.0.0 and >1.2.3, but got %s" % str(ver))
            sys.stdout.flush()
            vh.StopHub(1)

def OnTimer(timestamp=0):
    global seconds
    seconds += 1
    if not tester.test_is_active and seconds < 2:
        maybe_start_testing()
    elif seconds >= max_run_time:
        print("FAIL   The test maximum runtime of %d seconds was reached. Stopping the hub..." % max_run_time)
        sys.stdout.flush()
        tester.finish_testing(0)
        vh.StopHub(2)

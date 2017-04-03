#!/usr/bin/env python

# Copyright (C) 2014 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA.

import argparse
import functools
import os
import platform
import shlex
import subprocess
import sys
import threading
import time

import tester
import utilities


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, os.pardir, os.pardir))
XVFB_FILE = os.path.join(REPO_DIR, 'testing', 'xvfb.py')

LIN, OSX, WIN = ['linux', 'darwin', 'windows']
PLATFORM = platform.system().lower()

SUPPRESSIONS = (
)


class TestRunner(object):
    def __init__(self, testdirs, test_name, timeout, test_parameters):
        self.name = test_name
        self.parameters = test_parameters
        self.timeout = timeout
        self.xvfb = XVFB_FILE if PLATFORM == LIN else None
        self.testdirs = testdirs
        self.path = self.test_path()

        if self.path is None:
            utilities.log_error(
                "test {} not found\n".format(test_name),
                id_='tester script')
            sys.exit(1)

        self.proc = None
        self.timer = None
        self.killed = False

    def run_test(self):
        utilities.log(action='start header')
        utilities.log("\n{}{}\n".format(self.name, "*"*len(self.name)))
        utilities.log(action='stop header')

        if self.path.endswith('.py'):
            cmd = ['python', self.path]
        elif self.xvfb:
            cmd = [self.xvfb, os.path.dirname(self.path), self.path]
        else:
            cmd = [self.path]

        self.proc = subprocess.Popen(cmd + self.parameters, cwd=SCRIPT_DIR)
        self.timer = threading.Timer(self.timeout, self.timeout_action)
        self.timer.start()
        t0 = time.time()
        self.proc.communicate()
        self.timer.cancel()

        if not self.killed and time.time() - t0 > 0.75 * self.timeout:
            utilities.log_warning(
                "test {} ran dangerously close to timeout\n".format(self.name),
                id_="tester script")

        return not self.killed and self.proc.returncode == 0

    def timeout_action(self):
        utilties.log_error(
            "test {} timed out\n".format(self.name),
            id_="tester script")
        sys.stderr.flush()
        self.kill_test()

    def kill_test(self):
        if self.proc is not None:
            self.proc.kill()
            self.killed = True

    def test_path(self):
        out_directories = ['out', 'build']
        build_types = ['Release', 'Release_x64', 'Debug', 'Debug_x64']
        suffixes = ["", ".exe"]

        for out_directory in out_directories:
            for build_type in build_types:
                if self.testdirs:
                    directories = self.testdirs
                else:
                    directories = [os.path.join(REPO_DIR, out_directory, build_type)]

                for directory in directories:
                    if os.path.exists(directory):
                        for suffix in suffixes:
                            file_ = os.path.join(directory, self.name + suffix)

                            if os.path.exists(file_):
                                return file_
        return None


def _smoketest(args):
    success = True
    testlist = []

    if os.path.exists(args.testlist):
        with open(args.testlist, "rb") as fp:
            testlist = fp.readlines()
    else:
        utilities.log_warning("no test list found\n", id_='tester script')
        sys.exit(0)

    for line in testlist:
        line = line.strip()
        if not line or line.startswith('#'):  # skip empty lines and comments
            continue

        line = shlex.split(line)
        test_name = line[0]
        test_parameters = line[1:]

        runner = TestRunner(args.testdirs,
                            test_name,
                            args.timeout,
                            test_parameters)
        test_success = runner.run_test()

        if not test_success:
            success = False
            break

    if success:
        utilities.log("All tests passed.\n")

    sys.exit(int(not success))


def smoketest(args):
    f = functools.partial(_smoketest, args=args)
    return ("smoketest", f, {'suppressions': SUPPRESSIONS})


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--testdir", dest="testdirs", action="append",
                        help="path to the test binaries folder")
    parser.add_argument("--testlist", required=True,
                        help="path to the test list file")
    parser.add_argument("--timeout", default=180, type=int,
                        help="test timeout")

    return utilities.parse_tester_args(parser)


def main():
    args = parse_args()
    commands = [
        smoketest(args),
    ]

    cleanup_func = None
    sys.exit(tester.run(commands, args, cleanup_func))


if __name__ == "__main__":
    main()

#!/usr/bin/env python

# Copyright (C) 2014 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA.

# This file contains common code for the builder.py in this
# directory, as well as common/infra/builder.py in opera/work.git.

import argparse
import signal
import subprocess
import sys

import utilities
import builder_common


def run(commands, args, cleanup_func=None):
    """
    Run a list of commands. Aggregate the return code. args is the argument
    namespace from the project tester. If a cleanup function is specified
    it will run if the script is aborted with a SIGTERM or SIGINT.
    """
    if args.dry_run:
        utilities.print_commands(commands, verbose=args.verbose)
        sys.exit(0)

    ret = 0
    try:
        for description, command, kwargs in commands:
            skip = kwargs.pop('skip_if_earlier_error', False)
            if ret != 0 and skip:
                utilities.log_warning(
                    "* skipping '{}' because of previous "
                    "failures!\n".format(description),
                    id_='tester script')
                continue
            ret |= builder_common._run(
                description, command, buildbot_log=args.verbose, **kwargs)
    except SystemExit as e:
        if e.code == builder_common.CLEANUP_EXIT and cleanup_func:
            utilities.log_warning(
                "SIGTERM/SIGINT received, starting cleanup!\n",
                id_='tester script')
            cleanup_func()
        raise  # Just re raise and quit

    return ret


def main(testers):
    """
    Main dispatch of the program.

    Parses arguments, loads any extra arguments from the profile and
    runs the project tester in a subprocess.
    """
    usage = "tester.py <{}> [extras]".format('|'.join(testers))
    parser = argparse.ArgumentParser(
        usage=usage, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('project', metavar='project', choices=testers,
                        help="select from: {}".format(', '.join(testers)))
    parser.add_argument('--dry-run', action='store_true', default=False,
                        help="print a summary of the build steps")
    args, extras = parser.parse_known_args()

    extras.insert(0, '--verbose')
    tester = testers[args.project]
    cmd = ['python', tester] + extras

    if args.dry_run:
        cmd.append('--dry-run')

    if sys.platform == 'win32':
        p = subprocess.Popen(
            cmd, creationflags=subprocess.CREATE_NEW_PROCESS_GROUP)
        # DNA-57828: pass parent_cmd on windows to avoid using psutil.cmdline()
        #            which crashes randomly with: "WindowsError: [Error 299]
        #            Only part of ReadProcessMemory or WriteProcessmemory
        #            request was completed"
        builder_common._add_interrupt_handler(
            p.pid, signal.SIGBREAK, action='terminate', recursive=True,
            parent_cmd=cmd)
    else:
        p = subprocess.Popen(cmd)

    builder_common._add_interrupt_handler(
        p.pid, signal.SIGTERM, action='terminate', recursive=True,
        parent_cmd=cmd)
    builder_common._add_interrupt_handler(
        p.pid, signal.SIGINT, action='terminate', recursive=True,
        parent_cmd=cmd)

    utilities.wait_for_process(p)
    sys.exit(p.returncode)

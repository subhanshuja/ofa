#!/usr/bin/env python

# Copyright (C) 2014 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA.

# This file contains common code for the builder.py in this
# directory, as well as common/infra/builder.py in opera/work.git.

import argparse
import functools
import multiprocessing
import os
import re
import signal
import shlex
import subprocess
import sys
import threading

import utilities

# Use psutil (https://pythonhosted.org/psutil/) for cross platform shutdown of
# started child processes if a build job is aborted. This is crucial for not
# bogging down the build machines but not really required for other machines.
try:
    import psutil
except ImportError:
    psutil = None
    sys.stdout.write(
        "warning: psutil is not installed. To ensure shutdown of eventual "
        "build child processes if aborting the script please run 'pip "
        "install psutil'.\n")
    sys.stdout.flush()


CLEANUP_EXIT = 42


def run(commands, args, cleanup_func=None):
    """
    Run a list of commands. Aggregate the return code. args is the argument
    namespace from the project builder. If a cleanup function is specified
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
                utilities.log_warning("* skipping '{}' because of previous "
                                      "failures!\n".format(description))
                continue
            ret |= _run(description, command, buildbot_log=args.verbose,
                        **kwargs)
    except SystemExit as e:
        if e.code == CLEANUP_EXIT and cleanup_func:
            utilities.log_warning("SIGTERM/SIGINT received, starting cleanup!\n")
            cleanup_func()
        raise  # Just re raise and quit

    if args.verbose:
        utilities.print_buildbot_summary(commands, args)
    return ret


def _run_partial(command):
    """ Helper to _run(...) for executing a funtools.partial command. """
    p = _start_process(command, partial=True)
    p.join()

    return p.exitcode


def _run_command(description, command, buildbot_log, **kwargs):
    """ Helper to _run(...) for executing a Popen command. """
    suppressions = kwargs.pop('suppressions', ())
    if buildbot_log:
        kwargs['stdout'] = subprocess.PIPE
        kwargs['stderr'] = subprocess.PIPE

    p = _start_process(command, **kwargs)

    if buildbot_log:
        _add_logging_handler(p, description, suppressions)

    return p.wait()


def _run(description, command, buildbot_log=True, **kwargs):
    """
    Run a single command and tag standard output on the fly so that buildbot
    can parse the output into separate logs.

    description: a short string describing the command, e.g. compile/cleanup

    command, either of the following:
    - a string/list of strings containing commands to be run with with Popen
    - a python function wrapped in a bare functools.partial instance

    kwargs, may contain:
    - <suppressions>:  a sequence of compiled regex patterns whose matches are
        redirected into a separate log (useful for ignoring warnings)
    - additional arguments passed to Popen, e.g. <env>, <shell>, or <cwd>
    """
    if 'env' in kwargs:
        local = os.environ.copy()
        local.update(kwargs['env'])
        kwargs['env'] = local

    if buildbot_log:
        utilities.log(action='start log', id=description)

        utilities.log(action='start header', id=description)
        utilities.print_command(description, command, kwargs)
        utilities.log(action='stop header', id=description)
    else:
        utilities.print_command(description, command, kwargs, verbose=False)

    if isinstance(command, functools.partial):
        exitcode = _run_partial(command)
    else:
        exitcode = _run_command(description, command, buildbot_log, **kwargs)

    if buildbot_log:
        utilities.log(action='stop log', id=description)

    return exitcode


def _add_logging_handler(process, description, suppressions):
    """
    Parse stdout/stderr on the fly and tag certain kinds of messages.

    Threading is used to enable simultaneous parsing of stdout and stderr.
    """
    def interceptor(input, output):
        """ Read from input and write tags/messages to output. """
        for line in iter(input.readline, ''):
            tag = utilities.check_tags(line, suppressions)
            if tag:
                utilities.log(line, action=tag, id=description, file=output)
            else:
                utilities.log(line, file=output)

    threads = [
        threading.Thread(
            target=interceptor, args=(process.stdout, sys.stdout)),
        threading.Thread(
            target=interceptor, args=(process.stderr, sys.stderr)),
    ]
    for t in threads:
        t.start()
    for t in threads:
        t.join()


def _add_interrupt_handler(pid, signal_, action='kill', recursive=True,
                           returncode=None, parent_cmd=None):
    """
    Perform an action on pid and its child process(es) upon capturing signal_.

    action:     psutil.Process instance function that should be called
    recursive:  perform the action recursively on all descendants of pid
    returncode: if given exit with returncode after executing the action

    This function requires the psutil module.
    """
    if psutil is None:
        return

    if action not in ('kill', 'resume', 'suspend', 'terminate'):
        action = 'kill'

    termination = action in ['kill', 'terminate']
    parent = psutil.Process(pid)

    def call(func, **kwargs):
        """ Ignore psutil exceptions if the process is dead or unavailable. """
        try:
            return func(**kwargs)
        except (psutil.AccessDenied, psutil.NoSuchProcess):
            return None

    if parent_cmd is None:
        parent_cmd = "undefined process"

    log_prefix = '[{} pid: {}]'.format(action, parent.pid)

    if isinstance(parent_cmd, list):
        parent_cmd = ' '.join(parent_cmd)

    def handle_children(child_action, reverse=False):
        """Signal process children recursively."""
        children = call(parent.children, recursive=recursive)

        if not children:
            utilities.log('{} no child processes found to {}\n'.format(
                log_prefix, child_action), file=sys.stderr)
            return

        # reverse list to first kill youngest children, then their parents
        # https://pythonhosted.org/psutil/#psutil.Process.children
        children_list = reversed(children) if reverse else children

        for child in children_list:
            try:
                # run the requested action on each child process
                call(getattr(child, child_action))
            except Exception as e:
                # catch and log any exceptions
                utilities.log("{} {} child pid {} failed: {}\n".format(
                    log_prefix, child_action, child.pid, e), file=sys.stderr)

        utilities.log('{} signaled all children to {}\n'.format(
            log_prefix, child_action), file=sys.stderr)

    def handler(signum, frame):
        """ Cross platform signal handler to shutdown all child processes. """
        utilities.log('warning: received signum: {}\n'.format(
            signum), file=sys.stderr)
        utilities.log('warning: attempting to {} process pid {}: {}\n'.format(
            action, parent.pid, parent_cmd), file=sys.stderr)

        # work around windows issues with orphaned children
        # killing the parent on posix systems cleans up children just fine
        if sys.platform == 'win32':
            if termination:
                # suspend immediate children first so no new grandchildren are spawned
                handle_children('suspend')

            # terminate children in reverse, starting with the youngest grandchild
            handle_children(action, reverse=termination)

        try:
            # run the requested action on the parent process, log success
            call(getattr(parent, action))
            utilities.log("{} signaled parent\n".format(
                log_prefix), file=sys.stderr)
        except Exception as e:
            # catch and log any exceptions
            utilities.log("{} failed to signal parent: {}\n".format(
                log_prefix, e), file=sys.stderr)

        if returncode is not None:
            sys.exit(returncode)

    signal.signal(signal_, handler)


def _start_process(command, partial=False, **kwargs):
    """Start a process and add interrupt handlers."""
    if partial:
        # functools.partial command
        p = multiprocessing.Process(target=command)
        p.start()
        parent_cmd = "multiprocess {}: pid {}".format(p.name, p.pid)
    else:
        # subprocess command
        if sys.platform == 'win32':
            kwargs.update({'creationflags': subprocess.CREATE_NEW_PROCESS_GROUP})

        p = subprocess.Popen(command, **kwargs)
        parent_cmd = command

    if sys.platform == 'win32':
        _add_interrupt_handler(p.pid, signal.SIGBREAK, action='terminate',
                               recursive=True, parent_cmd=parent_cmd)

    _add_interrupt_handler(p.pid, signal.SIGTERM, action='terminate',
                           recursive=True, parent_cmd=parent_cmd)
    _add_interrupt_handler(p.pid, signal.SIGINT, action='terminate',
                           recursive=True, parent_cmd=parent_cmd)

    return p


def _lookup_profile(args, profile_dir):
    """
    Lookup a profile in profile_dir based on the build information.

    The profile lookup order is:
        project.profile.product.arch
        project.profile.product
        project.profile
    """
    profile_re = re.compile(
        r"{0.project}\.{0.profile}(\.{0.product}(\.{0.arch})?)?".format(args))

    for path, _, files in os.walk(profile_dir):
        for match in filter(None, map(profile_re.match, files)):
            return os.path.join(path, match.group())

    return None


def _load_profile(args, profile_dir):
    """ Parse and return profile arguments. """
    if args.profile == '-':
        return []
    path = _lookup_profile(args, profile_dir) or os.path.abspath(args.profile)

    try:
        with open(path, 'r') as f:
            text = f.read()
    except IOError:
        utilities.log_warning("error: Unable to load profile {}\n".format(path))
        sys.exit(1)

    comment_re = re.compile(r'#.*$', re.MULTILINE)
    stripped = comment_re.sub('', text)
    arguments = re.sub(r'\s+', ' ', stripped)  # Normalize whitespace
    return shlex.split(arguments)


def main(builders, profile_dir):
    """
    Main dispatch of the program.

    Parses arguments, loads any extra arguments from the profile and
    runs the project builder in a subprocess.
    """
    usage = "builder.py <{}> <profile> [extras]".format('|'.join(builders))
    parser = argparse.ArgumentParser(
        usage=usage, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('project', metavar='project', choices=builders,
                        help="select from: {}".format(', '.join(builders)))
    parser.add_argument('profile', help="a standard profile such as nightly "
                        "or stabilization,\nor a path to a custom profile")
    parser.add_argument('--arch', help=argparse.SUPPRESS)
    parser.add_argument('--product', help=argparse.SUPPRESS)
    args, extras = parser.parse_known_args()

    # Pass through arch and product to the builder script
    if args.arch:
        extras.insert(0, '--arch')
        extras.insert(1, args.arch)
    if args.product:
        extras.insert(0, '--product')
        extras.insert(1, args.product)
    extras.insert(0, '--verbose')

    builder = builders[args.project]
    profile = _load_profile(args, profile_dir)
    command = ['python', builder, '--project', args.project] + profile + extras
    p = _start_process(command)
    utilities.wait_for_process(p)
    sys.exit(p.returncode)

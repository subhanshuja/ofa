#!/usr/bin/env python

# Copyright (C) 2014 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA.

""" Common utilities used by the builder and/or the project scripts. """

import argparse
import collections
import errno
import functools
import glob
import hashlib
import json
import os
import platform
import pprint
import re
import shlex
import shutil
import subprocess
import sys
import threading
import time


verbose = False
thread_lock = threading.Lock()

# Prevent the \n -> \r\n translation when printing to the terminal on windows
if sys.platform == "win32":
    import msvcrt
    msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)
    msvcrt.setmode(sys.stderr.fileno(), os.O_BINARY)


BUILDBOT_TAG = ">>BUILDBOT>>{action}>>{id}>>\n"
# Patterns for identifying log messages
ERROR_PATTERNS = (
    re.compile(r"FAILED: .*"),  # ninja general failures
    re.compile(r".*: .*error: .*"),   # ninja compiler errors
    re.compile(r".*error [A-Z]*[0-9]*: .*"),  # msvs failures
    re.compile(r"^FAIL: "),  # Python unittests
    re.compile(r"^[FAIL] "),  # mobile JUnit tests
)
WARNING_PATTERNS = (
    re.compile(r".*\bwarning[: ].*", re.IGNORECASE),
)


def log(message=None, action=None, id=None, file=None):
    """
    Write and flush a message to a file. If file is None sys.stdout is used.
    Prefix the message with a buildbot tag if action and id are given.
    """
    if message is None:
        message = ''
    if file is None:
        file = sys.stdout

    if action and id:
        tag = BUILDBOT_TAG.format(action=action, id=id)
        if message:
            message = tag + message
        else:
            message = tag

    file.write(message)
    file.flush()


def log_warning(message, id_='builder script'):
    log('warning: ' + message, action='warnings', id=id_)


def log_error(message, id_='builder script'):
    log('error: ' + message, action='errors', id=id_)


def tag_counter(func):
    """ Decorator for counting the tags returned by check_tags(). """
    func.counter = {}
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        rv = func(*args, **kwargs)
        if rv is not None:
            with thread_lock:
                func.counter[rv] = func.counter.get(rv, 0) + 1
        return rv
    return wrapper


def safe_sleep(timeout):
    """Sleep with interrupt signal handling."""
    try:
        time.sleep(timeout)
    except IOError as e:
        if e.errno != errno.EINTR:
            raise


def wait_for_process(p):
    """Wait actively for a process to finish."""
    while True:
        safe_sleep(0.1)
        p.poll()

        if p.returncode is not None:
            break


@tag_counter
def check_tags(line, suppressions):
    """ Return a message tag if it matches any of the known patterns. """
    if any(pattern.match(line) for pattern in suppressions):
        return 'suppressions'
    elif any(pattern.match(line) for pattern in ERROR_PATTERNS):
        return 'errors'
    elif any(pattern.match(line) for pattern in WARNING_PATTERNS):
        return 'warnings'
    return None


def get_chromium_version():
    """ Read and parse the chromium version file. """
    this_dir = os.path.dirname(os.path.abspath(__file__))
    chromium_root = os.path.normpath(os.path.join(this_dir, os.pardir, os.pardir))
    path = os.path.join(chromium_root,  'chrome', 'VERSION')
    try:
        with open(path, 'r') as f:
            data = f.read()
        info = dict(row.split('=') for row in data.splitlines())
        return "{MAJOR}.{MINOR}.{BUILD}.{PATCH}".format(**info)
    except IOError:
        log_warning("could not open chromium version file {}\n".format(path))
    except ValueError:
        log_warning("could not parse chromium version file {}\n".format(path))
    except KeyError as e:
        log_warning("incomplete chromium version file {}, missing: '{}'".format(
            path, e.message))
    return "0.0.0.0"


def command_summary(desc, cmd, kwargs):
    """ Return a summary of the command. """
    if isinstance(cmd, functools.partial):
        args = ', '.join(map(repr, cmd.args))
        kw = ', '.join("{}={}".format(*kv) for kv in cmd.keywords.iteritems())
        cmd_str = "{}({}{})".format(cmd.func.func_name, args, kw)
    elif isinstance(cmd, basestring):
        cmd_str = cmd
    else:
        cmd = map(lambda k: '"{}"'.format(k) if ' ' in k else k, cmd)
        cmd_str = ' '.join(cmd)
    return {
        'command': cmd_str,
        'description': desc,
        'environment': kwargs.get('env', {}),
        'workdir': kwargs.get('cwd'),
    }


def package_summary(package_dir):
    """ Return a package: checksum dictionary. """
    if not os.path.exists(package_dir):
        log_warning("package directory does not exist! ({})\n".format(package_dir))
        return {}

    packages = {}
    for file in os.listdir(package_dir):
        file_path = os.path.join(package_dir, file)
        if os.path.isfile(file_path):
            packages[file] = sha1_hash(file_path)

    if not packages:
        log_warning("package directory is empty!\n")
    return packages


def print_command(desc, cmd, kwargs, verbose=True):
    """ Pretty print a single command """
    summary = command_summary(desc, cmd, kwargs)
    if not verbose:
        log("# {command}\n".format(**summary))
        return
    log("#"*80 + "\n")
    log("Command: {description}\n".format(**summary))
    log("Invocation: {command}\n".format(**summary))
    log("Work dir: {workdir}\n".format(**summary))
    log("Environment:\n{}\n".format(pprint.pformat(summary['environment'])))


def print_commands(commands, verbose=True):
    """ Pretty print the list of commands. """
    for desc, cmd, kwargs in commands:
        print_command(desc, cmd, kwargs, verbose)


def print_buildbot_summary(commands, args):
    """ Print a summary to be parsed into buildbot properties. """
    properties = [
        ('version_chromium', get_chromium_version()),
        ('target', ' '.join(args.targets)),
        ('tests', ' '.join(args.tests)),
        ('tag_counts', check_tags.counter),
    ]

    if hasattr(args, 'release_channel'):
        properties.append(('release_channel', args.release_channel))

    if 'smoke' in args.tests:
        properties.append(('smoketest_cmd', args.smoketest_cmd))

    if args.package and args.package_dir:
        properties.append(('package_source_dir', args.package_dir))
        properties.append(('package_files', package_summary(args.package_dir)))

    for name, data in properties:
        log(json.dumps(data) + "\n", action='save property', id=name)


def print_buildbot_test_summary(commands, args):
    """ Print a summary to be parsed into buildbot properties. """
    properties = [
        ('tag_counts', check_tags.counter),
    ]

    for name, data in properties:
        log(json.dumps(data) + "\n", action='save property', id=name)


def parse_args(parser, parser_defaults=None):
    """ Add common arguments to the builder's parser and parse inputs. """
    class ListAppend(argparse.Action):
        """ Append the incoming values to the destination """
        def __call__(self, parser, namespace, values, option_string=None):
            params = getattr(namespace, self.dest) or []
            params += values
            setattr(namespace, self.dest, params)

    class DictUpdate(argparse.Action):
        """ Update the destination dict with the incoming one """
        def __call__(self, parser, namespace, values, option_string=None):
            params = getattr(namespace, self.dest) or {}
            params.update(values)
            setattr(namespace, self.dest, params)

    def shlex_split_type(value):
        """ Split the input string using shell-like syntax. """
        return shlex.split(value)

    def gyp_param_type(value):
        """ Split input string into dict with expanded param names as keys"""
        params = {}
        gyp_lonely_flag_re = re.compile("^-\w$")

        def _add_param(param):
            param_key, param_value = param.split('=', 1)
            params.setdefault(param_key, param_value)

        for param in shlex.split(value):
            if gyp_lonely_flag_re.match(param):
                raise argparse.ArgumentTypeError(
                    "Don't support lonely options like '{}'. "
                    "Instead of '-I myoption', write '-Imyoption'."
                    .format(param))
            elif not param.startswith('-'):
                # Unqualified parameters are assumed to be defines
                _add_param('-D' + param)
            elif '=' in param:
                _add_param(param)
            else:
                params.setdefault(param, None)
        return params

    def gn_arg_type(value):
        """ Split input string into dict with expanded param names as keys"""
        params = {}

        def _add_param(param):
            param_key, param_value = param.split('=', 1)
            params.setdefault(param_key, param_value)

        for param in shlex.split(value):
            if '=' in param:
                _add_param(param)
            else:
                params.setdefault(param, None)
        return params

    if parser_defaults is None:
        parser_defaults = {}

    parser.set_defaults(env=parser_defaults.get('env', {}), package_dir=None)

    parser.add_argument('--no-ccache', dest='ccache', action='store_false',
                        default=True, help="don't use compiler cache")
    parser.add_argument('--no-clcache', dest='clcache', action='store_false',
                        default=True, help="don't use compiler cache (windows)")
    parser.add_argument('--no-pkg', dest='package', action='store_false',
                        default=True, help="don't build packages")
    parser.add_argument('--debug', dest='debug', action='store_true',
                        default=False, help="build with debug flags")
    parser.add_argument('--dry-run', action='store_true', default=False,
                        help="print a summary of the build steps")
    parser.add_argument('--use-gn', dest='use_gn', action='store_true',
                        default=False, help="use gn to generate ninja files")
    parser.add_argument('--use-gyp', dest='use_gyp', action='store_true',
                        default=False, help="use gyp to generate ninja files")
    parser.add_argument('--verbose', action='store_true', default=False,
                        help="print out log data that buildbot expects")
    parser.add_argument('--ubn', type=int, default=0,
                        help="unique build number")

    extras = parser.add_argument_group(title='general arguments')
    extras.add_argument('--project', type=str, default=parser_defaults['project'],
                        help=argparse.SUPPRESS)
    extras.add_argument('--targets', type=shlex_split_type, metavar='...',
                        default=parser_defaults.get('targets', []))
    extras.add_argument('--tests', type=shlex_split_type, metavar='...',
                        help="example: 'goth'",
                        default=parser_defaults.get('tests', []))
    extras.add_argument('--smoketest_cmd', type=shlex_split_type, metavar='...',
                        default=parser_defaults.get('smoketest_cmd', []),
                        help=argparse.SUPPRESS),
    extras.add_argument('--compile_params', type=shlex_split_type, metavar='...',
                        help="example: '--custom-flag'",
                        default=parser_defaults.get('compile_params', []))
    extras.add_argument('--gyp_params', type=gyp_param_type, metavar='...',
                        help="example: 'fastbuild=1 werror=0'",
                        default=parser_defaults.get('gyp_params', {}))
    extras.add_argument('--gn_args', type=gn_arg_type, metavar='...',
                        help="example: 'is_debug=false symbol_level=1'",
                        default=parser_defaults.get('gn_args', {}))
    extras.add_argument('--env_params', type=shlex_split_type, metavar='...',
                        help="example: 'FOO=BAR BAZ=FOO'",
                        default=parser_defaults.get('env_params', []))
    extras.add_argument('--add_compile_params', type=shlex_split_type, action=ListAppend,
                        metavar='...', dest="compile_params",
                        help=argparse.SUPPRESS, default=[])
    extras.add_argument('--add_gyp_params', type=gyp_param_type, action=DictUpdate,
                        metavar='...', dest="gyp_params",
                        help=argparse.SUPPRESS, default={})
    extras.add_argument('--add_env_params', type=shlex_split_type, action=ListAppend,
                        metavar='...', dest="env_params",
                        help=argparse.SUPPRESS, default=[])

    args, unknowns = parser.parse_known_args()

    if unknowns:
        log_warning("Received unknown arguments: {}\n".format(' '.join(unknowns)))

    args.build_type = 'Debug' if args.debug else 'Release'

    # Use a common internal cache flag for ccache and clcache
    if platform.system().lower() == 'windows':
        args.cache = args.clcache
    else:
        args.cache = args.ccache

    try:
        custom_env = dict(var.split('=', 1) for var in args.env_params)
    except ValueError:
        log_warning("Unable to parse custom environment parameters {}\n".format(
            ' '.join(args.env_params)))
    else:
        args.env.update(custom_env)

    return args


def parse_tester_args(parser, parser_defaults=None):
    if parser_defaults is None:
        parser_defaults = {}

    parser.add_argument('--dry-run', action='store_true', default=False,
                        help="print a summary of the build steps")
    parser.add_argument('--verbose', action='store_true', default=False,
                        help="print out log data that buildbot expects")

    args, unknowns = parser.parse_known_args()

    if unknowns:
        log_warning(
            "Received unknown arguments: {}\n".format(' '.join(unknowns)),
            id_='tester script')

    return args


def sha1_hash(file, blocksize=65536):
    """
    Return the sha1 hash of a file. The file is read in chunks of blocksize
    to improve performance/lessen memory footprint.
    """
    hasher = hashlib.sha1()
    with open(file, 'rb') as f:
        for chunk in iter(lambda: f.read(blocksize), b''):
            hasher.update(chunk)
    return hasher.hexdigest()


# Common builder commands

def ccache_setup(dir=None, size='10G'):
    """
    Return the ccache setup command.

    If dir is given, we are running with build-script managed
    cache directories.
    """
    env = {}
    if dir:
        env['CCACHE_DIR'] = dir
        cmd = "mkdir -p {!r} && ccache -M {} && ccache -z".format(dir, size)
    else:
        cmd = "ccache -z"
    return ("ccache setup", cmd, {'env': env, 'shell': True})


def ccache_stats(dir=None):
    """ Return the ccache stats command. """
    env = {"CCACHE_DIR": dir} if dir else {}
    cmd = ["ccache", "-s"]
    return ("ccache stats", cmd, {'env': env})


def _save_ubn(filename=None, data="", append=False):
    if not filename:
        raise ValueError("missing file argument")

    if not isinstance(data, basestring):
        raise ValueError("data is not a valid string")

    with open(filename, '{}b'.format('a' if append else 'w')) as fp:
        fp.write(data)


def save_ubn(ubn, repo_dir):
    """ Save the unique build number file. """
    filename = os.path.join(repo_dir, 'common', 'UBN')
    f = functools.partial(_save_ubn, filename=filename, data=str(ubn))
    return ("save ubn", f, {})


def clean_directory(path):
    """ Remove a directory and its contents. """
    f = functools.partial(shutil.rmtree, path=path, ignore_errors=True)
    desc = 'remove directory {}'.format(os.path.basename(path))
    return (desc, f, {})


def rename(src, dst, desc=None):
    """ Rename a file. """
    f = functools.partial(shutil.move, src=src, dst=dst)
    if not desc:
        desc = "rename {} to {}".format(os.path.basename(src), os.path.basename(dst))
    return (desc, f, {})


def script_command(description, script_path):
    """ Common script command. """
    return (description, script_path, {})


class Icecc(object):
    """
    Utilities for icecc.

    Public API:
    properties:
    - NotFoundError: exception class used for raising exceptions in this
                     utility class
    - AvailablityNotChanged: exception class used for flow control when
                             changing prefix dir
    - available: indicates whether or not icecc is available
    - prefix_dir: a path to the base folder of the icecc installation
    - binary: path to the 'icecc' executable
    - create_env: path to 'icecc-create-env' executable
    - version: icecc version as reported by 'icecc --version'

    methods:
    - update_prefix_dir: updates prefix dir and executable paths; re-checks
                         icecc availability
    - make_tarball: runs icecc-create-env to make the env tarball, returns
                    True on success and False on failure.

      args:
      - tarball_dir: folder in which to make the env tarball
      - create_env_params: parameter string for the icecc-create-env
                           executable
    """
    class NotFoundError(Exception):
        def __init__(self, tag, path):
            super(Icecc.NotFoundError, self).__init__("Cannot find %s at: %s\n" % (tag, path))


    class AvailabilityNotChanged(Exception):
        pass


    gyp_params = {
        '-Dlinux_use_debug_fission': '0',
        '-Dlinux_use_bundled_binutils': '0',
        '-Dclang_use_chrome_plugins': '0',
    }

    gn_args = {
        'use_debug_fission': 'false',
        'linux_use_bundled_binutils': 'false',
        'clang_use_chrome_plugins': 'false',
    }

    env = {
        'ICECC_VERSION': None,  # set in make_tarball
        'ICECC_CLANG_REMOTE_CPP': '1',
    }

    def __init__(self):
        self.available = False
        self.prefix_dir = None
        self.update_prefix_dir(os.getenv('ICECC_PREFIX', None))

    def update_prefix_dir(self, prefix_dir=None):
        log('Checking for icecc... ')
        error = ""

        try:
            if not prefix_dir or prefix_dir == self.prefix_dir:
                raise self.AvailabilityNotChanged()

            self.available = False
            self.prefix_dir = prefix_dir
            self.binary = os.path.join(self.prefix_dir, 'bin', 'icecc')
            self.create_env = os.path.join(self.prefix_dir, 'bin', 'icecc-create-env')

            if not os.path.exists(self.prefix_dir):
                raise self.NotFoundError("icecc prefix dir", self.prefix_dir)

            if not os.path.exists(self.binary):
                raise self.NotFoundError("icecc binary", self.binary)

            if not os.path.exists(self.create_env):
                raise self.NotFoundError("icecc-create-env", self.create_env)

            self.version = subprocess.check_output([self.binary, '--version']).strip()
            self.available = True
        except self.NotFoundError as e:
            error += e.message
        except self.AvailabilityNotChanged:
            pass
        except Exception as e:
            error += "Error when checking for icecc availablity:\n"
            error += e.message

        if self.available:
            log('found version: %s\n' % self.version)
        else:
            log('not found on this system.\n')

        if error:
            log_warning(error)

    def run_gyp_hooks(self, chromium_dir, script_args=None):
        # BUILDBOT-1016: we need to update clang *before* we make the tarball
        if script_args and not isinstance(script_args, (list, tuple)):
            script_args = [script_args]
        else:
            script_args = []

        cmd = [
            "python",
            os.path.join(chromium_dir, 'build', 'run_opera_hooks.py'),
        ]
        cmd.extend(script_args)

        try:
            log("- updating gyp hooks...\n")
            subprocess.check_output(cmd)
            return True
        except subprocess.CalledProcessError as e:
            log_warning("failed updating gyp hooks\n")
            log_warning("- cmd: {}\n".format(cmd))
            log_warning("- exit code: {}\n".format(e.returncode))
            log_warning("- output:\n{}\n".format(e.output))
            # can't use icecc knowing the tarball might not be using
            # the right compiler version
            self.available = False
            return False

    def make_tarball(self, tarball_dir, create_env_params):
        log("- making icecc tarball in {} ...\n".format(tarball_dir))
        # ensure icecc tarball foleder exists and is empty
        if os.path.exists(tarball_dir):
            shutil.rmtree(tarball_dir)  # purge folder if present

        os.makedirs(tarball_dir)  # create new folder

        # create icecc tarball
        cmd = [self.create_env] + shlex.split(create_env_params)

        try:
            subprocess.check_output(cmd, cwd=tarball_dir)  # make tarball
            tarball_file = os.listdir(tarball_dir)[0]  # get tarball name
            self.env['ICECC_VERSION'] = os.path.join(tarball_dir, tarball_file)
            log("done (tarball: {})\n".format(tarball_file))
            return True
        except subprocess.CalledProcessError as e:
            log_warning("failed creating icecc env tarball\n")
            log_warning("- cmd: {}\n".format(cmd))
            log_warning("- exit code: {}\n".format(e.returncode))
            log_warning("- output:\n{}\n".format(e.output))
        except IndexError:
            log_warning("cannot locate the icecc env tarball\n")

        return False


class Xcode(object):
    """
    Utilities for Xcode

    This utility class should only be used on Mac OS X. It uses the
    'xcode-select' and 'xcodebuild' commands to manipulate currently
    selected Xcode application path and fetch available Xcode SDKs.

    Public API:
    properties:
    - XcodeError: exception class used for raising exceptions in this
                  utility class

    - default_path: fetch from an env var on the computer running this
                    code or set to the default Xcode location in OS X.

    - current_path: determined during initialization, pointing to the
                    currently selected Xcode application path; used to
                    determine if path switching is required and as a
                    switch back path if it is.

    - available_sdks: a map of system wide available SDKs to a list of
                      Xcode application paths in which these SDKs are
                      provided

    methods:
    - switch_commands: returns a tuple of switch commands to be used
                       as infra_build sub-commands if path switching
                       is required to satisfy the selected SDK
                       requirement, otherwise returns (None, None)
      args:
      - sdk: the SDK requested for usage

    Private API:
    methods:
    - _get_available_paths: scans the system for installed Xcode
                            applications and returns a list of paths
                            for each installation

    - _get_available_sdks: fetches a list of available SDKs for the
                           currently selected Xcode application path

    - _get_current_path: returns the currently selected Xcode path in
                         the system, if an error is encoutered returns
                         DEFAULT_PATH
      kwargs:
        silent: (default False) is set to true suppresses logging

    - _select: switches current Xcode path in the system, if an error
               is encoutered an XcodeError is raised
      args:
      - path: path which the Xcode application path in the system
              should be switched to

      kwargs:
      - silent: (default False) is set to true suppresses logging

    - _get_path_for_sdk: get the latest/oldest Xcode application path
                         that provides the requested SDK, return None
                         if the request SDK is not found in
                         AVAILABLE_SDKS
      args:
      - sdk: the SDK to look up the path for

      kwargs:
      - latest: (default False) if set to true try to return the most
                recent Xcode version path, otherwise return the
                oldest one
    """
    class XcodeError(Exception):
        pass


    _instance = None

    def __new__(cls, *args, **kwargs):
        if not cls._instance:
            cls._instance = super(Xcode, cls).__new__(cls, *args, **kwargs)

        return cls._instance

    def __init__(self):
        """
        Initialize the AVAILABLE_SDKS map. Scan all installed Xcode
        application paths fetching available SDKs for each path.
        Create a mapping of available SDKs to a list of Xcode
        application paths by which they are provided.
        """
        self.default_path = os.getenv('BUILD_TOOLCHAIN_XCODE_DEFAULT',
                                      '/Applications/Xcode.app')
        self.current_path = None
        self.available_sdks = {}
        self.current_path = self._get_current_path()

        for path in self._get_available_paths():
            self._select(path, silent=True)

            for sdk in self._get_available_sdks():
                if not sdk in self.available_sdks:
                    self.available_sdks[sdk] = []

                self.available_sdks[sdk].append(path)

    # Public API:
    def switch_commands(self, sdk):
        """
        Generates a tuple of command triplets that are used as
        infra_build sub-commands if path switching is necessary to
        provide the requested SDK. Otherwise returns (None, None).
        """
        switch_path = self._get_path_for_sdk(sdk)

        if switch_path is None or switch_path == self.current_path:
            return (None, None)

        switch = (
            "xcode-select",
            functools.partial(self._select, path=switch_path),
            {}
        )
        switch_back = (
            "xcode-select-reset",
            functools.partial(self._select, path=self.current_path),
            {}
        )

        return (switch, switch_back)

    # Private API:
    @staticmethod
    def _get_available_paths():
        """Get a list of all installed Xcode application paths."""
        available_paths = []

        for app in glob.glob('/Applications/Xcode*.app'):
            path = '{}/Contents/Developer'.format(app)
            available_paths.append(path)

        return available_paths

    @staticmethod
    def _get_available_sdks():
        """
        Get a list of all available SDKs in the currently selected
        XCode path.
        """
        try:
            cmd = ["xcodebuild", "-showsdks"]
            blob = subprocess.check_output(cmd)
            current_sdks = re.findall('-sdk (?P<sdk>[a-z]+[0-9]+\.[0-9]+)',
                                      blob, re.M)
        except (subprocess.CalledProcessError, ValueError):
            log_warning("error fetching xcode SDKs for current path\n")
            current_sdks = []

        return current_sdks

    def _get_current_path(self, silent=False):
        """Get the currently selected Xcode application path."""
        path = None

        try:
            cmd = ['xcode-select', '--print-path']
            path = subprocess.check_output(cmd).strip()

            if not silent:
                log("current xcode path: '{}'\n".format(path))
        except (subprocess.CalledProcessError,):
            path = self.default_path

            if not silent:
                msg = ("failed reading current xcode path, using "
                       "default: '{}'\n".format(path))
                log_warning(msg)

        return path

    def _select(self, path, silent=False):
        """Switch Xcode application path to a selected installation."""
        if path == self._get_current_path(silent=silent):
            return

        try:
            cmd = ["sudo", "-n", "xcode-select", "--switch", path]
            subprocess.check_call(cmd)

            if not silent:
                log("switched xcode path to: '{}'\n".format(path))
        except (subprocess.CalledProcessError,):
            if not silent:
                msg = "failed switching xcode path to: {}\n".format(path)
                log_warning(msg)
                raise self.XcodeError(msg)

    def _get_path_for_sdk(self, sdk, latest=False):
        """
        Get the latest/oldest path that provides the requested sdk.
        """
        paths = sorted([path for path in self.available_sdks.get(sdk, [])])

        if not paths:
            msg = "no xcode paths found for the requested sdk: '{}'".format(sdk)
            log_warning(msg)
            return None

        # Usually the paths will be sorted by version like this:
        # 0: Xcode.app (the latest xcode installed)
        # 1: Xcode1.1.1.app (oldest xcode installed)
        # ...
        # X: XcodeX.X.X.app (second to the latest xcode installed)
        #
        # This may not be the case on developer machines. Since we
        # currently don't care about the actual Xcode version we use,
        # only the SDKs it provides this is fine. If we ever do start
        # caring about which version we select this needs to be
        # extended to fetch Xcode versions from the system.
        if latest or len(paths) == 1:
            return paths[0]  # return the latest (or only) path
        else:
            return paths[1]  # return the oldest path


class Sysroot(object):
    """
    Utilities for sysroots.

    Public API:
    properties:
    - NotFoundError: exception class used for raising exceptions
                            in this utility class
    - argparse_help: a string for arparse argument help generated based on
                     availablity of sysroots
    - available: indicates whether or not there are any available sysroots
    - available_distributions: a map of available sysroot distributions
                               to the list of available architectures
    - available_architectures: a map of available sysroot architectures
                               to the list of available distributions
    - basepath: a path to the base folder containing all sysroots fetched
                from an env var

    methods:
    - get_path: generates a path to a sysroot based on requested distribution
                and architecture and verifies that the desired sysroot exists

      args:
      - distro: the distribution of the requested sysroot
      - arch: the architecture of the requested sysroot
    """
    class NotFoundError(Exception):
        def __init__(self, path):
            msg = "Cannot find the requested sysroot path: {}".format(path)
            super(Sysroot.NotFoundError, self).__init__(msg)


    available = False

    def __init__(self, basepath):
        """
        Scan the configured sysroot base folder for available sysroots and
        initialize available distribution and architecture maps.
        """
        self.basepath = basepath
        self.available_distributions = collections.defaultdict(list)
        self.available_architectures = collections.defaultdict(list)

        if not self.basepath or not os.path.isdir(self.basepath):
            log("sysroots are not available on this system\n")
            return

        sysroots = os.listdir(self.basepath)
        valid_sysroots = 0

        for sysroot in sysroots:
            if not os.path.isdir(os.path.join(self.basepath, sysroot)):
                continue

            try:
                distro, arch = sysroot.rsplit('-', 1)
            except ValueError:
                log_warning(
                    "found an invalid sysroot folder '{}' in: {}\n".format(
                    sysroot, self.basepath))
                continue

            valid_sysroots += 1
            self.available_distributions[distro].append(arch)
            self.available_architectures[arch].append(distro)

        if self.available_distributions:
            log("{} sysroots are available under: {}\n".format(
                valid_sysroots, self.basepath))
            self.available = True  # use only when there is anything to use
        else:
            log_warning("no sysroots are available under: {}\n".format(
                self.basepath))

    def get_path(self, distro, arch):
        """
        Return a path to the requested distribution and architecture sysroot.
        Raise an exception if no such sysroot is found.
        """
        path = os.path.join(self.basepath, "{}-{}".format(distro, arch))

        if not os.path.isdir(path):
            raise self.NotFoundError(path)

        return path

    @property
    def argparse_help(self):
        """
        Return an argparse argument help string generated based on whether
        or not there are available sysroots in the system.
        """
        if self.available:
            bits = []

            for arch, distros in self.available_architectures.iteritems():
                bits.append('{} ({})'.format(arch, ', '.join(distros)))

            help_ = ('Available distributions per architecture: {}'.format(
                     '; '.join(bits)))
        else:
            help_ = ("Set environment variable BUILD_TOOLCHAIN_SYSROOTS to "
                     "a folder containing sysroots named '<distro>-<arch>/'")

        return help_

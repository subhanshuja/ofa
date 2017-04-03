#!/usr/bin/env python

# Copyright (C) 2014 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA.

"""Compile and build operadriver.

usage:
    infra_build.py --help
"""

import argparse
import os
import platform
import subprocess
import sys
import time

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import builder
import utilities


ALL_ARCHS = ['mipsel', 'arm', 'ia32', 'x64']
ALL_PRODUCTS = ['operadriver']
LIN, OSX, WIN = ['linux', 'darwin', 'windows']
PLATFORM = platform.system().lower()
CCACHE_SIZE = "10G"

CHROMIUM_DIR = builder.REPO_DIR
PACKAGE_DIR = os.path.join(CHROMIUM_DIR, 'chrome_staging')
OUTPUT_DIR = os.path.join(CHROMIUM_DIR, 'out', '{build_type}')
OUTPUT_DIR_ANDROID = os.path.join(OUTPUT_DIR, 'apks')

EXE = ".exe" if PLATFORM == WIN else ""
OPERADRIVER_ORIGINAL_NAME = "operadriver%s" % EXE
OPERADRIVER_PACKAGE = "operadriver2_server-{version}-{timestamp}-{build_type}%s" % EXE
TIMESTAMP = time.strftime("%Y.%m.%d_%H.%M.%S", time.gmtime())  # UTC
CLANG_VERSION = ''

# NOTE: These defaults are used by all chromium profiles
DEFAULT_TARGETS = {
    'operadriver' : ['operadriver']
}

if PLATFORM in (LIN, OSX):
    # Retrieve version of clang in this tree, used in ccache hashing.
    clang_version_script = os.path.join(CHROMIUM_DIR, 'tools', 'clang', 'scripts', 'update.sh')
    CLANG_VERSION = subprocess.check_output([clang_version_script, '--print-revision']).strip()

DEFAULT_ENV = {
    'CCACHE_COMPILERCHECK': 'echo -n ' + CLANG_VERSION,
    'CCACHE_COMPRESS': '1',
    'CCACHE_CPP2': 'yes',
    'CCACHE_SLOPPINESS': 'time_macros,include_file_mtime,file_macro',
    'GOMA_CLCACHE_PROJECT_ROOT_DIR': builder.REPO_DIR,
}

ENV_KEYS = {
    LIN: ['CCACHE_COMPILERCHECK', 'CCACHE_COMPRESS', 'CCACHE_CPP2',
          'CCACHE_SLOPPINESS'],
    OSX: ['CCACHE_COMPILERCHECK', 'CCACHE_COMPRESS', 'CCACHE_SLOPPINESS'],
    WIN: ['GOMA_CLCACHE_PROJECT_ROOT_DIR'],
}

# Strip keys irrelevant for the platform
for key in DEFAULT_ENV.keys():
    if key not in ENV_KEYS[PLATFORM]:
        del DEFAULT_ENV[key]

PARSER_DEFAULTS = {
    'env': DEFAULT_ENV,
    'project': 'operadriver',
    'tests': ['smoke'],
    'smoketest_cmd': [
        'python',
        builder.TESTER_PATH,
        'chromium',
        '--testlist', 'opera.smoketests',
    ],
}

SMOKETEST_DISABLED_PRODUCT = '{platform}.{product}.{arch}.{build_type}'
SMOKETEST_DISABLED_PRODUCTS = (
    # <PLATFORM>.<product>.<arch>.<build_type>
    # NOTE: on some platforms <build_type> might by suffixed with '_x64'
    'darwin.operadriver.ia32.Release',
    'darwin.operadriver.ia32.Debug',
    'darwin.operadriver.x64.Release',
    'darwin.operadriver.x64.Debug',
    'linux.operadriver.ia32.Release',
    'linux.operadriver.ia32.Debug',
    'linux.operadriver.x64.Release',
    'linux.operadriver.x64.Debug',
    'windows.operadriver.ia32.Release',
    'windows.operadriver.ia32.Debug',
    'windows.operadriver.x64.Release_x64',
    'windows.operadriver.x64.Debug_x64',
)


if PLATFORM == LIN:
    sysroot = utilities.Sysroot(os.getenv('BUILD_TOOLCHAIN_SYSROOTS'))
    DEFAULT_SYSROOT_DISTRO = 'wheezy'


if PLATFORM == OSX:
    xcode = utilities.Xcode()
    DEFAULT_XCODE_SDK = "macosx10.8"


def gyp(args):
    """ GYP command. """
    cmd = []
    script = os.path.join("build", "gyp_chromium")
    cmd.extend(["python", script, "--depth=."])
    for k, v in args.gyp_params.items():
        if v is None:
            cmd.append(k)
        else:
            cmd.append('%s=%s' % (k, v))
    return ("gyp", cmd, {'env': args.env, 'cwd': CHROMIUM_DIR})


def build_command(args):
    """ Return the build command for Chromium builds. """
    ninja = "clcache_ninja" if PLATFORM == WIN and args.cache else "ninja"

    cmd = []
    cmd.extend([ninja, "-k0"])
    cmd.extend(args.compile_params)
    cmd.extend(["-C", OUTPUT_DIR.format(build_type=args.build_type)])
    cmd.extend(args.targets)
    return ("compile", cmd, {'env': args.env, 'cwd': CHROMIUM_DIR})


def zip_packages(args):
    """ Return the packaging command for Chromium builds. """
    cmd = [
        "python",
        os.path.join('build', 'infra', 'opera_zip_build.py'),
        "--target=%s" % args.build_type,
        "--build-dir=out",
        "--build-version=%s" % utilities.get_chromium_version(),
        "--build-datetime=%s" % TIMESTAMP,
    ]
    return ("zip packages", cmd, {'cwd': CHROMIUM_DIR})


def set_defaults(args):
    """ Set extra parameter values based on argument inputs. """
    if PLATFORM == WIN and os.getenv('GOMA_CLCACHE_DISABLED') == '1':
        args.cache = False

    if args.cache:
        if os.getenv('BUILD_CCACHE_DIR'):
            ccache_dir = os.getenv('BUILD_CCACHE_DIR')
            args.env['CCACHE_DIR'] = ccache_dir.format(**args.__dict__)

    if not args.targets:
        args.targets = DEFAULT_TARGETS[args.product]

    if ('chromium_builder_tests' in args.targets and
            args.product == 'operadriver'):
        args.targets.remove('chromium_builder_tests')  # incompatible

    if args.product == 'operadriver':
        operadriver_defines = {
            '-Ddriver_name': 'OperaDriver',
            '-Dbrowser_name': 'Opera',
            '-Dbrowser_options_name': 'operaOptions',
            '-Dminimum_supported_chromium_version': '{39,0,2171,13}',
            '-Ddriver_version_file': 'test/operadriver/VERSION',
            '-Dcustomized_functions_dir':
                'test/operadriver/opera/customized_functions',
            '-Dandroid_device_socket': 'com.opera.browser.devtools',
            '-Dandroid_cmd_line_file':
                '/data/local/tmp/opera-browser-command-line',
        }
        for k, v in operadriver_defines.items():
            args.gyp_params.setdefault(k, v)

    if PLATFORM == LIN:
        if sysroot.available:
            args.gyp_params.setdefault(
                '-Dsysroot',
                sysroot.get_path(args.sysroot_distro, args.arch))

        if args.cache:
            args.gyp_params.setdefault('-Dclang', '1')  # TODO(sadrabinski): investigate why this is needed, DNA-37195
            clang_dir = os.path.join(CHROMIUM_DIR, 'third_party', 'llvm-build',
                                     'Release+Asserts', 'bin')
            args.env['CC'] = 'ccache %s' % os.path.join(clang_dir, 'clang')
            args.env['CXX'] = 'ccache %s' % os.path.join(clang_dir, 'clang++')

        if os.getenv('BUILD_TOOLCHAIN_DESKTOP_PATH'):
            toolchain_path = os.getenv('BUILD_TOOLCHAIN_DESKTOP_PATH').format(**args.__dict__)
            current_path = args.env.get('PATH', os.getenv('PATH'))
            args.env['PATH'] = os.path.pathsep.join([toolchain_path, current_path])

    if PLATFORM == OSX:
        if args.arch == 'x64':
            args.gyp_params.setdefault('-Dhost_arch', 'x64')

    if PLATFORM == WIN:
        args.targets = ["%s.exe" % t if t != 'chromium_builder_tests' else t
                        for t in args.targets]
        args.env['GYP_MSVS_VERSION'] = "2013"

        # http://www.chromium.org/developers/design-documents/64-bit-support
        if args.arch == 'x64':
            args.build_type += '_x64'

        if args.cache:
            if args.debug:
                args.gyp_params.setdefault('-Dwin_z7', '1')
            else:
                args.gyp_params.setdefault('-Dfastbuild', '1')

            goma_dir = os.getenv('GOMA_CLCACHE_DIR')
            if goma_dir:
                args.gyp_params.setdefault('-Duse_goma', '1')
                args.gyp_params.setdefault('-Dgomadir', goma_dir)

    if args.debug:
        args.gyp_params.setdefault('-Dcomponent', 'shared_library')

    args.gyp_params.setdefault('-Dtarget_arch', args.arch)
    args.gyp_params.setdefault('-Gconfig', args.build_type)

    args.package_dir = PACKAGE_DIR

    args.smoketest_cmd.extend(['--testdir', OUTPUT_DIR.format(build_type=args.build_type)])

    if ('smoke' in args.tests and
        SMOKETEST_DISABLED_PRODUCT.format(platform=PLATFORM, **args.__dict__) in
        SMOKETEST_DISABLED_PRODUCTS):
        args.tests.remove('smoke')

    if 'smoke' not in args.tests:
        # 'chromium_builder_tests' target fails compilation on all chromium products
        for target in ['chromium_builder_tests', 'xdisplaycheck']:
            if target in args.targets:
                args.targets.remove(target)


def parse_args():
    """ Add common arguments to the sub builder's parser and parse inputs. """
    parser = argparse.ArgumentParser()

    group = parser.add_argument_group(title="chromium arguments")
    group.add_argument('--product', metavar="product",
                       choices=ALL_PRODUCTS, required=True,
                       help="Select from: %s" % ', '.join(ALL_PRODUCTS))
    group.add_argument('--arch', metavar="arch",
                       choices=ALL_ARCHS, required=True,
                       help="Select from: %s" % ', '.join(ALL_ARCHS))

    if PLATFORM == OSX:
        group.add_argument(
            '--sdk', metavar="sdk", default=DEFAULT_XCODE_SDK,
            choices = xcode.available_sdks.keys(),
            help="Select from: %s" % ', '.join(xcode.available_sdks.keys()))

    if PLATFORM == LIN:
        group.add_argument(
            '--sysroot-distro', metavar="sysroot_distro", required=False,
            choices=sysroot.available_distributions.keys(),
            default=DEFAULT_SYSROOT_DISTRO,
            help=sysroot.argparse_help)

    return utilities.parse_args(parser, PARSER_DEFAULTS)


if __name__ == '__main__':
    args = parse_args()
    set_defaults(args)

    commands = [
        gyp(args),
        build_command(args),
    ]

    if args.package:
        def _rename(args, out_dir, package, original_name):
            out_dir = out_dir.format(build_type=args.build_type)
            pkg_name = package.format(
                version=utilities.get_chromium_version(),
                timestamp=TIMESTAMP, build_type=args.build_type)
            src = os.path.join(out_dir, original_name)
            dst = os.path.join(PACKAGE_DIR, pkg_name)

            if not os.path.exists(PACKAGE_DIR):
                os.makedirs(PACKAGE_DIR)

            return utilities.rename(src, dst)

        if args.product == 'operadriver':
            # Rename the generic operadriver package name to something more specific
            commands.append(_rename(args, OUTPUT_DIR, OPERADRIVER_PACKAGE,
                                    OPERADRIVER_ORIGINAL_NAME))
        else:
            utilities._log_warning("unknown product: %s, cannot deterime packaging strategy"
                                    % args.product)

    if args.cache and PLATFORM in (OSX, LIN):
        ccache_dir = args.env.get('CCACHE_DIR')
        commands.insert(0, utilities.ccache_setup(ccache_dir, CCACHE_SIZE))
        commands.append(utilities.ccache_stats(ccache_dir))

    cleanup_func = None

    # Any commands that rely on using the SDK requested in args.sdk have to be added
    # to the 'commands' list before this block
    if PLATFORM == OSX:
        switch_command, reset_command = xcode.switch_commands(args.sdk)

        if switch_command:
            commands.insert(0, switch_command)

        if reset_command:
            commands.append(reset_command)
            cleanup_func = reset_command[1] if reset_command else None

    sys.exit(builder.run(commands, args, cleanup_func))


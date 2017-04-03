#!/usr/bin/env python

# Copyright (C) 2014 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA.

"""Compile and build chromium products.

This build script was created to streamline and provide a uniform interface
for Opera's build system. While the intended main user for this script is
buildbot (or any other future build system) developer use is very much
encouraged to keep this script updated with the latest build procedures.

usage:
    infra_build.py --help
"""

import argparse
import os
import platform
import subprocess
import sys
import time

import builder
import utilities


ALL_ARCHS = ['mipsel', 'arm', 'ia32', 'x64']
ALL_PRODUCTS = ['android', 'chromedriver', 'chromium']
LIN, OSX, WIN = ['linux', 'darwin', 'windows']
PLATFORM = platform.system().lower()
CCACHE_SIZE = "10G"

CHROMIUM_DIR = builder.REPO_DIR
PACKAGE_DIR = os.path.join(CHROMIUM_DIR, 'chrome_staging')
OUTPUT_DIR = os.path.join(CHROMIUM_DIR, 'out', '{build_type}')
OUTPUT_DIR_ANDROID = os.path.join(OUTPUT_DIR, 'apks')

ANDROID_ORIGINAL_NAME = "ContentShell.apk"
ANDROID_PACKAGE = "content_shell-{version}-{timestamp}-{build_type}.apk"
EXE = ".exe" if PLATFORM == WIN else ""
CHROMEDRIVER_ORIGINAL_NAME = "chromedriver%s" % EXE
CHROMEDRIVER_PACKAGE = "chromedriver2_server-{version}-{timestamp}-{build_type}%s" % EXE
TIMESTAMP = time.strftime("%Y.%m.%d_%H.%M.%S", time.gmtime())  # UTC
CLANG_VERSION = ''
CLANG_DIR = os.path.join(CHROMIUM_DIR, 'third_party', 'llvm-build', 'Release+Asserts', 'bin')

ICECC_TARBALL_DIR = os.path.join(builder.REPO_DIR, 'icecc-tarball')
ICECC_CREATE_ENV_PARAMS = "--clang {}".format(os.path.join(CLANG_DIR, 'clang'))
ICECC_THREADS = os.getenv('BUILD_ICECC_THREADS', 50)  # set to 0 to disable icecc usage by default

# NOTE: These defaults are used by all chromium profiles
DEFAULT_TARGETS = {
    'android': ['content_shell_apk', 'chromium_builder_tests'],
    'chromedriver': ['chromedriver'],
    'chromium': ['chrome', 'content_shell', 'chromium_builder_tests'],
}

if PLATFORM in (LIN, OSX):
    # Retrieve version of clang in this tree, used in ccache hashing.
    clang_version_script = os.path.join(CHROMIUM_DIR, 'tools', 'clang', 'scripts', 'update.py')
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
    'project': 'chromium',
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
    'darwin.chromedriver.ia32.Release',
    'darwin.chromedriver.ia32.Debug',
    'darwin.chromedriver.x64.Release',
    'darwin.chromedriver.x64.Debug',
    'darwin.chromium.ia32.Release',
    'darwin.chromium.ia32.Debug',
    'darwin.chromium.x64.Release',
    'darwin.chromium.x64.Debug',
    'linux.android.mipsel.Release',
    'linux.android.mipsel.Debug',
    'linux.android.ia32.Release',
    'linux.android.ia32.Debug',
    'linux.android.arm.Release',
    'linux.android.arm.Debug',
    'linux.chromedriver.ia32.Release',
    'linux.chromedriver.ia32.Debug',
    'linux.chromedriver.x64.Release',
    'linux.chromedriver.x64.Debug',
    'linux.chromium.ia32.Release',
    'linux.chromium.ia32.Debug',
    'linux.chromium.x64.Release',
    'linux.chromium.x64.Debug',
    'windows.chromedriver.ia32.Release',
    'windows.chromedriver.ia32.Debug',
    'windows.chromedriver.x64.Release_x64',
    'windows.chromedriver.x64.Debug_x64',
    'windows.chromium.ia32.Release',
    'windows.chromium.ia32.Debug',
    'windows.chromium.x64.Release_x64',
    'windows.chromium.x64.Debug_x64',
)


if PLATFORM == OSX:
    xcode = utilities.Xcode()
    DEFAULT_XCODE_SDK = "macosx10.8"


if PLATFORM == LIN:
    sysroot = utilities.Sysroot(os.getenv('BUILD_TOOLCHAIN_SYSROOTS'))
    DEFAULT_SYSROOT_DISTRO = 'wheezy'
    icecc = utilities.Icecc()


def gn(args):
    """ gn command. """
    cmd = []
    script = os.path.join('build', 'infra', 'gn_gen.py')
    cmd.extend(['python', script, 'out/%s' % args.build_type])
    for k, v in args.gn_args.items():
        cmd.append('%s=%s' % (k, v))
    return ("gn", cmd, {'env': args.env, 'cwd': CHROMIUM_DIR})


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
            args.product == 'chromedriver'):
        args.targets.remove('chromium_builder_tests')  # incompatible

    if args.product == 'android':
        args.gyp_params.setdefault('-DOS', 'android')
        args.gyp_params.setdefault('-Dwerror', '')

    if PLATFORM == OSX:
        if args.arch == 'x64':
            args.gyp_params.setdefault('-Dhost_arch', 'x64')

    if PLATFORM == LIN:
        if args.product == 'chromium' and 'smoke' in args.tests:
            args.targets.append('xdisplaycheck')

        if sysroot.available:
            sysroot_path = sysroot.get_path(args.sysroot_distro, args.arch)
            args.gyp_params.setdefault('-Duse_sysroot', '1')
            args.gyp_params.setdefault('-Dsysroot', sysroot_path)
            args.gn_args.setdefault('use_sysroot', 'true')
            args.gn_args.setdefault('target_sysroot', '"%s"' % sysroot_path)

        if args.cache:
            args.gyp_params.setdefault('-Dclang', '1')  # TODO(sadrabinski): investigate why this is needed, DNA-37195
            args.env['CC'] = 'ccache %s' % os.path.join(CLANG_DIR, 'clang')
            args.env['CXX'] = 'ccache %s' % os.path.join(CLANG_DIR, 'clang++')
            args.gn_args.setdefault('cc_wrapper', '"ccache"')

        if (args.icecc_threads > 0 and
            icecc.available and
            icecc.run_gyp_hooks(CHROMIUM_DIR) and
            icecc.make_tarball(ICECC_TARBALL_DIR, ICECC_CREATE_ENV_PARAMS)):
            args.env.update(icecc.env)
            args.gyp_params.update(icecc.gyp_params)
            args.gn_args.update(icecc.gn_args)
            args.compile_params.extend(['-j', str(args.icecc_threads)])  # give icecc something to do

            if args.cache:
                args.env['CCACHE_PREFIX'] = icecc.binary
            else:
                args.env['CC_wrapper'] = icecc.binary
                args.env['CXX_wrapper'] = icecc.binary

        if os.getenv('BUILD_TOOLCHAIN_DESKTOP_PATH'):
            toolchain_path = os.getenv('BUILD_TOOLCHAIN_DESKTOP_PATH').format(**args.__dict__)
            current_path = args.env.get('PATH', os.getenv('PATH'))
            args.env['PATH'] = os.path.pathsep.join([toolchain_path, current_path])

    if PLATFORM == WIN:
        args.targets = ["%s.exe" % t if t != 'chromium_builder_tests' else t
                        for t in args.targets]
        args.env['GYP_MSVS_VERSION'] = "2015"

        # http://www.chromium.org/developers/design-documents/64-bit-support
        if args.arch == 'x64':
            args.build_type += '_x64'

        if args.cache:
            if args.debug:
                if args.product != 'chromium':
                    args.gyp_params.setdefault('-Dwin_z7', '1')
            else:
                args.gyp_params.setdefault('-Dfastbuild', '1')
                args.gn_args.setdefault('symbol_level', '1')

            goma_dir = os.getenv('GOMA_CLCACHE_DIR')
            if goma_dir:
                args.gyp_params.setdefault('-Duse_goma', '1')
                args.gyp_params.setdefault('-Dgomadir', goma_dir)
                args.gn_args.setdefault('use_goma', 'true')
                args.gn_args.setdefault('goma_dir', '"%s"' % goma_dir)

    if args.debug:
        args.gyp_params.setdefault('-Dcomponent', 'shared_library')
        args.gn_args.setdefault('is_debug', 'true')
        args.gn_args.setdefault('is_component_build', 'true')
    else:
        args.gn_args.setdefault('is_debug', 'false')

    args.gyp_params.setdefault('-Dtarget_arch', args.arch)
    if args.arch == 'ia32':
        args.gn_args.setdefault('target_cpu', '"x86"')
    else:
        args.gn_args.setdefault('target_cpu', '"%s"' % args.arch)

    args.gn_args.setdefault('enable_nacl', 'false')

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
            choices=xcode.available_sdks.keys(),
            help="Select from: %s" % ', '.join(xcode.available_sdks.keys()))

    if PLATFORM == LIN:
        group.add_argument(
            '--sysroot-distro', metavar="sysroot_distro", required=False,
            choices=sysroot.available_distributions.keys(),
            default=DEFAULT_SYSROOT_DISTRO,
            help=sysroot.argparse_help)

        group.add_argument(
            '--icecc-threads', '-i', metavar="icecc_threads",
            required=False, default=ICECC_THREADS, type=int,
            help="Number of compiler threads to use with icecc. Set to 0 to disable icecc.")

    return utilities.parse_args(parser, PARSER_DEFAULTS)


if __name__ == '__main__':
    args = parse_args()
    set_defaults(args)

    commands = [
        gn(args) if args.use_gn else gyp(args),
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

        if args.product == 'chromium':
            commands.append(zip_packages(args))
        elif args.product == 'android':
            # Rename the generic android package name to something more specific
            commands.append(_rename(args, OUTPUT_DIR_ANDROID, ANDROID_PACKAGE,
                                    ANDROID_ORIGINAL_NAME))
        elif args.product == 'chromedriver':
            # Rename the generic chromedriver package name to something more specific
            commands.append(_rename(args, OUTPUT_DIR, CHROMEDRIVER_PACKAGE,
                                    CHROMEDRIVER_ORIGINAL_NAME))
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

#!/usr/bin/python

# Copyright (C) 2016 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA.

import os

from argparse import ArgumentParser
from glob import glob
from os import path
from shutil import move
from subprocess import check_call, check_output
from sys import exit

parser = ArgumentParser(description = 'Create compiler tarballs for icecc.')
parser.add_argument('--icecc-dir', required = False, help = 'Path to custom icecc installation.')
parser.add_argument('--git-dir', required = True, help = 'Path to .git dir in toolchain repo.')
parser.add_argument('--version', required = True, help = 'Toolchain version.')
parser.add_argument('--triplet', required = True, help = 'Target triplet.')
parser.add_argument('--prefix', required = True, help = 'Prefix for cross gcc, including path.')
parser.add_argument('--output', required = True, help = 'Destination file for ICECC_VERSION value.')
args = parser.parse_args()

def create_tarball(tarball_path, tarball_pattern, command):
    """Given a directory, a pattern to use for the target filename and a command,
    generate a tarball and return the full path. The pattern may contain the
    substring {} where a unique token is optionally inserted. If one or more files
    matches the pattern already exists, the first match is returned and no new
    tarball is created.
    """
    matches = glob(path.join(tarball_path, tarball_pattern.format('*')))
    if (matches):
        return matches[0]

    print 'Creating icecc compiler tarball matching', tarball_pattern.format('*')

    # icecc-create-env writes the created file name to file descriptor 5
    (read_fd, write_fd) = os.pipe()
    os.dup2(write_fd, 5)

    null = open(os.devnull, 'w')

    check_call(command, stdout = null, stderr = null)

    unique_name = os.read(read_fd, 128).strip()

    if not path.exists(tarball_path):
        os.makedirs(tarball_path)

    tarball_name = path.join(tarball_path, tarball_pattern.format(unique_name.split('.')[0]))
    move(unique_name, tarball_name)
    return tarball_name

if args.icecc_dir:
    icecc = path.join(args.icecc_dir, 'bin', 'icecc')
    found = False

    for d in [ 'bin', 'libexec/icecc', 'lib/icecc', 'lib64/icecc' ]:
        icecc_create_env = path.join(args.icecc_dir, d, 'icecc-create-env')
        if path.exists(icecc_create_env):
            found = True
            break

    if not found:
        print('icecc-create-env not found in %s' % args.icecc_dir)
        exit(1)
else:
    icecc = 'icecc'
    icecc_create_env = 'icecc-create-env'

tarball_path = path.join(path.expanduser('~'), '.local', 'share', 'icecc')

# Build target tarball
revision = check_output(['git', '--git-dir', args.git_dir, 'rev-parse', 'HEAD']).strip()
target_tarball_pattern = 'gcc-' + args.version + '-' + args.triplet + '-' + revision + '.tar.gz'

cc = args.prefix + 'gcc'
cxx = args.prefix + 'g++'
target_tarball_name = create_tarball(tarball_path, target_tarball_pattern, [icecc_create_env, '--gcc', cc, cxx])

# Build host tarball
# Uses icecc --build-native which avoids picking up compiler wrappers like ccache.
host_tarball_name = create_tarball(tarball_path, 'gcc-host-{}.tar.gz', [icecc, '--build-native'])

icecc_version = host_tarball_name + ','
icecc_version += target_tarball_name + '=' + args.triplet

f = open(args.output, 'w')
f.write(icecc_version)

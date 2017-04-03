#!/usr/bin/env python

import argparse
import json
import os
import sys

# Builds made on buildbot will get all its version number variables from
# version_file, see BUILDBOT-25 and WAM-1676. Builds on the Bream portal, and
# developer builds, will have to resort to some predefined default values.
#
# NOTE: If you change this, please make sure that buildReportProblemUrl() in
# OperaSettingsFragment.java handles the format correctly!

CHROME_VERSION_FILE = 'chromium/src/chrome/VERSION'
COMMON_VERSION_FILE = 'common/VERSION'
OPERA_VERSION_FILE = 'mobile/mobile/VERSION'


def chrome_version(source_root):
    values = {}
    with open(os.path.join(source_root, CHROME_VERSION_FILE), 'r') as f:
        for line in f.readlines():
            key, val = line.rstrip('\r\n').split('=', 1)
            values[key] = int(val)
    return values


def opera_version(source_root):
    with open(os.path.join(source_root, OPERA_VERSION_FILE), 'r') as f:
        values = json.load(f)

    chrome_major_version = chrome_version(source_root)['MAJOR']

    values['major'] = values['major_offset'] + chrome_major_version
    del values['major_offset']
    return values


def common_version(source_root):
    with open(os.path.join(source_root, COMMON_VERSION_FILE), 'r') as f:
        return json.load(f)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source_root', metavar='SOURCE_ROOT',
                        help='Path to the opera/work repository')
    parser.add_argument('build_number', type=int,
                        metavar='BUILDNUMBER')
    args = parser.parse_args()

    version = opera_version(args.source_root)
    nightly = common_version(args.source_root)['nightly']

    default_patch = args.build_number
    patch = version.get('patch', default_patch)

    print("%s.%s.%s.%s" % (version['major'], version['minor'], nightly, patch))

if __name__ == '__main__':
    main()

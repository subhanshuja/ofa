#!/usr/bin/env python

# Copyright (C) 2014 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA.

"""Build chromium products.

usage:
    builder.py <chromium> <profile> [extras]

The script requires two inputs, a builder project category (chromium) and
a so called profile. Any extra arguments are passed as is to the chromium
builder script. The profile argument may be ignored by passing '-' instead.

Profiles should define the arguments needed for making certain build types
such as nightly or continuous, and the profile syntax is simply the list of
arguments you would pass to the project builder script.

The profile argument accepts a path to a profile or a name of a pre-defined
profile such as nightly or stabilization. In the latter case the script uses
the project category and, if applicable, also the given product/arch to
further specify/identify the matching profile.

Example:
    $ python builder.py chromium nightly --product chromium --arch x64

    Will look for a profile under profiles/* that matches the regex
    project\.profile(\.product(\.arch)?)?

    Which in the above case translates to, ordered by priority:
    chromium.nightly.chromium.x64
    chromium.nightly.chromium
    chromium.nightly

Example:
    $ python builder.py chromium profiles/README --product android --arch x64

    Where:

    $ cat profiles/README
    # This is a builder profile. It lists any static or extra
    # parameters that should be passed to the project specific
    # builder script when building a certain type of build.

    # Example:
    --no-ccache  # Inline comments works too
    --gyp_params "fastbuild=1 werror=1"
    --env_params "FOO=BAR"
    $

    Will invoke the chromium builder script with the following arguments:
    --product android --arch x64 --no-ccache --gyp_params "fastbuild=1 werror=1" --env_params "FOO=BAR"

"""
import os

from builder_common import main, run


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROFILE_DIR = os.path.join(SCRIPT_DIR, 'profiles')
REPO_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, os.pardir, os.pardir))
TESTER_PATH = os.path.join(SCRIPT_DIR, 'tester.py')

BUILDERS = {
    'chromium': os.path.join(SCRIPT_DIR, 'infra_build.py'),
    'operadriver': os.path.join(SCRIPT_DIR, 'operadriver/infra_build.py')
}


if __name__ == '__main__':
    main(BUILDERS, PROFILE_DIR)

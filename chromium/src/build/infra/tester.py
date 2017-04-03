#!/usr/bin/env python

# Copyright (C) 2014 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA.

"""Smoke test chromium products.

usage:
    tester.py <chromium> [extras]

The script requires a single input: the project category (chromium). Any extra
arguments are passed as is to the chromium testing script.

Example:
    $ python tester.py chromium --testlist opera.smoketests
    $ python tester.py chromium --testlist opera.smoketests --timeout 60
"""
import os
import re

from tester_common import main, run


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, os.pardir, os.pardir))

TESTERS = {
    'chromium': os.path.join(SCRIPT_DIR, 'infra_smoke.py'),
}


if __name__ == '__main__':
    main(TESTERS)

#!/usr/bin/python

# Copyright (C) 2016 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA.

from argparse import ArgumentParser
from os import path
from subprocess import check_call

parser = ArgumentParser(description = 'Minify JavaScript file.')

parser.add_argument('--chromium', required = True,  help = 'Chromium source tree.')
parser.add_argument('--input', required = True, help = 'Input file name.')
parser.add_argument("--output", required = True, help = 'Output file name.')

args = parser.parse_args()

with open(args.output, 'w') as output:
    check_call(['java', '-jar', path.join(args.chromium, 'third_party', 'closure_compiler', 'compiler', 'compiler.jar'),
    '-W', 'QUIET', args.input ], stdout = output)

#!/usr/bin/env python2

"""Expand the about_version.html template with the HEAD commit hash."""

import argparse
import cgi
import sys
import os

from os.path import abspath, dirname, exists, join
from subprocess import check_call, check_output

sys.path.append(abspath(join(dirname(__file__), os.pardir, "tools")))

from common import paths


def wam_root_hash():
    git_dir = join(paths.repo_root, ".git")
    if not exists(git_dir):
        return 'Source-release'
    return check_output(
            ['git', '--git-dir', git_dir, 'rev-parse', 'HEAD']).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__,
            formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--input", required = True,
                        help = "Location of about_version.html.jinja2")
    parser.add_argument("--output", required = True,
                        help = "Destination for about_version.html")
    args = parser.parse_args()

    assert exists(args.input)

    out_dir = dirname(args.output)
    if not exists(out_dir):
        os.makedirs(out_dir)

    expand_template_py = join(paths.wam_root, "build", "expand_template.py")
    call_params = " ".join([
        expand_template_py,
        "--input", args.input,
        "--output", args.output,
        "wam_root_hash=" + wam_root_hash()
    ])

    check_call(call_params, shell = True)

if __name__ == '__main__':
    main()

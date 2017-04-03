#!/usr/bin/env python

# Copyright (C) 2015 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA.

"""Expands a single Jinja template based on command line variable assignment

This module uses doctest unit tests. To run the tests for this module from the
command line, run
  $ python -m doctest <module>
"""


import argparse
import copy
import os
import sys
import textwrap

CHR_ROOT = os.path.join(os.path.dirname(__file__), '../../../chromium/src')
sys.path.append(os.path.join(CHR_ROOT, 'third_party'))
import jinja2  # pylint: disable=F0401


# Options as required by jinja2.Environment
JINJA2_OPTIONS = {
    'trim_blocks': True,
    'lstrip_blocks': True,
    'keep_trailing_newline': True,
    'undefined': jinja2.StrictUndefined
}


def format_c_family(value):
    """Formats python values to c style formatting"""
    if type(value) is bool:
        return "true" if value else "false"
    if type(value) in [int, float]:
        return str(value)
    if type(value) is str:
        return '"%s"' % value

    assert False, "Unknown value type: '%s'" % type(value).__name__


def interpret_variable(string):
    """Interprets values and assign appropriate types, defaulting to strings

    >>> interpret_variable("YES")
    True
    >>> interpret_variable("false")
    False
    >>> interpret_variable('"True"')
    'True'
    >>> interpret_variable('42')
    42
    >>> type(interpret_variable('3.1415'))
    <type 'float'>
    """

    truthy = ['yes', 'true']
    falsy = ['no', 'false']

    lowered = string.lower()
    if lowered in truthy or lowered in falsy:
        return lowered in truthy

    if string.isdigit():
        return int(string)

    try:
        return float(string)
    except ValueError:
        return string.strip('""')


def variable(string):
    """Produces a key-value object representing a variable assignment

    >>> variable("apa=bepa")
    {'apa': 'bepa'}
    >>> variable("apa.bepa=cepa")
    {'apa.bepa': 'cepa'}
    >>> variable("apa")
    Traceback (most recent call last):
        ...
    ArgumentTypeError: Invalid variable assignment: "apa"
    >>> variable("apa=YES")
    {'apa': True}
    """
    if '=' not in string:
        message_format = 'Invalid variable assignment: "%s"'
        raise argparse.ArgumentTypeError(message_format % string)

    key, value = string.split('=', 1)
    return {key: interpret_variable(value)}


def join_variable_dicts(variable_dicts):
    """Joins single variable dictionaies to a single object

    >>> variables = [{'apa': 'bepa'}, {'cepa': 'depa'}]
    >>> # Dictionary order isn't guaranteed, let's sort the test output
    >>> test = lambda v: sorted(join_variable_dicts(v).items())
    >>> test(variables)
    [('apa', 'bepa'), ('cepa', 'depa')]
    >>> test(variables + [{'cepa': 'epa'}])
    [('apa', 'bepa'), ('cepa', 'epa')]
    >>> test([])
    []
    """
    result = {}
    for d in variable_dicts:
        result.update(d)
    return result


def merge_dict(a, b):
    """Merges two dicts together into a new dict

    >>> merge_dict({'a': 1, 'b': {'x': 9}}, {'b': {'c': 3}})
    {'a': 1, 'b': {'x': 9, 'c': 3}}
    >>> merge_dict({'a': 1}, {'a': None})
    Traceback (most recent call last):
        ...
    AssertionError: Merge conflict
    >>> a = {'a': {'b': {'c': 'd'}}}
    >>> merge_dict(a, {'a': {'b': {'e': 'f'}}})
    {'a': {'b': {'c': 'd', 'e': 'f'}}}
    >>> a # should be unmodified
    {'a': {'b': {'c': 'd'}}}
    """
    def merge(a, b):
        for k in b:
            if k in a:
                if isinstance(a[k], dict) and isinstance(b[k], dict):
                    merge(a[k], b[k])
                else:
                    assert a[k] == b[k], "Merge conflict"
            else:
                a[k] = b[k]

        return a

    return merge(copy.deepcopy(a), b)


def apply_namespacing(variables):
    """Converts dotted variable names in a dict to an object hierarchy tree

    >>> variables = {'apa.bepa': 'trazan', 'apa.cepa': 'banarne'}
    >>> expectation = {'apa': {'bepa': 'trazan', 'cepa': 'banarne'}}
    >>> apply_namespacing(variables) == expectation
    True
    >>> variables = {'apa.bepa.cepa': 'trazan', 'apa.cepa': 'banarne'}
    >>> expectation = {'apa': {'bepa': {'cepa': 'trazan'}, 'cepa': 'banarne'}}
    >>> apply_namespacing(variables) == expectation
    True
    """
    # Builds a dictionary tree branch from a value and a list of keys
    branch = lambda val, ks: val if not ks else {ks[0]: branch(val, ks[1:])}
    branches = [branch(v, k.split('.')) for k, v in variables.iteritems()]

    return reduce(merge_dict, branches)


def main():
    parser = argparse.ArgumentParser(
            description=textwrap.dedent(sys.modules[__name__].__doc__),
            formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--input', required=True, dest='template_path',
                        help='Input file')
    parser.add_argument('--output', required=True, help='Output file')
    parser.add_argument('variables', metavar='key=value', type=variable,
                        nargs='+', help="""Template variable declaration.
                        Variable identifiers may be 'dotted', which will create
                        python dictionaries. apa.bepa=cepa will yield the result
                        in the template context as {'apa': {'bepa': 'cepa'}}
                        """)
    args = parser.parse_args()

    templates_dir = os.path.dirname(args.template_path)
    environment = jinja2.Environment(
            loader=jinja2.FileSystemLoader(templates_dir),
            **JINJA2_OPTIONS)
    environment.filters['c_format'] = format_c_family

    template = environment.get_template(os.path.basename(args.template_path))

    variables = join_variable_dicts(args.variables)
    assert len(variables) == len(args.variables), "Variable overridden"

    new = template.render(apply_namespacing(variables))

    if os.path.exists(args.output):
        with open(args.output, 'r') as f:
            old = f.read()
    else:
        old = None

    if new != old:
        print('Updating ' + args.output)
        with open(args.output, 'w') as f:
            f.write(new)

if __name__ == '__main__':
    main()

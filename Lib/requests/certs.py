#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
requests.certs
~~~~~~~~~~~~~~

This module returns the preferred default CA certificate bundle. There is
only one — the one from the certifi package.

If you are packaging Requests, e.g., for a Linux distribution or a managed
environment, you can change the definition of where() to return a separately
packaged CA bundle.
"""
from certifi import where
# Import ca_cert_data for single binarisation.
try:
    from certifi import ca_cert_data
except ImportError:
    pass

if __name__ == '__main__':
    print(where())

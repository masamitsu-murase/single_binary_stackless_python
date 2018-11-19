#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
certifi.py
~~~~~~~~~~

This module returns the installation location of cacert.pem.
"""
import os
import warnings


class DeprecatedBundleWarning(DeprecationWarning):
    """
    The weak security bundle is being deprecated. Please bother your service
    provider to get them to stop using cross-signed roots.
    """


def where():
    f = os.path.dirname(__file__)

    return os.path.join(f, 'cacert.pem')


def old_where():
    warnings.warn(
        "The weak security bundle has been removed. certifi.old_where() is now an alias "
        "of certifi.where(). Please update your code to use certifi.where() instead. "
        "certifi.old_where() will be removed in 2018.",
        DeprecatedBundleWarning
    )
    return where()


_ca_cert_data = None
def ca_cert_data():
    global _ca_cert_data
    if _ca_cert_data is None:
        _ca_cert_data = __loader__.get_data(where()).decode("utf-8")
    return _ca_cert_data


if __name__ == '__main__':
    print(where())

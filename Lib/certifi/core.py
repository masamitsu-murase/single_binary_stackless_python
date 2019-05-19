# -*- coding: utf-8 -*-

"""
certifi.py
~~~~~~~~~~

This module returns the installation location of cacert.pem.
"""
import os


def where():
    f = os.path.dirname(__file__)

    return os.path.join(f, 'cacert.pem')


_ca_cert_data = None
def ca_cert_data():
    global _ca_cert_data
    if _ca_cert_data is None:
        _ca_cert_data = __loader__.get_data(where()).decode("utf-8")
    return _ca_cert_data

# coding: utf-8


__version__ = "10"

__included_module_info = {
    "comtypes": {
        "author": "Thomas Heller",
        "license": "MIT License",
        "module_name": "comtypes"
    },
    "pycodestyle": {
        "author": "Johann C",
        "license": "MIT (Expat) License",
        "module_name": "pycodestyle"
    },
    "pyflakes": {
        "author": "",
        "license": "MIT License",
        "module_name": "pyflakes"
    },
    "pyreadline": {
        "author": "Jorgen Stenarson",
        "license": "BSD License",
        "module_name": "pyreadline"
    },
    "PyYAML": {
        "author": "Kirill Simonov",
        "license": "MIT License",
        "module_name": "yaml"
    },
    "certifi": {
        "author": "Kenneth Reitz",
        "license": "MPL",
        "module_name": "certifi"
    },
    "idna": {
        "author": "Kim Davies",
        "license": "BSD License",
        "module_name": "idna"
    },
    "requests": {
        "author": "Kenneth Reitz",
        "license": "Apache Software License",
        "module_name": "requests"
    },
    "urllib3": {
        "author": "Andrey Petrov",
        "license": "MIT License",
        "module_name": "urllib3"
    },
    "cchardet": {
        "author": "PyYoshi",
        "license": "GPL and MPL",
        "module_name": "cchardet"
    },
    "six": {
        "author": "Benjamin Peterson",
        "license": "MIT License",
        "module_name": "six"
    },
    "jinja2": {
        "author": "Armin Ronacher",
        "license": "BSD License",
        "module_name": "jinja2"
    },
    "markupsafe": {
        "author": "Armin Ronacher",
        "license": "BSD License",
        "module_name": "markupsafe"
    },

    "exepy": {
        "author": "Masamitsu MURASE",
        "license": "MIT License",
        "module_name": "exepy"
    },
    "flake8_lite": {
        "author": "Masamitsu MURASE",
        "license": "MIT License",
        "module_name": "flake8_lite"
    },
    "single_binary_stackless_python": {
        "author": "Masamitsu MURASE",
        "license": "MIT Lincense",
        "module_name": "single_binary_stackless_python"
    }
}


def included_module_names():
    return tuple(item["module_name"] for item in __included_module_info.values())


def included_module_info():
    return __included_module_info

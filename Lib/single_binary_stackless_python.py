# coding: utf-8


__version__ = "12"
__author__ = "Masamitsu MURASE"

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
        "license": "MIT License",
        "module_name": "single_binary_stackless_python"
    }
}


def included_module_names():
    return tuple(item["module_name"] for item in __included_module_info.values())


def included_module_info():
    return __included_module_info


if __name__ == "__main__":
    print("Version: %s" % __version__)
    print("")

    print("Included 3rd party's libraries:")
    for info in __included_module_info.values():
        module_name = info["module_name"]
        module = __import__(module_name)
        if hasattr(module, "__version__"):
            version = module.__version__
        elif module_name == "pyreadline":
            import pyreadline.release
            version = pyreadline.release.version
        print("* %s (%s)  " % (module_name, version))
        if "author" in info and info["author"]:
            print("  Developed by %s.  " % info["author"])
        print("  Distributed under the %s.  " % info["license"])

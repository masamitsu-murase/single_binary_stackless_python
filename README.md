[![Build status](https://ci.appveyor.com/api/projects/status/btjf8f567h0a1jc3/branch/master3?svg=true)](https://ci.appveyor.com/project/masamitsu-murase/single-binary-stackless-python/branch/master3)

# Single Binary StacklessPython

This is a StacklessPython for Windows, which is packed into a *single* binary.

## Binaries

* python.exe  
  32bit executable image.
* python64.exe  
  64bit executable image.
* python64_pe.exe  
  64bit executable image without _msi built-in module. You can run this image on Windows PE.

## Included Libraries

This binary includes standard library and the following libraries.

* [comtypes](https://pypi.org/project/comtypes/)  
  Developed by Thomas Heller. Released under the MIT License.
* [pycodestyle](https://pypi.org/project/pycodestyle/)  
  Developed by Johann C. Rocholl. Released under the MIT (Expat) License.
* [pyflakes](https://pypi.org/project/pyflakes/)  
  Released under the MIT License.
* [pyreadline](https://pypi.org/project/pyreadline/)  
  Developed by Jorgen Stenarson. Released under the BSD License.
* [PyYAML and libyaml](https://pypi.org/project/PyYAML/)  
  Developed by Kirill Simonov. Released under the MIT License.

## Additional features

### embeddedimport

This binary includes all standard libraries in itself. `embeddedimport` library provides this mechanism.

Please refer to `embeddedimport.c`.

### flake8_lite

This binary includes both pyflakes and pycodestyle. `flake8_lite` is a wrapper for these lint libraries.

You can use this library as follows:

```
python -m flake8_lite input.py
```

---

See an original [README.rst](https://github.com/masamitsu-murase/single_binary_stackless_python/blob/master3/README.rst) for more detail.

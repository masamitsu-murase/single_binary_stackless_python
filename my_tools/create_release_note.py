import argparse
import single_binary_stackless_python
import sys


def create_release_note_content(python_version, level, modules, comment=None):
    content = "Single Binary StacklessPython %s L%d\n" % (python_version,
                                                          level)
    if comment:
        content += comment.rstrip() + "\n\n"
    content += "Version: L%d\n\n" % level
    for info in modules:
        module_name = info["module_name"]
        module = __import__(module_name)
        if hasattr(module, "__version__"):
            version = module.__version__
        elif module_name == "pyreadline":
            import pyreadline.release
            version = pyreadline.release.version
        else:
            raise RuntimeError(module_name + " does not have version.")
        content += "* %s (%s)  \n" % (module_name, version)
        if "author" in info and info["author"]:
            content += "  Developed by %s.  \n" % info["author"]
        content += "  Distributed under the %s.  \n" % info["license"]
    return content


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("output", help="Filename of output")
    args = parser.parse_args()

    python_version = sys.version.split(" ", 1)[0]
    level = int(single_binary_stackless_python.__version__)
    modules = single_binary_stackless_python.included_module_info().values()
    comment = "This is a sample release by Azure Pipelines."
    content = create_release_note_content(python_version, level, modules,
                                          comment)

    with open(args.output, "w") as file:
        file.write(content)

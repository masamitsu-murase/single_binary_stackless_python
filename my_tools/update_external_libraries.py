
import sys
import os
DIRECTORY = os.path.dirname(os.path.abspath(__file__))
sys.path.append(DIRECTORY)

import shutil
import yaml
import github_downloader
import subprocess


LIB_DIR = os.path.normpath(os.path.join(DIRECTORY, "..", "Lib"))
MODULES_DIR = os.path.normpath(os.path.join(DIRECTORY, "..", "Modules"))


# Patch was created by the following command.
# git diff -R --relative .


def process_github_library(lib, base_dir):
    print("Updating %s..." % lib["name"])
    output_dir = os.path.join(base_dir, lib["output_dir"])
    if lib["output_dir"]:
        if os.path.isfile(output_dir):
            os.remove(output_dir)
        elif os.path.isdir(output_dir):
            shutil.rmtree(output_dir)
    if "tag" in lib:
        github_downloader.download_tag_with_filter(lib["github_user"], lib["github_repository"], lib["tag"],
                                                   output_dir, lib["filter_dir"],
                                                   lib.get("skip_depth"))
    else:
        github_downloader.download_sha_with_filter(lib["github_user"], lib["github_repository"], lib["sha"],
                                                   output_dir, lib["filter_dir"],
                                                   lib.get("skip_depth"))
    if "exclude_dirs" in lib:
        for exclude_dir in lib["exclude_dirs"]:
            if os.path.isfile(os.path.join(output_dir, exclude_dir)):
                os.remove(os.path.join(output_dir, exclude_dir))
            if os.path.isdir(os.path.join(output_dir, exclude_dir)):
                shutil.rmtree(os.path.join(output_dir, exclude_dir))
    if "post_processes" in lib:
        for post_process in lib["post_processes"]:
            if post_process["type"] == "2to3":
                print("  Executing lib2to3...")
                subprocess.check_call([sys.executable, "-m", "lib2to3", "-w", "-n", output_dir], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            elif post_process["type"] == "patch":
                print("  Applying patch '%s'..." % post_process['filename'])
                patch_path = os.path.normpath(os.path.join(DIRECTORY, "patches", post_process["filename"]))
                subprocess.check_call(["git", "--git-dir=", "apply", "--whitespace=nowarn", patch_path], cwd=output_dir)
            elif post_process["type"] == "python":
                print("  Running python command '%s'..." % post_process['command'])
                subprocess.check_call(["python", "-c"] + post_process["command"], cwd=output_dir)


def update():
    with open(os.path.join(DIRECTORY, "external_libraries.yaml"), "r") as file:
        external_libraries = yaml.load(file)

    for lib in external_libraries["python_libraries"]:
        try:
            process_github_library(lib, LIB_DIR)
        except Exception:
            print("  **Failed.**")

    for lib in external_libraries["c_libraries"]:
        try:
            process_github_library(lib, MODULES_DIR)
        except Exception:
            print("  **Failed.**")


if __name__ == "__main__":
    update()

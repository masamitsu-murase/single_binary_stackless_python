
import sys
import os
directory = os.path.dirname(os.path.abspath(__file__))
sys.path.append(directory)

import shutil
import yaml
import github_downloader


LIB_DIR = os.path.normpath(os.path.join(directory, "..", "Lib"))


def main():
    with open(os.path.join(directory, "external_libraries.yaml"), "r") as file:
        external_libraries = yaml.load(file)

    for lib in external_libraries:
        print("%s." % lib["name"])
        output_dir = os.path.join(LIB_DIR, lib["output_dir"])
        if os.path.exists(output_dir):
            shutil.rmtree(output_dir)
        if "tag" in lib:
            github_downloader.download_sha_with_filter(lib["github_user"], lib["github_repository"], lib["tag"],
                                                       output_dir, lib["filter_dir"])
        else:
            github_downloader.download_sha_with_filter(lib["github_user"], lib["github_repository"], lib["sha"],
                                                       output_dir, lib["filter_dir"])
        if "exclude_dirs" in lib:
            for exclude_dir in lib["exclude_dirs"]:
                if os.path.exists(os.path.join(output_dir, exclude_dir)):
                    shutil.rmtree(os.path.join(output_dir, exclude_dir))


if __name__ == "__main__":
    main()

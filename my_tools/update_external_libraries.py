
import sys
import os
directory = os.path.dirname(os.path.abspath(__file__))
sys.path.append(directory)

# import lib2to3.main
import shutil
import yaml
import github_downloader


LIB_DIR = os.path.normpath(os.path.join(directory, "..", "Lib"))


def update():
    with open(os.path.join(directory, "external_libraries.yaml"), "r") as file:
        external_libraries = yaml.load(file)

    for lib in external_libraries["python_libraries"]:
        print("Updating %s..." % lib["name"])
        output_dir = os.path.join(LIB_DIR, lib["output_dir"])
        if lib["output_dir"]:
            if os.path.isfile(output_dir):
                os.remove(output_dir)
            elif os.path.isdir(output_dir):
                shutil.rmtree(output_dir)
        if "tag" in lib:
            github_downloader.download_sha_with_filter(lib["github_user"], lib["github_repository"], lib["tag"],
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
                if post_process == "2to3":
                    print("Executing lib2to3...")
                    # if lib2to3.main.main("lib2to3.fixes", ["-w", "-n", output_dir]) != 0:
                    #     raise RuntimeError("lib2to3 failed.")
                elif post_process == "patch":
                    pass


if __name__ == "__main__":
    update()

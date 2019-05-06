
import io
import json
import pathlib
import zipfile
import requests


def extract_zipped_file_with_filter(data, output_dir, path_converter):
    output_path = pathlib.Path(output_dir).resolve()

    with zipfile.ZipFile(io.BytesIO(data)) as file:
        for info in file.infolist():
            path = path_converter(info.filename)
            if path is None:
                continue

            path = output_path.joinpath(path)
            if info.is_dir():
                path.mkdir(parents=True, exist_ok=True)
            else:
                path.parent.mkdir(parents=True, exist_ok=True)
                with open(path, "wb") as f:
                    f.write(file.read(info.filename))


def extract_zipped_file_partially(data, output_dir, directory=""):
    directory_split = list(filter(None, directory.split("/")))

    def path_converter(path):
        path_split = path.split("/")[1:]  # Skip first tag directory.
        if directory_split == path_split[:len(directory_split)]:
            return "/".join(path_split[len(directory_split):])
        else:
            return None

    return extract_zipped_file_with_filter(data, output_dir, path_converter)


def download_zip_with_filter(url, output_dir, filter_path=""):
    response = requests.get(url)
    if not response.ok:
        raise RuntimeError("GitHub access error: '%s': %d" % (url, response.status_code))

    extract_zipped_file_partially(response.content, output_dir, filter_path)


def github_tags(github_user, github_repository):
    url = "https://api.github.com/repos/%s/%s/tags" % (github_user, github_repository)
    response = requests.get(url)
    response.raise_for_status()
    return json.loads(response.text)


def download_tag_with_filter(github_user, github_repository, tag, output_dir, filter_path=""):
    tags = github_tags(github_user, github_repository)
    url = next(x["zipball_url"] for x in tags if x["name"] == tag)
    download_zip_with_filter(url, output_dir, filter_path)


def download_latest_tag_with_filter(github_user, github_repository, output_dir, filter_path):
    tags = github_tags(github_user, github_repository)
    url = tags[0]["zipball_url"]
    download_zip_with_filter(url, output_dir, filter_path)


def download_sha_with_filter(github_user, github_repository, sha, output_dir, filter_path=""):
    url = "https://api.github.com/repos/%s/%s/zipball/%s" % (github_user, github_repository, sha)
    download_zip_with_filter(url, output_dir, filter_path)


if __name__ == "__main__":
    download_latest_tag_with_filter("masamitsu-murase", "pausable_unittest", "pausable_unittest", "pausable_unittest")
    download_latest_tag_with_filter("masamitsu-murase", "wmi_device_manager", "wmidevicemanager", "python/wmidevicemanager")
    download_tag_with_filter("masamitsu-murase", "wmi_device_manager", "Ver_1_1_0", "wmidevicemanager_old", "python/wmidevicemanager")

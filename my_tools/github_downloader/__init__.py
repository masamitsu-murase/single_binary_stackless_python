
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


def extract_zipped_file_partially(data, output_dir, directory="", skip_depth=None):
    directory_split = list(filter(None, directory.split("/")))
    if skip_depth is None:
        skip_depth = len(directory_split)

    def path_converter(path):
        path_split = path.split("/")[1:]  # Skip first tag directory.
        if directory_split == path_split[:len(directory_split)]:
            return "/".join(path_split[skip_depth:])
        else:
            return None

    return extract_zipped_file_with_filter(data, output_dir, path_converter)


def download_zip_with_filter(session, url, output_dir, filter_path="", skip_depth=None):
    response = session.get(url)
    if not response.ok:
        raise RuntimeError("GitHub access error: '%s': %d" % (url, response.status_code))

    extract_zipped_file_partially(response.content, output_dir, filter_path, skip_depth)


def github_tags(session, github_user, github_repository):
    url = "https://api.github.com/repos/%s/%s/tags" % (github_user, github_repository)
    per_page = 100
    page = 1
    all_tags = []
    while True:
        response = session.get(url, params={"per_page": per_page, "page": page})
        response.raise_for_status()
        tags = json.loads(response.text)
        all_tags.extend(tags)
        if len(tags) < per_page:
            break
        page += 1
    return all_tags


def download_tag_with_filter(session, github_user, github_repository, tag, output_dir, filter_path="", skip_depth=None):
    tags = github_tags(session, github_user, github_repository)
    url = next(x["zipball_url"] for x in tags if x["name"] == tag)
    download_zip_with_filter(session, url, output_dir, filter_path, skip_depth)


def download_latest_tag_with_filter(session, github_user, github_repository, output_dir, filter_path="", skip_depth=None):
    tags = github_tags(session, github_user, github_repository)
    url = tags[0]["zipball_url"]
    download_zip_with_filter(session, url, output_dir, filter_path, skip_depth)


def download_sha_with_filter(session, github_user, github_repository, sha, output_dir, filter_path="", skip_depth=None):
    url = "https://api.github.com/repos/%s/%s/zipball/%s" % (github_user, github_repository, sha)
    download_zip_with_filter(session, url, output_dir, filter_path, skip_depth)


if __name__ == "__main__":
    session = requests.Session()
    download_latest_tag_with_filter(session, "masamitsu-murase", "pausable_unittest", "pausable_unittest", "pausable_unittest")
    download_latest_tag_with_filter(session, "masamitsu-murase", "wmi_device_manager", "wmidevicemanager", "python/wmidevicemanager")
    download_tag_with_filter(session, "masamitsu-murase", "wmi_device_manager", "Ver_1_1_0", "wmidevicemanager_old", "python/wmidevicemanager")

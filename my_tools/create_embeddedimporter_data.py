
import glob
import os
import os.path
import tokenize
import zlib


def get_file_data():
    file_list = []

    all_filenames = [i.replace(os.path.sep, "/") for i in glob.glob("**/*.py", recursive=True)] + \
        ["certifi/cacert.pem"] + \
        [i.replace(os.path.sep, "/") for i in glob.glob("werkzeug/debug/shared/*.*")]
    for filename in sorted(all_filenames):
        if filename.startswith("test/") or "/test/" in filename:
            continue
        if filename.startswith("__pycache__/") or "/__pycache__/" in filename:
            continue
        if os.path.basename(filename) == "ubuntu.ttf":
            continue

        if filename.endswith(".py"):
            # Remove comments
            with open(filename, "rb") as file:
                comments_tuple = tuple(x for x in tokenize.tokenize(file.readline) if x.type == tokenize.COMMENT)
            data = []
            comments_iter = iter(comments_tuple)
            with open(filename, "r", encoding="utf-8") as file:
                comment = next(comments_iter, None)
                for lineno, line in enumerate(file, 1):
                    if comment and comment.start[0] == lineno and comment.end[0] == lineno:
                        if line[comment.end[1]:] == "\n":
                            data.append(line[:comment.start[1]].rstrip() + line[comment.end[1]:])
                        else:
                            data.append(line[:comment.start[1]] + line[comment.end[1]:])
                        comment = next(comments_iter, None)
                    else:
                        data.append(line)
            bindata = "".join(data).encode("utf-8")
        else:
            with open(filename, "rb") as file:
                bindata = file.read()
        file_list.append({
            "filename": filename,
            "data": bindata + b"\0"
        })

    return file_list


def output_list(file_list):
    with open("Modules/embeddedimport_data.c", "w") as file:
        file.write('#include <stddef.h>\n\n')

        file.write("char embeddedimporter_filename[] = {\n")
        for item in file_list:
            file.write("  // " + item["filename"] + "\n")
            filename = item["filename"].encode("ascii")
            for slice_data in (filename[i:(i + 16)] for i in range(0, len(filename), 16)):
                file.write("  " + ",".join(("0x%02x" % ch) for ch in slice_data) + ",\n")
            file.write("  0x00,\n")
        file.write("  0x00\n")
        file.write("};\n\n")

        all_data = b"".join(item["data"] for item in file_list)

        file.write("const size_t embeddedimporter_raw_data_size = %d;\n" % len(all_data))
        file.write("\n")

        compressed = zlib.compress(all_data, 9)
        file.write("const unsigned char embeddedimporter_raw_data_compressed[] = {\n")
        for slice_data in (compressed[i:(i + 16)] for i in range(0, len(compressed), 16)):
            file.write("  " + ",".join(("0x%02x" % ch) for ch in slice_data) + ",\n")
        file.write("};\n")
        file.write("\n")

        file.write("const size_t embeddedimporter_raw_data_compressed_size = %d;\n" % len(compressed))
        file.write("\n")

        offset = 0
        file.write("const size_t embeddedimporter_data_offset[] = {\n")
        for item in file_list:
            file.write("  %d,  // %s\n" % (offset, item["filename"]))
            offset += len(item["data"])
        file.write("};\n")


pwd = os.getcwd()
try:
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    os.chdir("..")

    os.chdir("Lib")
    file_list = get_file_data()
    os.chdir("..")
    output_list(file_list)
finally:
    os.chdir(pwd)

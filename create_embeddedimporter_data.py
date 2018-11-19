
import glob
import os
import os.path
import binascii
import zlib


def create_certifi_cacert_data():
    with open("Lib/certifi/cacert.pem", "r") as file:
        cacert_data = file.read()
    with open("Lib/certifi/cacert.py", "w") as file:
        file.write("# This file was generated automatically.\n")
        file.write('_ca_cert_data = r"""')
        file.write(cacert_data)
        file.write('"""\n')


def create_werkzeug_shared_data():
    with open("Lib/werkzeug/debug/_shared.py", "w") as file:
        file.write("# This file was generated automatically.\n")
        file.write('shared_files = {\n')

        for filename in sorted(glob.glob("Lib/werkzeug/debug/shared/*.*")):
            if os.path.basename(filename) == "ubuntu.ttf":
                continue

            with open(filename, "rb") as binfile:
                bindata = binascii.b2a_base64(binfile.read(), newline=False).decode("ascii")
            data = "\n".join(bindata[i:(i + 60)] for i in range(0, len(bindata), 60))
            file.write('    "%s": """' % os.path.basename(filename))
            file.write(data)
            file.write('\n""",\n')
        file.write("}\n")


def get_file_data():
    file_list = []

    all_filenames = [i.replace(os.path.sep, "/") for i in glob.glob("**/*.py", recursive=True)]
    for filename in sorted(all_filenames):
        if filename.startswith("test/") or "/test/" in filename:
            continue
        if filename.startswith("__pycache__/") or "/__pycache__/" in filename:
            continue

        with open(filename, "r", encoding="utf-8") as file:
            bindata = file.read().encode("utf-8")
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
    create_certifi_cacert_data()
    create_werkzeug_shared_data()

    os.chdir("Lib")
    file_list = get_file_data()
    os.chdir("..")
    output_list(file_list)
finally:
    os.chdir(pwd)

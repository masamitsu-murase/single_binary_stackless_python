# coding: utf-8

import glob
import os
import os.path
import sys
import textwrap
import zlib


def collect_file_info(base_dir):
    curdir = os.getcwd()
    try:
        os.chdir(os.path.join(base_dir, "lib"))
        tcl_files = [x for x in glob.iglob("tcl8.6/**/*", recursive=True) if os.path.isfile(x)]
        tk_files = [x for x in glob.iglob("tk8.6/**/*", recursive=True) if os.path.isfile(x)]
        dirs = list(set(os.path.dirname(x) for x in (tcl_files + tk_files)))

        files = sorted(tcl_files + tk_files + dirs)

        all_contents = b""
        result = []
        for file in files:
            if os.path.isfile(file):
                with open(file, "rb") as f:
                    data = f.read()
                result.append({
                    "name": file,
                    "type": 1,
                    "file_offset": len(all_contents),
                    "file_size": len(data)
                })
                all_contents += data
            else:
                result.append({
                    "name": file,
                    "type": 0,
                    "file_offset": -1,
                    "file_size": 0
                })
        return result, all_contents
    finally:
        os.chdir(curdir)


def create_tcl_embeddedfilesystem_data(filename, result, compressed_contents,
                                       original_len):
    with open(filename, "w") as output:
        header = """
        #include <stdio.h>
        #include "tclEmbeddedFilesystem.h"
        int uncompress(unsigned char *dest, unsigned long *dest_len,
                       const unsigned char *src, unsigned long src_len);
        """
        output.write(textwrap.dedent(header[1:]) + "\n")
        output.write("static char gUncompressedData[%d] = {0};\n" % original_len)
        output.write("static char gCompressedData[] = {\n")
        for i, data in enumerate(compressed_contents):
            output.write("0x%02X, " % data)
            if i % 16 == 15:
                output.write("\n")
        output.write("};\n\n")

        output.write("EmbeddedFileInfo gEmbeddedFileInfo[] = {\n")
        for i, item in enumerate(result):
            output.write("{")
            output.write("\"" + item["name"].replace("\\", "/") + "\", ")
            output.write("%d, " % item["type"])
            if item["file_offset"] >= 0:
                output.write("gUncompressedData + %d, " % item["file_offset"])
            else:
                output.write("NULL, ")
            output.write("%d},\n" % item["file_size"])
        output.write("};\n\n")

        footer = """
        void EmbeddedFileInfoDataInitialize()
        {
            if (gUncompressedData[0] == '\\0') {
                unsigned long raw_data_size = sizeof(gUncompressedData);
                uncompress((unsigned char *)gUncompressedData, &raw_data_size,
                           (unsigned char *)gCompressedData, sizeof(gCompressedData));

            }
        }
        """
        output.write(textwrap.dedent(footer[1:]))
        output.write("\n\n")

        output.write("gEmbeddedFileInfoCount = %d;\n" % len(result))


def main(base_dir, output_file):
    result, all_contents = collect_file_info(base_dir)
    compressed_contents = zlib.compress(all_contents, 9)
    create_tcl_embeddedfilesystem_data(output_file, result,
                                       compressed_contents, len(all_contents))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])

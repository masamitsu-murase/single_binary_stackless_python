# coding: utf-8

import argparse
import os.path
import shutil
import struct
import sys
import time
import zlib
import ctypes
import ctypes.wintypes as wintypes
import glob
import tkinter
import tkinter.messagebox

__version__ = "1.0.0"

USER_SOURCE_ID = 200
USER_COMMAND_ID = 201

RT_RCDATA = 10
if ctypes.sizeof(ctypes.c_void_p) == ctypes.sizeof(ctypes.c_ulonglong):
    ULONG_PTR = ctypes.c_ulonglong
elif ctypes.sizeof(ctypes.c_void_p) == ctypes.sizeof(ctypes.c_ulong):
    ULONG_PTR = ctypes.c_ulong
else:
    raise NotImplementedError("No ULONG_PTR")


class DummyStdout(object):
    def write(self, *args, **kwargs):
        pass

    def flush(self):
        pass


if sys.stdout:
    stdout = sys.stdout
else:
    stdout = DummyStdout()


def show_message(message):
    if isinstance(stdout, DummyStdout):
        root = tkinter.Tk()
        root.withdraw()
        try:
            tkinter.messagebox.showinfo("exepy", message)
        finally:
            root.destroy()
    else:
        stdout.write(message)


def show_error(message):
    if isinstance(stdout, DummyStdout):
        root = tkinter.Tk()
        root.withdraw()
        try:
            tkinter.messagebox.showwarning("exepy", message)
        finally:
            root.destroy()
    else:
        stdout.write(message)


class ExepyArgParserError(Exception):
    pass


class ExepyArgParser(argparse.ArgumentParser):
    def _print_message(self, message, file=None):
        if message:
            if file is None:
                show_error(message)
                return
            file.write(message)

    def error(self, message):
        raise ExepyArgParserError(message)


class ExeResourceUpdater(object):
    def __init__(self, exe_dll_filename):
        self.exe_dll_filename = exe_dll_filename
        self.handle = None

    def __enter__(self):
        begin_update_resource = ctypes.windll.kernel32.BeginUpdateResourceW
        begin_update_resource.argtypes = [wintypes.LPWSTR, wintypes.BOOL]
        begin_update_resource.restype = wintypes.HANDLE
        handle = begin_update_resource(self.exe_dll_filename, False)
        if handle == 0:
            raise RuntimeError("BeginUpdateResource failure")
        self.handle = handle
        return self

    def update(self, resource_id, data, lang_id=0, data_type=RT_RCDATA):
        update_resource_api = ctypes.windll.kernel32.UpdateResourceW
        update_resource_api.argtypes = [wintypes.HANDLE, ULONG_PTR, ULONG_PTR,
                                        wintypes.WORD, wintypes.LPVOID,
                                        wintypes.DWORD]
        update_resource_api.restype = wintypes.BOOL
        c_buffer = ctypes.create_string_buffer(data)
        result = update_resource_api(self.handle, data_type, resource_id,
                                     lang_id, c_buffer, len(data))
        if result == 0:
            raise RuntimeError("UpdateResource failure")

    def __exit__(self, exc_type, exc_value, traceback):
        if self.handle is None:
            return

        try:
            end_update_resource = ctypes.windll.kernel32.EndUpdateResourceW
            end_update_resource.argtypes = [wintypes.HANDLE, wintypes.BOOL]
            end_update_resource.restype = wintypes.BOOL
            result = end_update_resource(self.handle, False)
            if result == 0:
                error_code = ctypes.windll.kernel32.GetLastError()
                raise RuntimeError("EndUpdateResource failure: %d" % error_code)
        finally:
            close_handle = ctypes.windll.kernel32.CloseHandle
            close_handle.argtypes = [wintypes.HANDLE]
            close_handle.restype = wintypes.BOOL
            close_handle(self.handle)


# Data structure
#   uint32_t compressed_size;
#   uint32_t uncompressed_size;
#   uint32_t file_count;
#   uint32_t [] file_offset;
#   unsigned char [] compressed_file_data;  // separated by '\0'
#   char [] file_name;  // separated by '\0'.
def create_resource_bin(input_filename_list):
    data = b""
    file_offset = []
    file_name = b""
    for input_filename in input_filename_list:
        with open(input_filename, "rb") as file:
            file_offset.append(len(data))
            data += file.read() + b"\0"
            file_name += input_filename.encode("ascii") + b"\0"
    compressed_data = zlib.compress(data, 9)

    return struct.pack("=LLL%dL" % len(file_offset), len(compressed_data),
                       len(data), len(file_offset), *file_offset) \
        + compressed_data + file_name


def check_args(output_filename, org_input_filename, force):
    if not force and os.path.exists(output_filename):
        show_error("Specify a new file as output.exe.\n")
        sys.exit(2)

    input_filename = []
    for filename in org_input_filename:
        if os.path.isdir(filename):
            input_filename += glob.glob("%s/**/*.py" % filename.replace("\\", "/").rstrip("/"),
                                        recursive=True)
        else:
            input_filename.append(filename)

    input_filename = [os.path.normpath(x) for x in input_filename]
    if os.path.isfile(input_filename[0]):
        if os.path.split(input_filename[0])[0] != "":
            show_error("File must be in the current directory.")
            sys.exit(2)

    curdir = os.path.abspath(os.path.curdir)
    for filename in input_filename:
        if os.path.commonpath([curdir, os.path.abspath(filename)]) != curdir:
            show_error("File must be in the current directory.")
            sys.exit(2)
        if os.path.isabs(filename):
            show_error("File must be a relative path.")
            sys.exit(2)

    return output_filename, input_filename


def create_command_resource_bin(commands):
    # uint32_t  argc
    # char []   argv
    command_str = "".join((x + "\0") for x in commands)
    cmd_ws_array = ctypes.create_unicode_buffer(command_str)
    command_array = ctypes.create_string_buffer(ctypes.sizeof(cmd_ws_array))
    ctypes.memmove(command_array, cmd_ws_array, ctypes.sizeof(cmd_ws_array))
    return struct.pack("=L", len(commands)) + command_array.raw


def create_exe(args):
    try:
        output_filename, input_filename_list = check_args(args.exe_name, args.input_py, args.force)
        resource_bin = create_resource_bin(input_filename_list)
        main_module = os.path.splitext(os.path.basename(input_filename_list[0]))[0]
        commands = ("-m", main_module)
        command_resource = create_command_resource_bin(commands)
    except Exception as error:
        try:
            os.remove(output_filename)
        except Exception:
            pass
        show_error(str(error))
        sys.exit(3)

    try:
        stdout.write("Generating.")
        stdout.flush()
        retry_max_count = 10
        for i in range(retry_max_count):
            try:
                shutil.copy(sys.executable, output_filename)
                with ExeResourceUpdater(os.path.abspath(output_filename)) as updater:
                    updater.update(USER_SOURCE_ID, resource_bin)
                    if not args.nocommand:
                        updater.update(USER_COMMAND_ID, command_resource)
                    time.sleep(0.3)
            except RuntimeError:
                if i != retry_max_count - 1:
                    stdout.write(".")
                    stdout.flush()
                    time.sleep(0.1)
                else:
                    raise
            else:
                break
        stdout.write("\n")
        stdout.flush()
        show_message("Done.\n%s was built successfully.\n" % output_filename)
    except Exception as error:
        try:
            os.remove(output_filename)
        except Exception:
            pass
        show_error(str(error))
        sys.exit(3)


def extract_exe(args):
    pass


def main():
    root_parser = ExepyArgParser(description="Create/Extract a single executable binary.", add_help=False)
    subparsers = root_parser.add_subparsers(dest="command", title="command", metavar="[create|extract]")

    # create
    parser = subparsers.add_parser('create', aliases=["c"], help="Create exe file. See 'create -h'")
    parser.add_argument("--force", "-f", help="overwrite exe_name", action="store_true")
    parser.add_argument("--nocommand", "-n", help="do not run '-m your_module'", action="store_true")
    parser.add_argument("exe_name", help="executable filename")
    parser.add_argument("input_py", help="input filenames", nargs="+")

    # extract
    parser = subparsers.add_parser("extract", aliases=["e"], help="Extract exe file. See 'extract -h'")
    parser.add_argument("--force", "-f", help="overwrite output files", action="store_true")
    parser.add_argument("exe_name", help="executable filename")

    try:
        args = root_parser.parse_args()
    except ExepyArgParserError as exc:
        show_error(root_parser.format_help() + "\n\n" + exc.args[0] + "\n")
        sys.exit(1)

    if args.command == "create":
        create_exe(args)
    elif args.command == "extract":
        extract_exe(args)
    else:
        show_error(root_parser.format_help() + "\n")
        sys.exit(1)


if __name__ == "__main__":
    main()

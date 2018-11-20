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
import pathlib
import tkinter
import tkinter.messagebox

__version__ = "2.0.0"

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
        filepath = pathlib.Path(filename)
        if filepath.is_dir():
            input_filename += [str(i).replace("\\", "/") for i in filepath.glob("**/*") if i.is_file()]
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
    # wchar_t []   argv
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


class ExeResourceLoader(object):
    def __init__(self, exe_filename):
        self.exe_filename = exe_filename
        self.module = None

    def __enter__(self):
        load_library = ctypes.windll.kernel32.LoadLibraryW
        load_library.argtypes = [wintypes.LPCWSTR]
        load_library.restype = wintypes.HMODULE
        module = load_library(self.exe_filename)
        if module == 0:
            raise RuntimeError("LoadLibrary failed.")
        self.module = module

        return self

    def load_resource(self, resource_id):
        find_resource = ctypes.windll.kernel32.FindResourceW
        find_resource.argtypes = [wintypes.HMODULE, ULONG_PTR, ULONG_PTR]
        find_resource.restype = wintypes.HRSRC
        resource = find_resource(self.module, resource_id, RT_RCDATA)
        if resource == 0:
            return None

        load_resource = ctypes.windll.kernel32.LoadResource
        load_resource.argtypes = [wintypes.HMODULE, wintypes.HRSRC]
        load_resource.restype = wintypes.HGLOBAL
        data_handle = load_resource(self.module, resource)
        if data_handle == 0:
            raise RuntimeError("LoadResource failed.")

        lock_resource = ctypes.windll.kernel32.LockResource
        lock_resource.argtypes = [wintypes.HGLOBAL]
        lock_resource.restype = wintypes.LPVOID
        data = lock_resource(data_handle)
        if data == 0:
            raise RuntimeError("LockResource failed.")

        sizeof_resource = ctypes.windll.kernel32.SizeofResource
        sizeof_resource.argtypes = [wintypes.HMODULE, wintypes.HRSRC]
        sizeof_resource.restype = wintypes.DWORD
        size = sizeof_resource(self.module, resource)
        if size == 0:
            raise RuntimeError("SizeofResource failed.")

        buffer = ctypes.create_string_buffer(size)
        ctypes.memmove(buffer, data, size)

        return buffer.raw

    def __exit__(self, exc_type, exc_value, traceback):
        if self.module is None:
            return

        free_library = ctypes.windll.kernel32.FreeLibrary
        free_library.argtypes = [wintypes.HMODULE]
        free_library.restype = wintypes.BOOL
        free_library(self.module)


def extract_resource_in_exe(exe_name):
    if not os.path.isfile(exe_name):
        raise RuntimeError("%s is not a valid file." % exe_name)

    #     //   uint32_t compressed_size;
    #     //   uint32_t uncompressed_size;
    #     //   uint32_t file_count;
    #     //   uint32_t [] file_offset;
    #     //   unsigned char [] compressed_file_data;  // separated by '\0'
    #     //   char [] file_name;  // separated by '\0'.
    with ExeResourceLoader(exe_name) as loader:
        source = loader.load_resource(USER_SOURCE_ID)
        command = loader.load_resource(USER_COMMAND_ID)

    offset = 0
    compressed_size, uncompressed_size, file_count = struct.unpack_from("=LLL", source)
    offset += 12
    file_offset = struct.unpack_from("=%dL" % file_count, source, offset)
    offset += 4 * file_count
    compressed_file_data = struct.unpack_from("=%ds" % compressed_size, source, offset)[0]
    uncompressed_file_data = zlib.decompress(compressed_file_data)
    offset += compressed_size
    file_name = struct.unpack_from("=%ds" % (len(source) - offset), source, offset)[0]

    file_info = []
    file_name_list = file_name.split(b"\0")
    file_offset = file_offset + (uncompressed_size, )
    for i in range(file_count):
        info = {
            "filename": file_name_list[i].decode("ascii"),
            "content": uncompressed_file_data[file_offset[i]:(file_offset[i + 1] - 1)]
        }
        file_info.append(info)

    argv = None
    if command:
        # uint32_t  argc
        # wchar_t []   argv
        offset = 0
        command_count = struct.unpack_from("=L", command)[0]
        offset += 4
        command_byte = struct.unpack_from("=%ds" % (len(command) - offset), command, offset)[0]
        # Add sentinel.
        byte_buffer = ctypes.create_string_buffer(command_byte + (b"\0" * ctypes.sizeof(ctypes.c_wchar)))

        argv = []
        address = ctypes.addressof(byte_buffer)
        address_end = address + len(command_byte)
        for i in range(command_count):
            arg = ctypes.wstring_at(address)
            arg_len = ctypes.sizeof(ctypes.create_unicode_buffer(arg))
            address += arg_len
            argv.append(arg)
            if address >= address_end:
                raise RuntimeError("command string is corrupted.")

    return file_info, argv


def extract_exe(args):
    try:
        file_info, argv = extract_resource_in_exe(args.exe_name)

        if not args.force:
            for info in file_info:
                if os.path.exists(info["filename"]):
                    raise RuntimeError("'%s' already exists." % info["filename"])
        for info in file_info:
            directory = os.path.dirname(info["filename"])
            if directory and not os.path.isdir(directory):
                os.makedirs(directory)
            with open(info["filename"], "wb") as file:
                file.write(info["content"])

        if argv:
            show_message("argv: " + " ".join(('"' + x + '"') for x in argv) + "\n")
        else:
            show_message("no argv" + "\n")

        show_message("Files were extracted." + "\n")
    except Exception as error:
        show_error(str(error) + "\n")
        sys.exit(1)


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

    if args.command in ("create", "c"):
        create_exe(args)
    elif args.command in ("extract", "e"):
        extract_exe(args)
    else:
        show_error(root_parser.format_help() + "\n")
        sys.exit(1)


if __name__ == "__main__":
    main()

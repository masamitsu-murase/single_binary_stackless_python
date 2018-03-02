@echo off

if "%1" == "x86" (
    set BUILD_TARGET_CPU=x86
    set WINDOWS_PE=0
) else if "%1" == "x64" (
    set BUILD_TARGET_CPU=x64
    set WINDOWS_PE=0
) else if "%1" == "x64_pe" (
    set BUILD_TARGET_CPU=x64
    set WINDOWS_PE=1
) else (
    set BUILD_TARGET_CPU=x86
    set WINDOWS_PE=0
)

if "%BUILD_TARGET_CPU%" == "x86" (
    set PYTHON=python.exe
) else if "%WINDOWS_PE%" == "0" (
    set PYTHON=python64.exe
) else (
    set PYTHON=python64_pe.exe
)

echo BUILD_TARGET_CPU %BUILD_TARGET_CPU%
echo WINDOWS_PE %WINDOWS_PE%
echo PYTHON %PYTHON%

cd /d "%~dp0"
%PYTHON% -c "import sys; print(sys.version)"

%PYTHON% my_test\test_original_changes.py
if ERRORLEVEL 1 (
    echo "Test failed."
    exit /b 1
)

%PYTHON% -m test.regrtest test___all__ test___future__ test__locale test__osx_support test_abc test_abstract_numbers test_aepack test_aifc test_al test_anydbm
REM test_argparse test_asynchat test_asyncore
%PYTHON% -m test.regrtest test_applesingle test_array test_ascii_formatd test_ast test_atexit test_audioop test_augassign
%PYTHON% -m test.regrtest test_base64 test_bastion test_bigaddrspace test_bigmem test_binascii test_binhex test_binop test_bisect test_bool test_bsddb
REM test_calendar
%PYTHON% -m test.regrtest test_bsddb185 test_bsddb3 test_buffer test_bufio test_builtin test_bytes test_bz2 test_call test_capi
REM test_class test_cmath
%PYTHON% -m test.regrtest test_cd test_cfgparser test_cgi test_charmapcodec test_cl test_cmd test_cmd_line test_cmd_line_script
%PYTHON% -m test.regrtest test_code test_codeccallbacks test_codecencodings_cn test_codecencodings_hk test_codecencodings_iso2022 test_codecencodings_jp test_codecencodings_kr test_codecencodings_tw test_codecmaps_cn test_codecmaps_hk
REM test_codecs
%PYTHON% -m test.regrtest test_codecmaps_jp test_codecmaps_kr test_codecmaps_tw test_codeop test_coercion test_collections test_colorsys test_commands test_compare
%PYTHON% -m test.regrtest test_compile test_compileall test_compiler test_complex test_complex_args test_contains test_contextlib test_cookie test_cookielib test_copy
%PYTHON% -m test.regrtest test_copy_reg test_cpickle test_cprofile test_crypt test_csv test_ctypes test_curses test_datetime test_dbm test_decimal
%PYTHON% -m test.regrtest test_decorators test_defaultdict test_deque test_descr test_descrtut test_dict test_dictcomps test_dictviews test_difflib test_difflib_expect
%PYTHON% -m test.regrtest test_dircache test_dis test_distutils test_dl test_doctest test_doctest test_doctest2 test_doctest2 test_doctest3 test_doctest4
%PYTHON% -m test.regrtest test_docxmlrpc test_dumbdbm test_dummy_thread test_dummy_threading test_email test_email_codecs test_email_renamed test_ensurepip test_enumerate test_eof
%PYTHON% -m test.regrtest test_epoll test_errno test_exception_variations test_exceptions test_extcall test_fcntl test_file test_file_eintr test_file2k test_filecmp
%PYTHON% -m test.regrtest test_fileinput test_fileio test_float test_fnmatch test_fork1 test_format test_fpformat test_fractions test_frozen test_ftplib
%PYTHON% -m test.regrtest test_funcattrs test_functools test_future test_future_builtins test_future1 test_future2 test_future3 test_future4 test_future5 test_gc
%PYTHON% -m test.regrtest test_gdb test_gdbm test_generators test_genericpath test_genexps test_getargs test_getargs2 test_getopt test_gettext test_gl
%PYTHON% -m test.regrtest test_glob test_global test_grammar test_grp test_gzip test_hash test_hashlib test_heapq test_hmac test_hotshot
%PYTHON% -m test.regrtest test_htmllib test_htmlparser test_httplib test_httpservers test_idle test_imageop test_imaplib test_imgfile test_imghdr test_imp
%PYTHON% -m test.regrtest test_import test_import_magic test_importhooks test_importlib test_index test_inspect test_int test_int_literal test_io test_ioctl
%PYTHON% -m test.regrtest test_isinstance test_iter test_iterlen test_itertools test_json test_kqueue test_largefile test_lib2to3 test_linecache test_linuxaudiodev
%PYTHON% -m test.regrtest test_list test_locale test_logging test_long test_long_future test_longexp test_macos test_macostools test_macpath test_macurl2path
%PYTHON% -m test.regrtest test_mailbox test_marshal test_math test_md5 test_memoryio test_memoryview test_mhlib test_mimetools test_mimetypes test_MimeWriter
%PYTHON% -m test.regrtest test_minidom test_mmap test_module test_modulefinder test_msilib test_multibytecodec test_multifile test_multiprocessing test_mutants test_mutex
%PYTHON% -m test.regrtest test_netrc test_new test_nis test_nntplib test_normalization test_ntpath test_old_mailbox test_opcodes test_openpty test_operator
%PYTHON% -m test.regrtest test_optparse test_ordered_dict test_os test_ossaudiodev test_parser test_pdb test_peepholer test_pep247 test_pep277 test_pep352
%PYTHON% -m test.regrtest test_pickle test_pickletools test_pipes test_pkg test_pkgimport test_pkgutil test_platform test_plistlib test_poll test_popen
%PYTHON% -m test.regrtest test_popen2 test_poplib test_posix test_posixpath test_pow test_pprint test_print test_profile test_property test_pstats
%PYTHON% -m test.regrtest test_pty test_pwd test_py_compile test_py3kwarn test_pyclbr test_pydoc test_pyexpat test_queue test_quopri test_random
%PYTHON% -m test.regrtest test_re test_readline test_regrtest test_repr test_resource test_rfc822 test_richcmp test_rlcompleter test_robotparser test_runpy
%PYTHON% -m test.regrtest test_sax test_scope test_scriptpackages test_select test_set test_setcomps test_sets test_sgmllib test_sha test_shelve
%PYTHON% -m test.regrtest test_shlex test_shutil test_signal test_SimpleHTTPServer test_site test_slice test_smtplib test_smtpnet test_socket test_socketserver
%PYTHON% -m test.regrtest test_softspace test_sort test_source_encoding test_spwd test_sqlite test_ssl test_startfile test_stat test_str test_strftime
%PYTHON% -m test.regrtest test_string test_StringIO test_stringprep test_strop test_strptime test_strtod test_struct test_structmembers test_structseq test_subprocess
%PYTHON% -m test.regrtest test_sunau test_sunaudiodev test_sundry test_support test_symtable test_syntax test_sys test_sys_setprofile test_sys_settrace test_sysconfig
%PYTHON% -m test.regrtest test_tarfile test_tcl test_telnetlib test_tempfile test_test_support test_textwrap test_thread test_threaded_import test_threadedtempfile test_threading
%PYTHON% -m test.regrtest test_threading_local test_threadsignals test_time test_timeit test_timeout test_tk test_tokenize test_tools test_trace test_traceback
%PYTHON% -m test.regrtest test_transformer test_ttk_guionly test_ttk_textonly test_tuple test_turtle test_typechecks test_types test_ucn test_unary test_undocumented_details
%PYTHON% -m test.regrtest test_unicode test_unicode_file test_unicodedata test_unittest test_univnewlines test_univnewlines2k test_unpack test_urllib test_urllib2 test_urllib2_localnet
%PYTHON% -m test.regrtest test_urllib2net test_urllibnet test_urlparse test_userdict test_userlist test_userstring test_uu test_uuid test_wait3 test_wait4
%PYTHON% -m test.regrtest test_warnings test_wave test_weakref test_weakset test_whichdb test_winreg test_winsound test_with test_wsgiref test_xdrlib
%PYTHON% -m test.regrtest test_xml_etree test_xml_etree_c test_xmllib test_xmlrpc test_xpickle test_xrange test_zipfile test_zipfile64 test_zipimport test_zipimport_support
%PYTHON% -m test.regrtest test_zlib
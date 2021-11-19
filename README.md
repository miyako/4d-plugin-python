![version](https://img.shields.io/badge/version-18%2B-EB8E5F)
![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm%20|%20win-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/4d-plugin-python)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/4d-plugin-python/total)

# 4d-plugin-python
4D implementation of libpython.

* want more functionality than [micropython](https://micropython.org), etc.
* want static library for package distribution.

### Project Method Callback

```4d
/*
	import fourd
	d = {"foo":"bar"};
	fourd.call("cb_test", d);
*/

  //invoke the project method "cb_test" passing d as $1 (C_OBJECT)

METHOD GET CODE(Current method path;$code)

ARRAY LONGINT($pos;0)
ARRAY LONGINT($len;0)

If (Match regex("(?s)(?:\\/\\*)(.*)(?:\\*\\/)";$code;1;$pos;$len))
	$py:=Substring($code;$pos{1};$len{1})
	$status:=python ($py)
End if 
```

### Issues 

`stdout` and `stderr` are not captured. they are sent to `stdout` and `stderr` of the host application.

The standard technique using `dup` and `dup2` does not seeem to work for redirecting `libpython`.

* https://docs.microsoft.com/ja-jp/cpp/c-runtime-library/reference/dup-dup2?view=msvc-170

### TODO

* imput stream [PyOS_InputHook](https://docs.python.org/3.6/c-api/veryhigh.html#c.PyOS_InputHook)
* 4D method callback [PyCFunction](https://docs.python.org/3.6/c-api/structures.html#c.PyCFunction)

### CMake Options

> (On) Windows it is not possible to build shared .dll extensions against a static libpython, so you must build any extensions you want into libpython itself.
 
 - [x] BUILTIN_ARRAY
 - [x] BUILTIN_ATEXIT
 - [x] BUILTIN_AUDIOOP
 - [x] BUILTIN_BINASCII
 - [x] BUILTIN_BISECT
 - [x] BUILTIN_BLAKE2
 - [x] BUILTIN_BZ2
 - [x] BUILTIN_CMATH
 - [x] BUILTIN_CODECS
 - [x] BUILTIN_CODECS_CN
 - [x] BUILTIN_CODECS_HK
 - [x] BUILTIN_CODECS_ISO2022
 - [x] BUILTIN_CODECS_JP
 - [x] BUILTIN_CODECS_KR
 - [x] BUILTIN_CODECS_TW
 - [x] BUILTIN_CSV
 - [x] BUILTIN_CTYPES
 - [x] BUILTIN_CURSES
 - [x] BUILTIN_CURSES_PANEL
 - [x] BUILTIN_DATETIME
 - [x] BUILTIN_DBM
 - [x] BUILTIN_DECIMAL
 - [x] BUILTIN_ELEMENTTREE
 - [x] BUILTIN_ERRNO
 - [x] BUILTIN_FAULTHANDLER
 - [x] BUILTIN_FCNTL
 - [x] BUILTIN_FUNCTOOLS
 - [x] BUILTIN_GDBM
 - [x] BUILTIN_GRP
 - [x] BUILTIN_HASHLIB
 - [x] BUILTIN_HEAPQ
 - [x] BUILTIN_IO
 - [x] BUILTIN_ITERTOOLS
 - [x] BUILTIN_JSON
 - [x] BUILTIN_LOCALE
 - [x] BUILTIN_LSPROF
 - [x] BUILTIN_LZMA
 - [x] BUILTIN_MATH
 - [x] BUILTIN_MD5
 - [x] BUILTIN_MMAP
 - [x] BUILTIN_MULTIBYTECODEC
 - [x] BUILTIN_MULTIPROCESSING
 - [x] BUILTIN_OPCODE
 - [x] BUILTIN_PARSER
 - [x] BUILTIN_PICKLE
 - [x] BUILTIN_POSIX
 - [x] BUILTIN_POSIXSUBPROCESS
 - [x] BUILTIN_PWD
 - [x] BUILTIN_PYEXPAT
 - [x] BUILTIN_RANDOM
 - [x] BUILTIN_READLINE
 - [x] BUILTIN_RESOURCE
 - [x] BUILTIN_SCPROXY
 - [x] BUILTIN_SELECT
 - [x] BUILTIN_SHA1
 - [x] BUILTIN_SHA256
 - [x] BUILTIN_SHA3
 - [x] BUILTIN_SHA512
 - [x] BUILTIN_SOCKET
 - [x] BUILTIN_SQLITE3
 - [x] BUILTIN_SRE
 - [x] BUILTIN_SSL
 - [x] BUILTIN_STAT
 - [x] BUILTIN_STRUCT
 - [x] BUILTIN_SYMTABLE
 - [x] BUILTIN_SYSLOG
 - [x] BUILTIN_TERMIOS
 - [x] BUILTIN_TESTBUFFER
 - [x] BUILTIN_TESTIMPORTMULTIPLE
 - [x] BUILTIN_TESTMULTIPHASE
 - [x] BUILTIN_THREAD
 - [x] BUILTIN_TIME
 - [x] BUILTIN_TKINTER
 - [x] BUILTIN_TRACEMALLOC
 - [x] BUILTIN_UNICODEDATA
 - [x] BUILTIN_WEAKREF
 - [x] BUILTIN_XXLIMITED
 - [x] BUILTIN_XXSUBTYPE
 - [x] BUILTIN_ZLIB

Windows or UNIX specific built-in extensions are left untouched. 

### 参考記事

* https://github.com/python-cmake-buildsystem/python-cmake-buildsystem
* [python-cmake-buildsystem で libpython を自前 C/C++ アプリに組み込む](https://qiita.com/syoyo/items/5a935fdcbdf89e0a2635)
* [Fatal Python error: Py_Initialize: unable to load the file system](https://stackoverflow.com/questions/34724057/embed-python3-without-standard-library)

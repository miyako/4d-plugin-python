# 4d-plugin-python
4D implementation of libpython.

* want more functionality than [micropython](https://micropython.org), etc.
* want static library for package distribution.

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
 - [ ] BUILTIN_ELEMENTTREE=ON
 - [ ] BUILTIN_FCNTL=ON
 - [ ] BUILTIN_FUNCTOOLS=ON
 - [ ] BUILTIN_FUTURE_BUILTINS=ON
 - [ ] BUILTIN_GDBM=ON
 - [ ] BUILTIN_GRP=ON
 - [ ] BUILTIN_HASHLIB=ON
 - [ ] BUILTIN_HEAPQ=ON
 - [ ] BUILTIN_HOTSHOT=ON
 - [ ] BUILTIN_IO=ON
 - [ ] BUILTIN_ITERTOOLS=ON
 - [ ] BUILTIN_JSON=ON
 - [ ] BUILTIN_LINUXAUDIODEV=ON
 - [ ] BUILTIN_LOCALE=ON
 - [ ] BUILTIN_LSPROF=ON
 - [ ] BUILTIN_LZMA=ON
 - [ ] BUILTIN_MATH=ON
 - [ ] BUILTIN_MMAP=ON
 - [ ] BUILTIN_MULTIBYTECODEC=ON
 - [ ] BUILTIN_MULTIPROCESSING=ON
 - [ ] BUILTIN_NIS=ON
 - [ ] BUILTIN_NT=ON
 - [ ] BUILTIN_OPERATOR=ON
 - [ ] BUILTIN_OSSAUDIODEV=ON
 - [ ] BUILTIN_PARSER=ON
 - [ ] BUILTIN_POSIX=ON
 - [ ] BUILTIN_PWD=ON
 - [ ] BUILTIN_PYEXPAT=ON
 - [ ] BUILTIN_RANDOM=ON
 - [ ] BUILTIN_READLINE=ON
 - [ ] BUILTIN_RESOURCE=ON
 - [ ] BUILTIN_SELECT=ON
 - [ ] BUILTIN_SOCKET=ON
 - [ ] BUILTIN_SPWD=ON
 - [ ] BUILTIN_SQLITE3=ON
 - [ ] BUILTIN_SSL=ON
 - [ ] BUILTIN_STROP=ON
 - [ ] BUILTIN_STRUCT=ON
 - [ ] BUILTIN_SYSLOG=ON
 - [ ] BUILTIN_TERMIOS=ON
 - [ ] BUILTIN_TESTCAPI=ON
 - [ ] BUILTIN_TIME=ON
 - [ ] BUILTIN_TKINTER=ON
 - [ ] BUILTIN_UNICODEDATA
 - [x] BUILTIN_WEAKREF
 - [x] BUILTIN_XXLIMITED
 - [x] BUILTIN_XXSUBTYPE
 - [x] BUILTIN_ZLIB

### 参考記事

* https://github.com/python-cmake-buildsystem/python-cmake-buildsystem
* [python-cmake-buildsystem で libpython を自前 C/C++ アプリに組み込む](https://qiita.com/syoyo/items/5a935fdcbdf89e0a2635)

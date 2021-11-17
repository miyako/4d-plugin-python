# 4d-plugin-python
4D implementation of libpython.

* want more functionality than [micropython](https://micropython.org), etc.
* want static library for package distribution.

### CMake Options

> (On) Windows it is not possible to build shared .dll extensions against a static libpython, so you must build any extensions you want into libpython itself.
 
 - [ ] BUILTIN_ARRAY=ON
 - [ ] BUILTIN_AUDIOOP=ON
 - [ ] BUILTIN_BINASCII=ON
 - [ ] BUILTIN_BISECT=ON
 - [ ] BUILTIN_BSDDB=ON
 - [ ] BUILTIN_BZ2=ON
 - [ ] BUILTIN_CMATH=ON
 - [ ] BUILTIN_CODECS_CN=ON
 - [ ] BUILTIN_CODECS_HK=ON
 - [ ] BUILTIN_CODECS_ISO2022=ON
 - [ ] BUILTIN_CODECS_JP=ON
 - [ ] BUILTIN_CODECS_KR=ON
 - [ ] BUILTIN_CODECS_TW=ON
 - [ ] BUILTIN_COLLECTIONS=ON
 - [ ] BUILTIN_CPICKLE=ON
 - [ ] BUILTIN_CRYPT=ON
 - [ ] BUILTIN_CSTRINGIO=ON
 - [ ] BUILTIN_CSV=ON
 - [ ] BUILTIN_CTYPES=ON
 - [ ] BUILTIN_CTYPES_TEST=ON
 - [ ] BUILTIN_CURSES=ON
 - [ ] BUILTIN_CURSES_PANEL=ON
 - [ ] BUILTIN_DATETIME=ON
 - [ ] BUILTIN_DBM=ON
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
 - [ ] BUILTIN_UNICODEDATA=ON
 - [ ] BUILTIN_ZLIB=ON

### 参考記事

* https://github.com/python-cmake-buildsystem/python-cmake-buildsystem
* [python-cmake-buildsystem で libpython を自前 C/C++ アプリに組み込む](https://qiita.com/syoyo/items/5a935fdcbdf89e0a2635)

![version](https://img.shields.io/badge/version-18%2B-EB8E5F)
![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm%20|%20win-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/4d-plugin-python)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/4d-plugin-python/total)

# 4d-plugin-python

python runs Python code inside 4D. The plugin embeds [CPython](https://www.python.org) 3.6 as a static library (`libpython`), so nothing needs to be installed on the machine; the Python standard library ships inside the plugin bundle. You pass the code as `Text`, optionally with an `Object` of local variables, and get back an `Object` with the interpreter's version, the value the code produced and any error message. Python code can call back into 4D project methods through a built-in `fourd` module.

* want more functionality than [micropython](https://micropython.org), etc.
* want static library for package distribution.

| Command | Returns | Purpose |
|---|---|---|
| [python](#python) | `Object` | Run Python code, with optional local variables, and get its result |

**Platforms:** macOS (Apple silicon and Intel), Windows (64-bit)

---

## Requirements & platform notes

- **One command; only the first parameter is required.** `python(&T;&J;&L):J` in the manifest: code, locals, mode. The test methods call it with one, two or three parameters.
- **The interpreter is started once, when 4D loads the plugin**, and stays alive until 4D quits. Every call, from every 4D process, runs in the same interpreter and shares the same `__main__` module, so modules you import and names you put in `__main__` remain for later calls.
- **The interpreter is isolated from the system.** Its module search path is only the standard library inside the plugin (`Contents/Resources/python/lib`); `site` is not loaded, the user site directory is ignored, and `PYTHON*` environment variables are ignored. Third-party packages are not available unless they are pure Python and copied into that folder. Some standard modules don't work in this build (the test project notes `tempfile` and `shutil`).
- **Don't call the command from preemptive processes.** The manifest declares it thread-safe, but the plugin never acquires or releases Python's global interpreter lock, which the interpreter requires when it is used from more than one thread. Use it from cooperative processes.
- **Errors are reported in the result, not as 4D errors.** Check `errorMessage` (see [Error handling & troubleshooting](#error-handling--troubleshooting)).
- **Python 3.6 is past its end of life** (no security fixes since 2021). Don't run code you don't trust.

---

## python

### Syntax

```
python ( code {; locals {; mode}} ) → status
```

| Parameter | Type | Description |
|---|---|---|
| `code` | Text | The Python source to run |
| `locals` | Object | Optional. Variables made available to the code as its local namespace |
| `mode` | Longint | Optional. How `code` is compiled: `python_exec` (default), `python_eval` or `python_line` |
| Result | Object | Interpreter information, `returnValue` and `errorMessage` (see below) |

**Link Type constants** (`mode`):

| Constant | Value | Python start symbol | Use for |
|---|---|---|---|
| `python_exec` | `0` | `Py_file_input` | A sequence of statements, like a module. Produces no value |
| `python_eval` | `1` | `Py_eval_input` | A single expression whose value you want back |
| `python_line` | `2` | `Py_single_input` | A single interactive statement |

Any other `mode` value runs the code like `python_exec`.

### Description

`python` converts `locals` to a Python `dict` (through `JSON Stringify`), then runs `code` with that dict as the local namespace and `__main__`'s dict as the global namespace. Names the code assigns at top level go into the locals dict, which is discarded after the call.

**Values in `locals`** arrive in Python as:

| 4D value | Python value |
|---|---|
| Text | `str` |
| Integer number | `int` |
| Other number | `float` |
| Boolean | `bool` |
| Null | `None` |
| Object | `dict` |
| Collection | `list` |

Values that `JSON Stringify` turns into strings (dates, for example) arrive as `str`. **Earlier builds** dropped objects and collections nested inside an object; with the fixed source in this repository they are passed.

**Result object:**

| Property | Type | Description |
|---|---|---|
| `version` | Text | `sys.version` of the embedded interpreter |
| `build` | Text | Build information of the interpreter |
| `copyright` | Text | Python's copyright notice |
| `compiler` | Text | Compiler used to build the interpreter |
| `returnValue` | Object | The value the code produced, when it is a `dict` (see below). Absent if the code raised an exception |
| `errorMessage` | Text | The exception message, if the code raised one |

`returnValue` is always an object. Only a `dict` result is converted: `python_eval` of `{"value": 5 + 5}` gives `{"value":10}`, while `python_eval` of `5 + 5`, or any `python_exec` call (whose value is `None`), gives an empty object. To return something else, wrap it in a dict.

Inside a returned `dict`, keys must be `str` and values are converted as follows: `str` → Text, `int` and `float` → Real, `bool` → Boolean, `None` → Null, `dict` → Object, `list` → Collection. In a `list`, `int` becomes a Longint (32-bit). Other types (tuples, bytes, …) are skipped. **Earlier builds** turned `True`/`False` into `1`/`0` and skipped `None`; the fixed source converts them to Boolean and Null.

**Earlier builds** didn't set `errorMessage` for ordinary exceptions (the exception object wasn't converted to text). With the fixed source, it contains `str()` of the exception, for example `name 'x' is not defined`.

### Calling 4D from Python: the fourd module

The interpreter has a built-in module `fourd` with one function:

```
fourd.call(method_name, params=None)
```

- `method_name` (`str`): name of a 4D project method.
- `params` (`dict`, optional): passed to the method as `$1` (Object). If you omit it, or pass something other than a dict, the method is called without parameters.
- Returns the method's `$0` converted to Python: Object → `dict`, Collection → `list`, Text → `str`, Real → `float`, Longint/Integer/Time → `int`, Boolean → `bool`, Null → `None`, BLOB → `bytes`. Dates and pictures are not converted.

With the fixed source, `fourd.call` returns `None` when the method doesn't exist or its result can't be converted. **Earlier builds** raised `SystemError` in that case, which stopped the Python code.

### Example

From the plugin's own test method (`TEST_1.4dm`), the three modes:

```4d
/*
to evaluate a statement; the value type must be object
*/

$status:=python("{\"value\" : 5 + 5 }"; New object:C1471; python_eval)
//Py_eval_input (isolated expressions)

/*
this with evalute the script like a module but return nothing. this is the default
*/

$status:=python("from time import time,ctime\n\nprint('Today is', ctime(time()))\n"; New object:C1471; python_exec)
//Py_file_input (sequences of statements)

/*
not sure if this is useful, because it is intended for interactive loops but the plugin is one-shot
*/

$status:=python("{\"value\" : 5 + 5 }"; New object:C1471; python_line)
//Py_single_input (single statement)
```

From `TEST_2.4dm`: the Python code is kept in a comment of the method itself and calls the project method `cb_test` with a dict:

```4d
/*
import fourd
d = {"foo":"bar"};
fourd.call("cb_test", d);
*/

  //invoke the project method "cb_test" passing d as $1 (C_OBJECT)

METHOD GET CODE:C1190(Current method path:C1201;$code)

ARRAY LONGINT:C221($pos;0)
ARRAY LONGINT:C221($len;0)

If (Match regex:C1019("(?s)(?:\\/\\*)(.*)(?:\\*\\/)";$code;1;$pos;$len))
	$py:=Substring:C12($code;$pos{1};$len{1})
	$status:=python ($py)
End if 
```

`cb_test.4dm`, the callback:

```4d
C_OBJECT:C1216($1;$0)

ALERT:C41(JSON Stringify:C1217($1;*))

$0:=New object:C1471
```

From `TEST_3.4dm`: pass 4D values as locals and forward them with `locals()`:

```4d
/*
import fourd
fourd.call("cb_test", locals());
fourd.call("cb_test", locals());
*/

//invoke the project method "cb_test" passing locals() as $1 (C_OBJECT)

METHOD GET CODE:C1190(Current method path:C1201; $code)

$locals:=New object:C1471("foo"; "bar")

ARRAY LONGINT:C221($pos; 0)
ARRAY LONGINT:C221($len; 0)

If (Match regex:C1019("(?s)(?:\\/\\*)(.*)(?:\\*\\/)"; $code; 1; $pos; $len))
	$py:=Substring:C12($code; $pos{1}; $len{1})
	$status:=python($py; $locals)  //warning! do not break the callback method!
End if 
```

Compute a value from locals and read it back:

```4d
$status:=python("{\"total\": sum(values), \"count\": len(values)}"; New object("values"; New collection(3; 4; 5)); python_eval)
If ($status.errorMessage=Null)
	$total:=$status.returnValue.total  // 12
End if 
```

---

## Error handling & troubleshooting

- **`errorMessage` is set.** The code raised an exception (syntax error, `NameError`, an import that isn't available, …). `returnValue` is absent in that case.
- **`returnValue` is an empty object.** The code didn't produce a `dict`: use `python_eval` with an expression that builds a dict, as in the examples.
- **`errorMessage` says Python is not initialized.** The standard library is missing from `Contents/Resources/python/lib` in the plugin bundle. Reinstall the plugin. (Earlier builds crashed 4D at startup in that situation.)
- **`ModuleNotFoundError` for a third-party package.** The interpreter only sees the bundled standard library; see [Requirements & platform notes](#requirements--platform-notes).
- **4D crashes or hangs when `python` runs in a preemptive process.** The plugin doesn't manage Python's global interpreter lock; call it from cooperative processes only.
- **A callback seems to do nothing.** Check the method name passed to `fourd.call`, and that the method returns an Object, Collection or scalar in `$0`.

---

## Quick reference

```4d
$status:=python("{\"value\": 5 + 5}"; New object; python_eval)  // $status.returnValue.value = 10
$status:=python("print('hello')")                                 // python_exec, no value
$status:=python($code; New object("foo"; "bar"))                   // locals: foo
If ($status.errorMessage#Null)
	ALERT($status.errorMessage)
End if 
```

---

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

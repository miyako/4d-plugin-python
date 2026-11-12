# Ownership regressions

`run.sh <source-root> <fixed|baseline>` builds `engine_harness.cpp` with
ASan/UBSan against the real `4DPlugin-python.cpp`, `support/4DPlugin-JSON.cpp`,
the bundled, unmodified 4D Plugin API and the bundled jsoncpp, and runs each
test in its own process. `Call4D` is replaced by a model of the 4D engine that
tracks every string, object, collection and handle, so leaks and double frees
are reported by name. The embedded Python interpreter is stubbed at its C API
boundary: no Python code runs, and libpython is not linked.

`defect_*` tests reproduce issue #6 and fail on the pre-fix tree; `control_*`
tests pass on both. To check a baseline:

    git archive a7abec2c8da8 | tar -x -C /tmp/base
    tests/ownership/run.sh /tmp/base baseline
    tests/ownership/run.sh . fixed

macOS only. Not covered: the Windows build and a real 4D runtime.
